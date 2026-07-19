#pragma once
#include "hand_tracking_types.h"
#include "hand_tracker_backend.h"
#include "hand_overlay_renderer.h"
#include <opencv2/core.hpp>
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace livevel {

// Main facade for the hand tracking system.
// Manages backends, active tracking state, and rendering.
class HandTracker {
public:
    HandTracker();
    ~HandTracker();

    // Initialize the tracking system.
    // Tries to load MediaPipe, falls back to OpenCV skin detection.
    bool initialize(const std::string& modelDir = "models", int maxHands = 2);

    // Process a frame: detect hands and recognize gestures.
    // Returns the tracking result.
    HandTrackingResult processFrame(const cv::Mat& frame, int64_t timestampMs);

    // Get the latest recognized gestures.
    std::vector<GestureEvent> getGestures() const;

    // Draw landmarks, skeletons, and gestures onto the frame.
    void drawOverlay(cv::Mat& frame, const HandTrackingResult& result, double fps = -1.0) const;

    // Return the name of the active backend.
    std::string activeBackend() const;

    // Register a callback to be notified of gesture events.
    using GestureCallback = std::function<void(const GestureEvent&)>;
    void onGesture(GestureCallback callback);

private:
    std::unique_ptr<IHandTrackerBackend> backend_;
    HandOverlayRenderer renderer_;

    std::vector<GestureEvent> latestGestures_;
    std::vector<GestureCallback> gestureCallbacks_;
};

} // namespace livevel
