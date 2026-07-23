#include <chrono>
#include <iostream>
#include <thread>

#ifndef LIVEVEL_HAVE_OPENCV
#define LIVEVEL_HAVE_OPENCV 0
#endif

#if LIVEVEL_HAVE_OPENCV
#include "camera.h"
#include "frame_processor.h"
#include "hand_tracker.h"
#include "renderer.h"
#include "utils.h"


#include <opencv2/highgui.hpp>
#endif

int main() {
#if LIVEVEL_HAVE_OPENCV
    std::cout << "Starting livevel optimized application...\n";

    livevel::Config config;
    config.regionWidth = 15;
    config.regionHeight = 0; // 0 means full height (vertical strips)
    config.cacheSize = 100;
    config.cacheTolerance = 5.0; // Small tolerance for camera noise

    livevel::Camera camera(0);
    if (!camera.open()) {
        std::cerr << "Failed to open camera 0.\n";
        return 5;
    }

    livevel::Renderer renderer("livevel optimized");
    unsigned int threads = std::thread::hardware_concurrency();
    if (threads == 0) {
        threads = 4;
    }

    livevel::FrameProcessor processor(config, threads);

    livevel::HandTracker handTracker;
    if (!handTracker.initialize("models", 1)) {
        std::cerr << "Warning: Failed to initialize HandTracker. Hand tracking will be disabled.\n";
    }

    cv::Mat frame;
    int frameCount = 0;
    int64_t lastTimestamp = 0;
    double currentFps = 30.0;
    auto tStart = std::chrono::steady_clock::now();

    while (camera.isOpened()) {
        if (!camera.read(frame)) {
            std::cerr << "Failed to read frame.\n";
            break;
        }

        cv::Mat processedFrame = processor.process(frame);
        frameCount++;
        if (frameCount % 30 == 0) {
            auto tEnd = std::chrono::steady_clock::now();
            currentFps = 30.0 / std::chrono::duration<double>(tEnd - tStart).count();
            tStart = tEnd;

            const auto& cache = processor.getCache();
            int hits = cache.getHits();
            int misses = cache.getMisses();
            double hitRate = (hits + misses) > 0
                ? static_cast<double>(hits) / (hits + misses) * 100.0
                : 0.0;

            std::cout << "FPS: " << currentFps << " | Cache Hit Rate: " << hitRate << "% ("
                      << hits << " hits, " << misses << " misses)\n";
        }

        // Ensure monotonically increasing timestamp for MediaPipe Live Stream
        auto now = std::chrono::steady_clock::now();
        int64_t timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        if (timestampMs <= lastTimestamp) {
            timestampMs = lastTimestamp + 1;
        }
        lastTimestamp = timestampMs;

        auto result = handTracker.processFrame(frame, timestampMs);
        handTracker.drawOverlay(processedFrame, result, currentFps);

        renderer.render(processedFrame);

        if (cv::waitKey(1) == 27) {
            break;
        }
    }

    return 0;
#else
    std::cerr << "livevel was built without OpenCV. Install OpenCV and "
                 "reconfigure CMake with OpenCV_DIR or CMAKE_PREFIX_PATH.\n";
    return 2;
#endif
}
