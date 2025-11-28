// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// Common types and structures for S57-PostGIS
// Port of Njord's data structures

#ifndef S57_POSTGIS_TYPES_HPP
#define S57_POSTGIS_TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace s57 {

// Chart metadata structure
struct ChartInfo {
    std::string name;           // Chart name (from DSID.DSNM)
    int scale = 0;              // Chart scale (from DSID.DSPM_CSCL)
    std::string fileName;       // Source file name
    std::string updated;        // Update date (DSID.UADT)
    std::string issued;         // Issue date (DSID.ISDT)
    int zoom = 0;               // Calculated zoom level
    std::string covrGeoJson;    // Coverage geometry as GeoJSON
    std::string dsidProps;      // DSID properties as JSON
    std::string chartTxt;       // Chart text as JSON (M_COVR properties)
};

// Feature structure
struct Feature {
    std::string layer;          // Layer name
    std::string geomGeoJson;    // Geometry as GeoJSON
    std::string propsJson;      // Properties as JSON
    int minZ = 0;               // Minimum zoom level
    int maxZ = 28;              // Maximum zoom level
    std::vector<std::string> lnamRefs;  // LNAM references
};

// Processing result
struct ProcessingResult {
    bool success = false;
    std::string fileName;
    std::string chartName;
    int featureCount = 0;
    std::string errorMessage;
};

// Processing options
struct ProcessingOptions {
    std::string databaseUrl = "postgresql://localhost/njord";
    int workers = 4;
    bool recursive = false;
    bool verbose = false;
    bool listOnly = false;
    bool infoOnly = false;
    bool initSchema = false;
};

// Excluded layers that should not be processed as features
inline const std::vector<std::string> EXCLUDED_LAYERS = {
    "DSID",
    "IsolatedNode",
    "ConnectedNode", 
    "Edge",
    "Face"
};

// GDAL options matching Njord exactly
inline const char* GDAL_S57_OPTIONS = 
    "RETURN_PRIMITIVES=OFF,"
    "RETURN_LINKAGES=OFF,"
    "LNAM_REFS=ON,"
    "UPDATES=APPLY,"
    "SPLIT_MULTIPOINT=ON,"
    "RECODE_BY_DSSI=ON:"
    "ADD_SOUNDG_DEPTH=ON";

} // namespace s57

#endif // S57_POSTGIS_TYPES_HPP
