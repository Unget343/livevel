#pragma once
#include <opencv2/core.hpp>
#include <vector>
#include <mutex>
#include "utils.h"

namespace livevel {

struct CacheEntry {
    cv::Mat originalData;
    cv::Mat processedData;
    bool isValid = false;
};

class RegionCache {
public:
    RegionCache(int maxSize, double tolerance);

    // Returns true if found, and fills outputProcessed with cached result
    bool find(const cv::Mat& inputData, cv::Mat& outputProcessed);

    // Adds a new entry, evicting the oldest if full
    void add(const cv::Mat& inputData, const cv::Mat& processedData);

    // Provide some stats
    int getHits() const { return hits_; }
    int getMisses() const { return misses_; }

private:
    std::vector<CacheEntry> buffer_;
    int maxSize_;
    int headIndex_;
    double tolerance_;

    int hits_ = 0;
    int misses_ = 0;
    std::mutex mutex_;
};

} // namespace livevel
