// Copyright 2024 S57-PostGIS Authors
// SPDX-License-Identifier: Apache-2.0
//
// ChartIngest class implementation
// Port of Njord's ChartIngest.kt

#include "ingest.hpp"
#include "s57.hpp"
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace fs = std::filesystem;

namespace s57 {

ChartIngest::ChartIngest(Database& database) 
    : database_(database) {
}

void ChartIngest::setWorkerCount(int count) {
    workerCount_ = std::max(1, count);
}

void ChartIngest::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = std::move(callback);
}

void ChartIngest::setVerbose(bool verbose) {
    verbose_ = verbose;
}

std::vector<std::string> ChartIngest::findS57Files(const std::string& path, bool recursive) {
    std::vector<std::string> files;
    
    fs::path inputPath(path);
    
    if (!fs::exists(inputPath)) {
        return files;
    }
    
    if (fs::is_regular_file(inputPath)) {
        // Single file
        if (inputPath.extension() == ".000") {
            files.push_back(inputPath.string());
        }
        return files;
    }
    
    if (!fs::is_directory(inputPath)) {
        return files;
    }
    
    // Directory
    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(inputPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".000") {
                files.push_back(entry.path().string());
            }
        }
    } else {
        for (const auto& entry : fs::directory_iterator(inputPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".000") {
                files.push_back(entry.path().string());
            }
        }
    }
    
    // Sort for consistent ordering
    std::sort(files.begin(), files.end());
    
    return files;
}

ProcessingResult ChartIngest::processFile(const std::string& filePath) {
    ProcessingResult result;
    result.fileName = fs::path(filePath).filename().string();
    
    try {
        // Open and parse the S-57 file
        S57 s57(filePath);
        
        if (!s57.isOpen()) {
            result.success = false;
            result.errorMessage = "Failed to open file";
            return result;
        }
        
        // Get chart info
        ChartInfo chartInfo = s57.getChartInfo();
        result.chartName = chartInfo.name;
        
        if (verbose_) {
            std::cout << "Processing: " << chartInfo.name 
                      << " (scale 1:" << chartInfo.scale << ")" << std::endl;
        }
        
        // Check if chart already exists
        if (database_.chartExists(chartInfo.name)) {
            if (verbose_) {
                std::cout << "  Updating existing chart: " << chartInfo.name << std::endl;
            }
            database_.deleteChart(chartInfo.name);
        }
        
        // Insert chart
        auto chartIdOpt = database_.insertChart(chartInfo);
        if (!chartIdOpt.has_value()) {
            result.success = false;
            result.errorMessage = "Failed to insert chart";
            return result;
        }
        
        int64_t chartId = chartIdOpt.value();
        
        // Get all features
        auto features = s57.getAllFeatures();
        result.featureCount = static_cast<int>(features.size());
        
        if (verbose_) {
            std::cout << "  Found " << features.size() << " features" << std::endl;
        }
        
        // Insert features in batches
        const size_t batchSize = 1000;
        for (size_t i = 0; i < features.size(); i += batchSize) {
            size_t end = std::min(i + batchSize, features.size());
            std::vector<Feature> batch(
                features.begin() + static_cast<long>(i),
                features.begin() + static_cast<long>(end)
            );
            
            if (!database_.insertFeatures(chartId, batch)) {
                result.success = false;
                result.errorMessage = "Failed to insert features";
                return result;
            }
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }
    
    return result;
}

std::vector<ProcessingResult> ChartIngest::processFiles(const std::vector<std::string>& files) {
    std::vector<ProcessingResult> results;
    results.reserve(files.size());
    
    processedCount_ = 0;
    successCount_ = 0;
    failCount_ = 0;
    totalFeatures_ = 0;
    
    int total = static_cast<int>(files.size());
    
    // For simplicity, process files sequentially
    // Multi-threading would require connection pooling for database
    for (const auto& file : files) {
        ProcessingResult result = processFile(file);
        results.push_back(result);
        
        ++processedCount_;
        if (result.success) {
            ++successCount_;
            totalFeatures_ += result.featureCount;
        } else {
            ++failCount_;
            if (verbose_) {
                std::cerr << "Failed: " << result.fileName 
                          << " - " << result.errorMessage << std::endl;
            }
        }
        
        if (progressCallback_) {
            progressCallback_(processedCount_, total, result.fileName);
        }
    }
    
    return results;
}

std::vector<ProcessingResult> ChartIngest::processDirectory(const std::string& dirPath, bool recursive) {
    auto files = findS57Files(dirPath, recursive);
    
    if (verbose_) {
        std::cout << "Found " << files.size() << " S-57 files" << std::endl;
    }
    
    return processFiles(files);
}

ChartIngest::Statistics ChartIngest::getStatistics() const {
    Statistics stats;
    stats.totalFiles = processedCount_;
    stats.successCount = successCount_;
    stats.failCount = failCount_;
    stats.totalFeatures = totalFeatures_;
    return stats;
}

} // namespace s57
