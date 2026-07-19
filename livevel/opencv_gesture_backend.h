#pragma once
#include "hand_tracker_backend.h"
#include "gesture_classifier.h"
#include <opencv2/core.hpp>

namespace livevel {

// OpenCV-based hand tracking backend using skin color detection
// and contour analysis. Does not require any external DLLs or models.
//
// Detection pipeline:
//   1. BGR → YCrCb color space conversion
//   2. Skin color thresholding (Cr: 133-173, Cb: 77-127)
//   3. Morphological operations (erode + dilate) for noise removal
//   4. Contour detection and largest contour selection
//   5. Convex hull + convexity defects for finger detection
//   6. Pseudo-landmark generation from contour geometry
//   7. Gesture classification via GestureClassifier
class OpenCVGestureBackend : public IHandTrackerBackend {
public:
    OpenCVGestureBackend();
    ~OpenCVGestureBackend() override = default;

    bool initialize(const std::string& modelPath, int maxHands) override;
    HandTrackingResult detect(const cv::Mat& frame, int64_t timestampMs) override;
    std::vector<GestureEvent> recognizeGestures(const HandTrackingResult& result) override;
    std::string backendName() const override { return "OpenCV Skin Detection"; }

private:
    // Extract a hand contour from the frame using skin color detection.
    // Returns the contour points and the bounding rect.
    bool findHandContour(const cv::Mat& frame, std::vector<cv::Point>& contour,
                         cv::Rect& boundingRect) const;

    // Generate pseudo-landmarks from a hand contour.
    // Approximates the 21-point MediaPipe layout using contour geometry.
    HandData generateLandmarks(const std::vector<cv::Point>& contour,
                               const cv::Rect& boundingRect,
                               int frameWidth, int frameHeight) const;

    // Count fingers using convexity defects analysis.
    int countFingers(const std::vector<cv::Point>& contour) const;

    GestureClassifier gestureClassifier_;
    int maxHands_ = 2;
    bool initialized_ = false;

    // Skin color thresholds in YCrCb space
    cv::Scalar skinLower_{0, 133, 77};
    cv::Scalar skinUpper_{255, 173, 127};

    // Minimum contour area relative to frame (filters noise)
    double minContourAreaRatio_ = 0.01;
};

} // namespace livevel
