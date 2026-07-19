#pragma once
#include <opencv2/core.hpp>
#include "utils.h"
#include "thread_manager.h"
#include "region_cache.h"
#include <memory>

namespace livevel {

class FrameProcessor {
public:
    FrameProcessor(const Config& config, size_t numThreads = 4);

    // Processes the frame by splitting it, using cache, and returning the result
    cv::Mat process(const cv::Mat& inputFrame);

    const RegionCache& getCache() const { return *cache_; }

private:
    // A dummy processing function (e.g. invert colors or blur)
    cv::Mat processRegionData(const cv::Mat& input);

    Config config_;
    size_t numThreads_;
    std::unique_ptr<ThreadManager> threadManager_;
    std::unique_ptr<RegionCache> cache_;
};

} // namespace livevel
