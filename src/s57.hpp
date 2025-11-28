// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// S57 class header - S-57 file processing
// Port of Njord's S57.kt

#ifndef S57_POSTGIS_S57_HPP
#define S57_POSTGIS_S57_HPP

#include "types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations for GDAL types
class GDALDataset;

namespace s57 {

// S57 class for processing S-57 (.000) chart files
// This is a port of Njord's S57.kt class
class S57 {
public:
    // Constructor - opens the S-57 file
    explicit S57(const std::string& filePath);
    
    // Destructor - closes the file
    ~S57();

    // Prevent copying
    S57(const S57&) = delete;
    S57& operator=(const S57&) = delete;

    // Allow moving
    S57(S57&& other) noexcept;
    S57& operator=(S57&& other) noexcept;

    // Check if file was opened successfully
    bool isOpen() const;

    // Get the file path
    const std::string& getFilePath() const;

    // Get chart metadata
    ChartInfo getChartInfo() const;

    // Get list of layer names
    std::vector<std::string> getLayerNames() const;

    // Get all features from a layer
    std::vector<Feature> getLayerFeatures(const std::string& layerName) const;

    // Get all features from all non-excluded layers
    std::vector<Feature> getAllFeatures() const;

    // Process all features with a callback (for streaming)
    void processFeatures(const std::function<void(const Feature&)>& callback) const;

    // Get DSID layer properties
    std::map<std::string, std::string> getDsidProperties() const;

    // Get M_COVR layer properties (chart text)
    std::map<std::string, std::string> getMCovrProperties() const;

    // Get coverage geometry as GeoJSON
    std::string getCoverageGeoJson() const;

private:
    std::string filePath_;
    GDALDataset* dataset_ = nullptr;
    bool initialized_ = false;

    // Extract properties from a feature
    std::map<std::string, std::string> extractProperties(void* feature) const;

    // Convert OGR geometry to GeoJSON
    std::string geometryToGeoJson(void* geometry) const;

    // Get SCAMIN/SCAMAX from feature properties
    std::pair<int, int> getScaleRange(const std::map<std::string, std::string>& props) const;

    // Check if a layer should be excluded
    static bool isExcludedLayer(const std::string& layerName);
};

} // namespace s57

#endif // S57_POSTGIS_S57_HPP
