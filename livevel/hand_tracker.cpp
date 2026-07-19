#include "hand_tracker.h"
#include "mediapipe_backend.h"
#include "opencv_gesture_backend.h"
#include <iostream>

namespace livevel {

HandTracker::HandTracker() = default;
HandTracker::~HandTracker() = default;

bool HandTracker::initialize(const std::string& modelDir, int maxHands) {
    // Attempt MediaPipe first
    backend_ = std::make_unique<MediaPipeBackend>();
    if (backend_->initialize(modelDir, maxHands)) {
        return true;
    }

    // Fallback to OpenCV if MediaPipe fails or is unavailable
    std::cout << "[HandTracker] Falling back to OpenCV backend.\n";
    backend_ = std::make_unique<OpenCVGestureBackend>();
    return backend_->initialize(modelDir, maxHands);
}

HandTrackingResult HandTracker::processFrame(const cv::Mat& frame, int64_t timestampMs) {
    if (!backend_ || frame.empty()) {
        return HandTrackingResult{ {}, timestampMs };
    }

    HandTrackingResult result = backend_->detect(frame, timestampMs);
    latestGestures_ = backend_->recognizeGestures(result);

    // Notify callbacks
    if (!gestureCallbacks_.empty() && !latestGestures_.empty()) {
        for (const auto& gesture : latestGestures_) {
            for (const auto& cb : gestureCallbacks_) {
                cb(gesture);
            }
        }
    }

    return result;
}

std::vector<GestureEvent> HandTracker::getGestures() const {
    return latestGestures_;
}

void HandTracker::drawOverlay(cv::Mat& frame, const HandTrackingResult& result, double fps) const {
    if (!backend_ || frame.empty()) return;
    renderer_.draw(frame, result, latestGestures_, backend_->backendName(), fps);
}

std::string HandTracker::activeBackend() const {
    return backend_ ? backend_->backendName() : "None";
}

void HandTracker::onGesture(GestureCallback callback) {
    gestureCallbacks_.push_back(std::move(callback));
}

} // namespace livevel
