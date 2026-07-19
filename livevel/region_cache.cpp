#include "region_cache.h"
#include <opencv2/core.hpp>

namespace livevel {

RegionCache::RegionCache(int maxSize, double tolerance)
    : maxSize_(maxSize), tolerance_(tolerance), headIndex_(0) {
    buffer_.resize(maxSize_);
}

bool RegionCache::find(const cv::Mat& inputData, cv::Mat& outputProcessed) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (int i = 0; i < maxSize_; ++i) {
        if (!buffer_[i].isValid) continue;

        // Ensure sizes and types match before norm
        if (buffer_[i].originalData.size() == inputData.size() && 
            buffer_[i].originalData.type() == inputData.type()) {
            
            double diff = cv::norm(buffer_[i].originalData, inputData, cv::NORM_L2);
            if (diff <= tolerance_) {
                // Cache hit
                buffer_[i].processedData.copyTo(outputProcessed);
                hits_++;
                return true;
            }
        }
    }
    
    misses_++;
    return false;
}

void RegionCache::add(const cv::Mat& inputData, const cv::Mat& processedData) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    buffer_[headIndex_].originalData = inputData.clone();
    buffer_[headIndex_].processedData = processedData.clone();
    buffer_[headIndex_].isValid = true;
    
    headIndex_ = (headIndex_ + 1) % maxSize_;
}

} // namespace livevel
