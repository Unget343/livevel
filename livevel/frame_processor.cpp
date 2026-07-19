#include "frame_processor.h"
#include <opencv2/imgproc.hpp>
#include <vector>

namespace livevel {

FrameProcessor::FrameProcessor(const Config& config, size_t numThreads)
    : config_(config), numThreads_(numThreads) {
    threadManager_ = std::make_unique<ThreadManager>(numThreads);
    cache_ = std::make_unique<RegionCache>(config_.cacheSize, config_.cacheTolerance);
}

cv::Mat FrameProcessor::process(const cv::Mat& inputFrame) {
    if (inputFrame.empty()) return inputFrame;

    cv::Mat outputFrame = inputFrame.clone();
    int width = inputFrame.cols;
    int height = inputFrame.rows;
    
    int rw = config_.regionWidth;
    int rh = config_.regionHeight <= 0 ? height : config_.regionHeight;

    // Collect all rectangles
    std::vector<cv::Rect> rects;
    for (int y = 0; y < height; y += rh) {
        for (int x = 0; x < width; x += rw) {
            int w = std::min(rw, width - x);
            int h = std::min(rh, height - y);
            rects.emplace_back(x, y, w, h);
        }
    }

    size_t totalRects = rects.size();
    if (totalRects == 0) return outputFrame;

    size_t numTasks = std::min(numThreads_, totalRects);
    size_t rectsPerTask = totalRects / numTasks;
    size_t remainder = totalRects % numTasks;

    size_t startIdx = 0;
    for (size_t i = 0; i < numTasks; ++i) {
        size_t count = rectsPerTask + (i < remainder ? 1 : 0);
        if (count == 0) break;
        
        size_t endIdx = startIdx + count;

        threadManager_->enqueue([this, &inputFrame, &outputFrame, rects, startIdx, endIdx]() {
            for (size_t r = startIdx; r < endIdx; ++r) {
                const cv::Rect& rect = rects[r];
                cv::Mat regionData = inputFrame(rect);
                cv::Mat processedRegion;
                
                if (!cache_->find(regionData, processedRegion)) {
                    processedRegion = this->processRegionData(regionData);
                    cache_->add(regionData, processedRegion);
                }
                
                // Write back to output
                // Since blocks are non-overlapping, it is thread-safe to write to outputFrame
                processedRegion.copyTo(outputFrame(rect));
            }
        });
        
        startIdx = endIdx;
    }

    // Wait for all regions of the frame to finish processing
    threadManager_->waitAll();

    return outputFrame;
}

cv::Mat FrameProcessor::processRegionData(const cv::Mat& input) {
    cv::Mat output;
    // For now, simply copy the input to avoid color inversion.
    // In practice, this could be edge detection, blur, deep learning inference, etc.
    input.copyTo(output);
    return output;
}

} // namespace livevel
