// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// ChartIngest class header
// Port of Njord's ChartIngest.kt

#ifndef S57_POSTGIS_INGEST_HPP
#define S57_POSTGIS_INGEST_HPP

#include "types.hpp"
#include "database.hpp"
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace s57 {

// Progress callback type
using ProgressCallback = std::function<void(int current, int total, const std::string& fileName)>;

// ChartIngest class for batch processing S-57 files
// Port of Njord's ChartIngest.kt
class ChartIngest {
public:
    // Constructor
    explicit ChartIngest(Database& database);

    // Set the number of worker threads
    void setWorkerCount(int count);

    // Set progress callback
    void setProgressCallback(ProgressCallback callback);

    // Set verbose mode
    void setVerbose(bool verbose);

    // Find all .000 files in a directory
    static std::vector<std::string> findS57Files(const std::string& path, bool recursive);

    // Process a single file
    ProcessingResult processFile(const std::string& filePath);

    // Process multiple files
    std::vector<ProcessingResult> processFiles(const std::vector<std::string>& files);

    // Process a directory
    std::vector<ProcessingResult> processDirectory(const std::string& dirPath, bool recursive);

    // Get processing statistics
    struct Statistics {
        int totalFiles = 0;
        int successCount = 0;
        int failCount = 0;
        int totalFeatures = 0;
    };

    Statistics getStatistics() const;

private:
    Database& database_;
    int workerCount_ = 4;
    bool verbose_ = false;
    ProgressCallback progressCallback_;
    
    std::atomic<int> processedCount_{0};
    std::atomic<int> successCount_{0};
    std::atomic<int> failCount_{0};
    std::atomic<int> totalFeatures_{0};
};

} // namespace s57

#endif // S57_POSTGIS_INGEST_HPP
