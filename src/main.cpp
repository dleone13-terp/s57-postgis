// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// Main entry point with CLI
// S57-PostGIS - C++ port of Njord's S-57 chart processing

#include "types.hpp"
#include "s57.hpp"
#include "database.hpp"
#include "ingest.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// Program version
const char* VERSION = "1.0.0";

// Print usage information
void printUsage(const char* progName) {
    std::cout << "S57-PostGIS v" << VERSION << "\n"
              << "C++ port of Njord's S-57 chart processing system\n\n"
              << "Usage: " << progName << " <input> [options]\n\n"
              << "Input:\n"
              << "  <input>                 S-57 file (.000) or directory\n\n"
              << "Database Options:\n"
              << "  -d, --database <conn>   PostgreSQL connection string\n"
              << "                          Default: postgresql://localhost/njord\n"
              << "  --init-schema           Initialize database schema\n\n"
              << "Processing Options:\n"
              << "  -w, --workers <n>       Number of parallel workers (default: 4)\n"
              << "  -r, --recursive         Recursively search directories\n"
              << "  -v, --verbose           Verbose output\n\n"
              << "Other Options:\n"
              << "  --list                  List all .000 files found\n"
              << "  --info                  Show chart metadata (for single file)\n"
              << "  -h, --help              Show this help\n"
              << "  --version               Show version\n\n"
              << "Examples:\n"
              << "  " << progName << " chart.000 -d postgresql://localhost/njord\n"
              << "  " << progName << " /charts -r -v\n"
              << "  " << progName << " /charts --list\n"
              << std::endl;
}

// Print version
void printVersion() {
    std::cout << "s57-postgis " << VERSION << std::endl;
}

// Show chart info
void showChartInfo(const std::string& filePath) {
    s57::S57 chart(filePath);
    
    if (!chart.isOpen()) {
        std::cerr << "Error: Failed to open " << filePath << std::endl;
        return;
    }
    
    auto info = chart.getChartInfo();
    
    std::cout << "Chart Information:\n"
              << "  Name:     " << info.name << "\n"
              << "  Scale:    1:" << info.scale << "\n"
              << "  File:     " << info.fileName << "\n"
              << "  Updated:  " << info.updated << "\n"
              << "  Issued:   " << info.issued << "\n"
              << "  Zoom:     " << info.zoom << "\n\n"
              << "Layers:\n";
    
    auto layers = chart.getLayerNames();
    for (const auto& layer : layers) {
        std::cout << "  - " << layer << "\n";
    }
    
    std::cout << "\nDSID Properties:\n" << info.dsidProps << "\n\n"
              << "Coverage:\n" << info.covrGeoJson << "\n"
              << std::endl;
}

// List files
void listFiles(const std::string& path, bool recursive) {
    auto files = s57::ChartIngest::findS57Files(path, recursive);
    
    std::cout << "Found " << files.size() << " S-57 files:\n";
    for (const auto& file : files) {
        std::cout << "  " << file << "\n";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Default options
    s57::ProcessingOptions opts;
    std::string inputPath;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--version") {
            printVersion();
            return 0;
        }
        if (arg == "-d" || arg == "--database") {
            if (i + 1 < argc) {
                opts.databaseUrl = argv[++i];
            } else {
                std::cerr << "Error: --database requires a connection string\n";
                return 1;
            }
            continue;
        }
        if (arg == "-w" || arg == "--workers") {
            if (i + 1 < argc) {
                opts.workers = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --workers requires a number\n";
                return 1;
            }
            continue;
        }
        if (arg == "-r" || arg == "--recursive") {
            opts.recursive = true;
            continue;
        }
        if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
            continue;
        }
        if (arg == "--list") {
            opts.listOnly = true;
            continue;
        }
        if (arg == "--info") {
            opts.infoOnly = true;
            continue;
        }
        if (arg == "--init-schema") {
            opts.initSchema = true;
            continue;
        }
        
        // Input path
        if (inputPath.empty() && arg[0] != '-') {
            inputPath = arg;
        }
    }
    
    // Handle --list
    if (opts.listOnly) {
        if (inputPath.empty()) {
            std::cerr << "Error: No input path specified\n";
            return 1;
        }
        listFiles(inputPath, opts.recursive);
        return 0;
    }
    
    // Handle --info
    if (opts.infoOnly) {
        if (inputPath.empty()) {
            std::cerr << "Error: No input file specified\n";
            return 1;
        }
        showChartInfo(inputPath);
        return 0;
    }
    
    // Handle --init-schema only
    if (opts.initSchema && inputPath.empty()) {
        std::cout << "Initializing database schema..." << std::endl;
        s57::Database db(opts.databaseUrl);
        if (!db.isConnected()) {
            std::cerr << "Error: Failed to connect to database" << std::endl;
            return 1;
        }
        if (db.initSchema()) {
            std::cout << "Schema initialized successfully." << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Failed to initialize schema" << std::endl;
            return 1;
        }
    }
    
    // Validate input
    if (inputPath.empty()) {
        std::cerr << "Error: No input specified\n\n";
        printUsage(argv[0]);
        return 1;
    }
    
    if (!fs::exists(inputPath)) {
        std::cerr << "Error: Input path does not exist: " << inputPath << std::endl;
        return 1;
    }
    
    // Connect to database
    if (opts.verbose) {
        std::cout << "Connecting to database: " << opts.databaseUrl << std::endl;
    }
    
    s57::Database db(opts.databaseUrl);
    if (!db.isConnected()) {
        std::cerr << "Error: Failed to connect to database" << std::endl;
        return 1;
    }
    
    // Initialize schema if requested
    if (opts.initSchema) {
        std::cout << "Initializing database schema..." << std::endl;
        if (!db.initSchema()) {
            std::cerr << "Error: Failed to initialize schema" << std::endl;
            return 1;
        }
    }
    
    // Create ingest processor
    s57::ChartIngest ingest(db);
    ingest.setWorkerCount(opts.workers);
    ingest.setVerbose(opts.verbose);
    
    // Set progress callback
    if (!opts.verbose) {
        ingest.setProgressCallback([](int current, int total, const std::string& fileName) {
            std::cout << "\rProcessing: " << current << "/" << total 
                      << " (" << fileName << ")          " << std::flush;
        });
    }
    
    // Process input
    std::vector<s57::ProcessingResult> results;
    
    if (fs::is_regular_file(inputPath)) {
        // Single file
        auto result = ingest.processFile(inputPath);
        results.push_back(result);
    } else {
        // Directory
        results = ingest.processDirectory(inputPath, opts.recursive);
    }
    
    // Print summary
    if (!opts.verbose) {
        std::cout << std::endl;
    }
    
    auto stats = ingest.getStatistics();
    std::cout << "\nProcessing Complete:\n"
              << "  Files processed: " << stats.totalFiles << "\n"
              << "  Successful:      " << stats.successCount << "\n"
              << "  Failed:          " << stats.failCount << "\n"
              << "  Total features:  " << stats.totalFeatures << "\n";
    
    // Print failures
    if (stats.failCount > 0) {
        std::cout << "\nFailed files:\n";
        for (const auto& result : results) {
            if (!result.success) {
                std::cout << "  " << result.fileName << ": " << result.errorMessage << "\n";
            }
        }
    }
    
    return stats.failCount > 0 ? 1 : 0;
}
