// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// JSON utilities header
// Port of Njord's JSON handling

#ifndef S57_POSTGIS_JSON_UTILS_HPP
#define S57_POSTGIS_JSON_UTILS_HPP

#include <string>
#include <vector>
#include <map>

namespace s57 {
namespace json {

// Escape a string for JSON
std::string escapeString(const std::string& str);

// Convert a map of key-value pairs to a JSON object string
std::string toJsonObject(const std::map<std::string, std::string>& props);

// Create a JSON array from a vector of strings
std::string toJsonArray(const std::vector<std::string>& items);

// Create a simple GeoJSON point
std::string pointToGeoJson(double x, double y);

// Create a simple GeoJSON point with Z
std::string pointToGeoJson(double x, double y, double z);

} // namespace json
} // namespace s57

#endif // S57_POSTGIS_JSON_UTILS_HPP
