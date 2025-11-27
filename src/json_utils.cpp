// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// JSON utilities implementation

#include "json_utils.hpp"
#include <sstream>
#include <iomanip>

namespace s57 {
namespace json {

std::string escapeString(const std::string& str) {
    std::ostringstream ss;
    for (char c : str) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                       << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

std::string toJsonObject(const std::map<std::string, std::string>& props) {
    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& [key, value] : props) {
        if (!first) ss << ",";
        first = false;
        ss << "\"" << escapeString(key) << "\":\"" << escapeString(value) << "\"";
    }
    ss << "}";
    return ss.str();
}

std::string toJsonArray(const std::vector<std::string>& items) {
    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& item : items) {
        if (!first) ss << ",";
        first = false;
        ss << "\"" << escapeString(item) << "\"";
    }
    ss << "]";
    return ss.str();
}

std::string pointToGeoJson(double x, double y) {
    std::ostringstream ss;
    ss << std::setprecision(15);
    ss << "{\"type\":\"Point\",\"coordinates\":[" << x << "," << y << "]}";
    return ss.str();
}

std::string pointToGeoJson(double x, double y, double z) {
    std::ostringstream ss;
    ss << std::setprecision(15);
    ss << "{\"type\":\"Point\",\"coordinates\":[" << x << "," << y << "," << z << "]}";
    return ss.str();
}

} // namespace json
} // namespace s57
