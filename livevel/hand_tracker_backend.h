#pragma once
#include "hand_tracking_types.h"
#include <opencv2/core.hpp>
#include <string>
#include <vector>

namespace livevel {

// Abstract interface for hand tracking backends.
// Implementations provide hand detection + gesture recognition.
class IHandTrackerBackend {
public:
    virtual ~IHandTrackerBackend() = default;

    // Initialize the backend with model path and max number of hands.
    // Returns true on success.
    virtual bool initialize(const std::string& modelPath, int maxHands) = 0;

    // Detect hands in a frame. timestampMs must be monotonically increasing.
    virtual HandTrackingResult detect(const cv::Mat& frame, int64_t timestampMs) = 0;

    // Classify gestures from a tracking result.
    virtual std::vector<GestureEvent> recognizeGestures(const HandTrackingResult& result) = 0;

    // Human-readable name of this backend.
    virtual std::string backendName() const = 0;
};

} // namespace livevel
