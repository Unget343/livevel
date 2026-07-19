#pragma once
#include "hand_tracking_types.h"
#include <opencv2/core.hpp>
#include <string>
#include <vector>

namespace livevel {

// Draws hand tracking results (landmarks, skeleton, gesture labels)
// as an overlay on a camera frame.
class HandOverlayRenderer {
public:
    HandOverlayRenderer() = default;

    // Draw all hand tracking data onto the frame.
    void draw(cv::Mat& frame, const HandTrackingResult& result,
              const std::vector<GestureEvent>& gestures,
              const std::string& backendName, double fps = -1.0) const;

private:
    // Draw landmarks (colored dots) for a single hand.
    void drawLandmarks(cv::Mat& frame, const HandData& hand) const;

    // Draw skeleton connections between landmarks.
    void drawSkeleton(cv::Mat& frame, const HandData& hand) const;

    // Draw gesture label near the hand.
    void drawGestureLabel(cv::Mat& frame, const HandData& hand,
                          const GestureEvent& gesture) const;

    // Draw info panel (FPS, backend name, hand count).
    void drawInfoPanel(cv::Mat& frame, const std::string& backendName,
                       int handCount, double fps) const;

    // Get color for a landmark based on its finger group.
    cv::Scalar getLandmarkColor(int landmarkIndex) const;

    // Landmark radius in pixels.
    int landmarkRadius_ = 5;

    // Skeleton line thickness.
    int skeletonThickness_ = 2;
};

} // namespace livevel
