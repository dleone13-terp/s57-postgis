// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// PostGIS database implementation
// Port of Njord's ChartDao.kt and GeoJsonDao.kt

#include "database.hpp"
#include <pqxx/pqxx>
#include <iostream>
#include <sstream>
#include <fstream>

namespace s57 {

// SQL for schema initialization (matching Njord's up.sql)
static const char* SCHEMA_SQL = R"(
CREATE TABLE IF NOT EXISTS meta (
    key     VARCHAR UNIQUE NOT NULL,
    value   VARCHAR NULL
);

INSERT INTO meta VALUES ('version', '1') ON CONFLICT (key) DO NOTHING;

CREATE TABLE IF NOT EXISTS charts (
    id         BIGSERIAL PRIMARY KEY,
    name       VARCHAR UNIQUE           NOT NULL,
    scale      INTEGER                  NOT NULL,
    file_name  VARCHAR                  NOT NULL,
    updated    VARCHAR                  NOT NULL,
    issued     VARCHAR                  NOT NULL,
    zoom       INTEGER                  NOT NULL,
    covr       GEOMETRY(GEOMETRY, 4326) NOT NULL,
    dsid_props JSONB                    NOT NULL,
    chart_txt  JSONB                    NOT NULL
);

CREATE INDEX IF NOT EXISTS charts_gist ON charts USING GIST (covr);
CREATE INDEX IF NOT EXISTS charts_idx ON charts (id);

CREATE TABLE IF NOT EXISTS features (
    id        BIGSERIAL PRIMARY KEY,
    layer     VARCHAR                       NOT NULL,
    geom      GEOMETRY(GEOMETRY, 4326)      NOT NULL,
    props     JSONB                         NOT NULL,
    chart_id  BIGINT REFERENCES charts (id) NOT NULL,
    lnam_refs VARCHAR[]                     NULL,
    z_range   INT4RANGE                     NOT NULL
);

CREATE INDEX IF NOT EXISTS features_gist ON features USING GIST (geom);
CREATE INDEX IF NOT EXISTS features_idx ON features (id);
CREATE INDEX IF NOT EXISTS features_layer_idx ON features (layer);
CREATE INDEX IF NOT EXISTS features_zoom_idx ON features USING GIST (z_range);
CREATE INDEX IF NOT EXISTS features_lnam_idx ON features USING GIN (lnam_refs);
)";

Database::Database(const std::string& connectionString) {
    try {
        conn_ = std::make_unique<pqxx::connection>(connectionString);
    } catch (const std::exception& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        conn_ = nullptr;
    }
}

Database::~Database() {
    if (inTransaction_) {
        try {
            rollbackTransaction();
        } catch (...) {}
    }
}

bool Database::isConnected() const {
    return conn_ && conn_->is_open();
}

bool Database::execute(const std::string& sql) {
    if (!isConnected()) return false;
    
    try {
        pqxx::work txn(*conn_);
        txn.exec(sql);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "SQL execution failed: " << e.what() << std::endl;
        return false;
    }
}

bool Database::initSchema() {
    if (!isConnected()) return false;

    try {
        // First ensure PostGIS extension exists
        pqxx::work txn(*conn_);
        txn.exec("CREATE EXTENSION IF NOT EXISTS postgis");
        txn.exec(SCHEMA_SQL);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Schema initialization failed: " << e.what() << std::endl;
        return false;
    }
}

std::optional<int64_t> Database::insertChart(const ChartInfo& chart) {
    if (!isConnected()) return std::nullopt;

    try {
        pqxx::work txn(*conn_);
        
        // SQL matching Njord's ChartDao.insertChart()
        pqxx::result result = txn.exec_params(
            R"(INSERT INTO charts (name, scale, file_name, updated, issued, zoom, covr, dsid_props, chart_txt) 
               VALUES ($1, $2, $3, $4, $5, $6, ST_SetSRID(ST_GeomFromGeoJSON($7), 4326), $8::jsonb, $9::jsonb)
               RETURNING id)",
            chart.name,
            chart.scale,
            chart.fileName,
            chart.updated,
            chart.issued,
            chart.zoom,
            chart.covrGeoJson,
            chart.dsidProps,
            chart.chartTxt
        );
        
        txn.commit();
        
        if (!result.empty()) {
            return result[0][0].as<int64_t>();
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Chart insertion failed: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::string Database::lnamRefsToArrayLiteral(const std::vector<std::string>& refs) {
    if (refs.empty()) {
        return "NULL";
    }
    
    std::ostringstream ss;
    ss << "ARRAY[";
    bool first = true;
    for (const auto& ref : refs) {
        if (!first) ss << ",";
        first = false;
        // Escape single quotes in the string
        std::string escaped;
        for (char c : ref) {
            if (c == '\'') escaped += "''";
            else escaped += c;
        }
        ss << "'" << escaped << "'";
    }
    ss << "]::varchar[]";
    return ss.str();
}

bool Database::insertFeature(int64_t chartId, const Feature& feature) {
    if (!isConnected()) return false;

    try {
        pqxx::work txn(*conn_);
        
        std::string lnamRefsLiteral = lnamRefsToArrayLiteral(feature.lnamRefs);
        
        // SQL matching Njord's GeoJsonDao.insertFeature()
        std::ostringstream sql;
        sql << "INSERT INTO features (layer, geom, props, chart_id, lnam_refs, z_range) "
            << "VALUES ($1, ST_SetSRID(ST_GeomFromGeoJSON($2), 4326), $3::jsonb, $4, "
            << lnamRefsLiteral << ", int4range($5, $6))";
        
        txn.exec_params(
            sql.str(),
            feature.layer,
            feature.geomGeoJson,
            feature.propsJson,
            chartId,
            feature.minZ,
            feature.maxZ
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Feature insertion failed: " << e.what() << std::endl;
        return false;
    }
}

bool Database::insertFeatures(int64_t chartId, const std::vector<Feature>& features) {
    if (!isConnected()) return false;
    if (features.empty()) return true;

    try {
        pqxx::work txn(*conn_);
        
        for (const auto& feature : features) {
            std::string lnamRefsLiteral = lnamRefsToArrayLiteral(feature.lnamRefs);
            
            std::ostringstream sql;
            sql << "INSERT INTO features (layer, geom, props, chart_id, lnam_refs, z_range) "
                << "VALUES ($1, ST_SetSRID(ST_GeomFromGeoJSON($2), 4326), $3::jsonb, $4, "
                << lnamRefsLiteral << ", int4range($5, $6))";
            
            txn.exec_params(
                sql.str(),
                feature.layer,
                feature.geomGeoJson,
                feature.propsJson,
                chartId,
                feature.minZ,
                feature.maxZ
            );
        }
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Batch feature insertion failed: " << e.what() << std::endl;
        return false;
    }
}

bool Database::chartExists(const std::string& name) {
    if (!isConnected()) return false;

    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec_params(
            "SELECT COUNT(*) FROM charts WHERE name = $1",
            name
        );
        txn.commit();
        
        return !result.empty() && result[0][0].as<int64_t>() > 0;
    } catch (const std::exception& e) {
        std::cerr << "Chart exists check failed: " << e.what() << std::endl;
        return false;
    }
}

bool Database::deleteChart(const std::string& name) {
    if (!isConnected()) return false;

    try {
        pqxx::work txn(*conn_);
        
        // First get chart ID
        pqxx::result idResult = txn.exec_params(
            "SELECT id FROM charts WHERE name = $1",
            name
        );
        
        if (idResult.empty()) {
            txn.commit();
            return true; // Chart doesn't exist, nothing to delete
        }
        
        int64_t chartId = idResult[0][0].as<int64_t>();
        
        // Delete features first (foreign key constraint)
        txn.exec_params("DELETE FROM features WHERE chart_id = $1", chartId);
        
        // Delete chart
        txn.exec_params("DELETE FROM charts WHERE id = $1", chartId);
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Chart deletion failed: " << e.what() << std::endl;
        return false;
    }
}

int64_t Database::getChartCount() {
    if (!isConnected()) return 0;

    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec("SELECT COUNT(*) FROM charts");
        txn.commit();
        
        return result.empty() ? 0 : result[0][0].as<int64_t>();
    } catch (const std::exception& e) {
        std::cerr << "Chart count failed: " << e.what() << std::endl;
        return 0;
    }
}

int64_t Database::getFeatureCount() {
    if (!isConnected()) return 0;

    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec("SELECT COUNT(*) FROM features");
        txn.commit();
        
        return result.empty() ? 0 : result[0][0].as<int64_t>();
    } catch (const std::exception& e) {
        std::cerr << "Feature count failed: " << e.what() << std::endl;
        return 0;
    }
}

void Database::beginTransaction() {
    // Note: pqxx handles transactions per work object
    // This is a placeholder for explicit transaction management if needed
    inTransaction_ = true;
}

void Database::commitTransaction() {
    inTransaction_ = false;
}

void Database::rollbackTransaction() {
    inTransaction_ = false;
}

} // namespace s57
