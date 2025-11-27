// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// S57 class implementation - S-57 file processing
// Port of Njord's S57.kt

#include "s57.hpp"
#include "zfinder.hpp"
#include "json_utils.hpp"

#include <gdal.h>
#include <ogrsf_frmts.h>
#include <cpl_conv.h>

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstring>

namespace s57 {

namespace {
    // GDAL initialization helper
    class GDALInit {
    public:
        GDALInit() {
            GDALAllRegister();
            // Set S-57 options matching Njord exactly
            CPLSetConfigOption("OGR_S57_OPTIONS", GDAL_S57_OPTIONS);
        }
    };
    
    // Ensure GDAL is initialized
    static GDALInit& initGDAL() {
        static GDALInit init;
        return init;
    }
}

S57::S57(const std::string& filePath) : filePath_(filePath) {
    initGDAL();
    
    dataset_ = static_cast<GDALDataset*>(
        GDALOpenEx(filePath.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, 
                   nullptr, nullptr, nullptr));
    
    initialized_ = (dataset_ != nullptr);
}

S57::~S57() {
    if (dataset_) {
        GDALClose(dataset_);
        dataset_ = nullptr;
    }
}

S57::S57(S57&& other) noexcept 
    : filePath_(std::move(other.filePath_))
    , dataset_(other.dataset_)
    , initialized_(other.initialized_) {
    other.dataset_ = nullptr;
    other.initialized_ = false;
}

S57& S57::operator=(S57&& other) noexcept {
    if (this != &other) {
        if (dataset_) {
            GDALClose(dataset_);
        }
        filePath_ = std::move(other.filePath_);
        dataset_ = other.dataset_;
        initialized_ = other.initialized_;
        other.dataset_ = nullptr;
        other.initialized_ = false;
    }
    return *this;
}

bool S57::isOpen() const {
    return initialized_ && dataset_ != nullptr;
}

const std::string& S57::getFilePath() const {
    return filePath_;
}

std::vector<std::string> S57::getLayerNames() const {
    std::vector<std::string> names;
    if (!isOpen()) return names;

    int layerCount = dataset_->GetLayerCount();
    for (int i = 0; i < layerCount; ++i) {
        OGRLayer* layer = dataset_->GetLayer(i);
        if (layer) {
            names.push_back(layer->GetName());
        }
    }
    return names;
}

bool S57::isExcludedLayer(const std::string& layerName) {
    for (const auto& excluded : EXCLUDED_LAYERS) {
        if (layerName == excluded) {
            return true;
        }
    }
    return false;
}

std::map<std::string, std::string> S57::extractProperties(void* featurePtr) const {
    std::map<std::string, std::string> props;
    OGRFeature* feature = static_cast<OGRFeature*>(featurePtr);
    if (!feature) return props;

    int fieldCount = feature->GetFieldCount();
    for (int i = 0; i < fieldCount; ++i) {
        const char* fieldName = feature->GetFieldDefnRef(i)->GetNameRef();
        if (feature->IsFieldSet(i) && !feature->IsFieldNull(i)) {
            const char* value = feature->GetFieldAsString(i);
            if (value && strlen(value) > 0) {
                props[fieldName] = value;
            }
        }
    }
    return props;
}

std::string S57::geometryToGeoJson(void* geometryPtr) const {
    OGRGeometry* geometry = static_cast<OGRGeometry*>(geometryPtr);
    if (!geometry) return "{}";

    // Transform to WGS84 if needed
    OGRSpatialReference* wgs84 = new OGRSpatialReference();
    wgs84->SetWellKnownGeogCS("WGS84");
    wgs84->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    const OGRSpatialReference* srcSRS = geometry->getSpatialReference();
    if (srcSRS && !srcSRS->IsSame(wgs84)) {
        OGRCoordinateTransformation* transform = 
            OGRCreateCoordinateTransformation(srcSRS, wgs84);
        if (transform) {
            geometry->transform(transform);
            OGRCoordinateTransformation::DestroyCT(transform);
        }
    }
    
    wgs84->Release();

    // Export to GeoJSON
    char* json = geometry->exportToJson();
    std::string result = json ? json : "{}";
    CPLFree(json);
    
    return result;
}

std::pair<int, int> S57::getScaleRange(const std::map<std::string, std::string>& props) const {
    int scamin = 0;
    int scamax = 0;

    auto it = props.find("SCAMIN");
    if (it != props.end()) {
        try {
            scamin = std::stoi(it->second);
        } catch (...) {}
    }

    it = props.find("SCAMAX");
    if (it != props.end()) {
        try {
            scamax = std::stoi(it->second);
        } catch (...) {}
    }

    return ZFinder::calculateZRange(scamin, scamax);
}

std::map<std::string, std::string> S57::getDsidProperties() const {
    std::map<std::string, std::string> props;
    if (!isOpen()) return props;

    OGRLayer* dsidLayer = dataset_->GetLayerByName("DSID");
    if (!dsidLayer) return props;

    dsidLayer->ResetReading();
    OGRFeature* feature = dsidLayer->GetNextFeature();
    if (feature) {
        props = extractProperties(feature);
        OGRFeature::DestroyFeature(feature);
    }
    
    return props;
}

std::map<std::string, std::string> S57::getMCovrProperties() const {
    std::map<std::string, std::string> props;
    if (!isOpen()) return props;

    OGRLayer* mcovrLayer = dataset_->GetLayerByName("M_COVR");
    if (!mcovrLayer) return props;

    mcovrLayer->ResetReading();
    OGRFeature* feature = mcovrLayer->GetNextFeature();
    if (feature) {
        props = extractProperties(feature);
        OGRFeature::DestroyFeature(feature);
    }
    
    return props;
}

std::string S57::getCoverageGeoJson() const {
    if (!isOpen()) return "{}";

    OGRLayer* mcovrLayer = dataset_->GetLayerByName("M_COVR");
    if (!mcovrLayer) return "{}";

    mcovrLayer->ResetReading();
    OGRFeature* feature = mcovrLayer->GetNextFeature();
    if (!feature) return "{}";

    OGRGeometry* geometry = feature->GetGeometryRef();
    std::string result = geometryToGeoJson(geometry);
    OGRFeature::DestroyFeature(feature);
    
    return result;
}

ChartInfo S57::getChartInfo() const {
    ChartInfo info;
    if (!isOpen()) return info;

    // Get DSID properties
    auto dsidProps = getDsidProperties();
    
    // Chart name from DSNM
    auto it = dsidProps.find("DSNM");
    if (it != dsidProps.end()) {
        info.name = it->second;
    } else {
        // Fallback to filename
        info.name = std::filesystem::path(filePath_).stem().string();
    }

    // Scale from DSPM_CSCL
    it = dsidProps.find("DSPM_CSCL");
    if (it != dsidProps.end()) {
        try {
            info.scale = std::stoi(it->second);
        } catch (...) {
            info.scale = 0;
        }
    }

    // Update date from UADT
    it = dsidProps.find("UADT");
    if (it != dsidProps.end()) {
        info.updated = it->second;
    }

    // Issue date from ISDT
    it = dsidProps.find("ISDT");
    if (it != dsidProps.end()) {
        info.issued = it->second;
    }

    // File name
    info.fileName = std::filesystem::path(filePath_).filename().string();

    // Calculate zoom from scale
    if (info.scale > 0) {
        info.zoom = ZFinder::findZoom(info.scale);
    }

    // Coverage geometry
    info.covrGeoJson = getCoverageGeoJson();

    // DSID properties as JSON
    info.dsidProps = json::toJsonObject(dsidProps);

    // M_COVR properties as chart text
    auto mcovrProps = getMCovrProperties();
    info.chartTxt = json::toJsonObject(mcovrProps);

    return info;
}

std::vector<Feature> S57::getLayerFeatures(const std::string& layerName) const {
    std::vector<Feature> features;
    if (!isOpen()) return features;
    if (isExcludedLayer(layerName)) return features;

    OGRLayer* layer = dataset_->GetLayerByName(layerName.c_str());
    if (!layer) return features;

    // Get chart info for zoom calculation (used by caller if needed)
    // auto chartInfo = getChartInfo();
    // int baseZoom = chartInfo.zoom;

    layer->ResetReading();
    OGRFeature* ogrFeature;
    
    while ((ogrFeature = layer->GetNextFeature()) != nullptr) {
        Feature feat;
        feat.layer = layerName;

        // Extract properties
        auto props = extractProperties(ogrFeature);

        // Special handling for SOUNDG layer - extract METERS from Z coordinate
        if (layerName == "SOUNDG") {
            OGRGeometry* geom = ogrFeature->GetGeometryRef();
            if (geom) {
                OGRwkbGeometryType geomType = wkbFlatten(geom->getGeometryType());
                if (geomType == wkbPoint) {
                    OGRPoint* point = static_cast<OGRPoint*>(geom);
                    if (point->Is3D()) {
                        double meters = point->getZ();
                        std::ostringstream ss;
                        ss << std::fixed << std::setprecision(1) << meters;
                        props["METERS"] = ss.str();
                    }
                }
            }
        }

        // Get geometry as GeoJSON
        OGRGeometry* geometry = ogrFeature->GetGeometryRef();
        if (geometry) {
            feat.geomGeoJson = geometryToGeoJson(geometry);
        }

        // Properties to JSON
        feat.propsJson = json::toJsonObject(props);

        // Calculate zoom range
        auto [minZ, maxZ] = getScaleRange(props);
        feat.minZ = minZ;
        feat.maxZ = maxZ;

        // Extract LNAM_REFS if present
        int lnamRefsIdx = ogrFeature->GetFieldIndex("LNAM_REFS");
        if (lnamRefsIdx >= 0 && ogrFeature->IsFieldSet(lnamRefsIdx)) {
            int count = 0;
            const char* const* refs = ogrFeature->GetFieldAsStringList(lnamRefsIdx);
            if (refs) {
                while (refs[count]) {
                    feat.lnamRefs.push_back(refs[count]);
                    ++count;
                }
            }
        }

        features.push_back(std::move(feat));
        OGRFeature::DestroyFeature(ogrFeature);
    }

    return features;
}

std::vector<Feature> S57::getAllFeatures() const {
    std::vector<Feature> allFeatures;
    if (!isOpen()) return allFeatures;

    auto layerNames = getLayerNames();
    for (const auto& layerName : layerNames) {
        if (!isExcludedLayer(layerName)) {
            auto features = getLayerFeatures(layerName);
            allFeatures.insert(allFeatures.end(), 
                              std::make_move_iterator(features.begin()),
                              std::make_move_iterator(features.end()));
        }
    }

    return allFeatures;
}

void S57::processFeatures(const std::function<void(const Feature&)>& callback) const {
    if (!isOpen()) return;

    auto layerNames = getLayerNames();
    for (const auto& layerName : layerNames) {
        if (!isExcludedLayer(layerName)) {
            auto features = getLayerFeatures(layerName);
            for (const auto& feat : features) {
                callback(feat);
            }
        }
    }
}

} // namespace s57
