// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// PostGIS database header
// Port of Njord's ChartDao.kt and GeoJsonDao.kt

#ifndef S57_POSTGIS_DATABASE_HPP
#define S57_POSTGIS_DATABASE_HPP

#include "types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>

// Forward declaration
namespace pqxx {
    class connection;
}

namespace s57 {

// Database class for PostGIS operations
// Port of Njord's ChartDao and GeoJsonDao
class Database {
public:
    // Constructor with connection string
    explicit Database(const std::string& connectionString);
    
    // Destructor
    ~Database();

    // Prevent copying
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Check if connected
    bool isConnected() const;

    // Initialize the database schema
    bool initSchema();

    // Insert a chart and return its ID
    // Port of ChartDao.insertChart()
    std::optional<int64_t> insertChart(const ChartInfo& chart);

    // Insert a feature
    // Port of GeoJsonDao.insertFeature()
    bool insertFeature(int64_t chartId, const Feature& feature);

    // Insert multiple features in a batch (for performance)
    bool insertFeatures(int64_t chartId, const std::vector<Feature>& features);

    // Check if a chart exists by name
    bool chartExists(const std::string& name);

    // Delete a chart by name (and all its features)
    bool deleteChart(const std::string& name);

    // Get chart count
    int64_t getChartCount();

    // Get feature count
    int64_t getFeatureCount();

    // Begin a transaction
    void beginTransaction();

    // Commit the current transaction
    void commitTransaction();

    // Rollback the current transaction
    void rollbackTransaction();

private:
    std::unique_ptr<pqxx::connection> conn_;
    bool inTransaction_ = false;

    // Execute a SQL statement
    bool execute(const std::string& sql);

    // Convert LNAM refs to PostgreSQL array literal
    std::string lnamRefsToArrayLiteral(const std::vector<std::string>& refs);
};

} // namespace s57

#endif // S57_POSTGIS_DATABASE_HPP
