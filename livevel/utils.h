#pragma once
#include <opencv2/core.hpp>
#include <string>

namespace livevel {

struct Config {
    int regionWidth = 15;
    int regionHeight = 15;
    int cacheSize = 100;
    double cacheTolerance = 10.0; // Tolerance for cache matching

    // Hand tracking config
    bool enableHandTracking = true;
    std::string modelDir = "models";
    int maxHands = 2;
};

struct Region {
    cv::Rect rect;
};

} // namespace livevel
