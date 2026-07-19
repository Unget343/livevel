#include "hand_overlay_renderer.h"
#include <opencv2/imgproc.hpp>
#include <cstdio>

namespace livevel {

void HandOverlayRenderer::draw(cv::Mat& frame, const HandTrackingResult& result,
                               const std::vector<GestureEvent>& gestures,
                               const std::string& backendName, double fps) const {
    if (frame.empty()) return;

    // Draw hands
    for (const auto& hand : result.hands) {
        drawSkeleton(frame, hand);
        drawLandmarks(frame, hand);
    }

    // Draw gestures
    for (const auto& gesture : gestures) {
        if (gesture.handIndex >= 0 && gesture.handIndex < static_cast<int>(result.hands.size())) {
            drawGestureLabel(frame, result.hands[gesture.handIndex], gesture);
        }
    }

    // Draw info panel
    drawInfoPanel(frame, backendName, static_cast<int>(result.hands.size()), fps);
}

void HandOverlayRenderer::drawLandmarks(cv::Mat& frame, const HandData& hand) const {
    int fw = frame.cols;
    int fh = frame.rows;

    for (int i = 0; i < HandLandmarkIndex::COUNT; ++i) {
        const auto& lm = hand.landmarks[i];
        cv::Point pt(static_cast<int>(lm.x * fw), static_cast<int>(lm.y * fh));
        
        cv::Scalar color = getLandmarkColor(i);
        cv::circle(frame, pt, landmarkRadius_, color, -1, cv::LINE_AA);
        cv::circle(frame, pt, landmarkRadius_ + 1, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }
}

void HandOverlayRenderer::drawSkeleton(cv::Mat& frame, const HandData& hand) const {
    int fw = frame.cols;
    int fh = frame.rows;

    for (const auto& connection : kHandConnections) {
        const auto& lm1 = hand.landmarks[connection[0]];
        const auto& lm2 = hand.landmarks[connection[1]];

        cv::Point pt1(static_cast<int>(lm1.x * fw), static_cast<int>(lm1.y * fh));
        cv::Point pt2(static_cast<int>(lm2.x * fw), static_cast<int>(lm2.y * fh));

        cv::Scalar color = cv::Scalar(200, 200, 200); // Default skeleton color
        cv::line(frame, pt1, pt2, color, skeletonThickness_, cv::LINE_AA);
    }
}

void HandOverlayRenderer::drawGestureLabel(cv::Mat& frame, const HandData& hand,
                                           const GestureEvent& gesture) const {
    if (gesture.gesture == HandGesture::None) return;

    int fw = frame.cols;
    int fh = frame.rows;
    
    // Find bounding box to place label
    int minX = fw, minY = fh, maxX = 0, maxY = 0;
    for (int i = 0; i < HandLandmarkIndex::COUNT; ++i) {
        int x = static_cast<int>(hand.landmarks[i].x * fw);
        int y = static_cast<int>(hand.landmarks[i].y * fh);
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    std::string label = std::string(gestureToString(gesture.gesture)) + " (" + 
                        handednessToString(gesture.hand) + ")";
    
    // Format confidence
    char confStr[32];
    snprintf(confStr, sizeof(confStr), " %.1f%%", gesture.confidence * 100.0f);
    label += confStr;

    cv::Point textPos(minX, std::max(minY - 10, 20));
    
    // Draw background rect
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &baseline);
    cv::Rect bgRect(textPos.x, textPos.y - textSize.height - 5, textSize.width, textSize.height + 10);
    
    // Ensure rect is within frame
    bgRect.x = std::max(0, bgRect.x);
    bgRect.y = std::max(0, bgRect.y);
    if (bgRect.x + bgRect.width > fw) bgRect.width = fw - bgRect.x;
    if (bgRect.y + bgRect.height > fh) bgRect.height = fh - bgRect.y;

    cv::rectangle(frame, bgRect, cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(frame, label, textPos, cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
}

void HandOverlayRenderer::drawInfoPanel(cv::Mat& frame, const std::string& backendName,
                                        int handCount, double fps) const {
    std::string infoStr = "Backend: " + backendName + " | Hands: " + std::to_string(handCount);
    if (fps >= 0) {
        char fpsStr[32];
        snprintf(fpsStr, sizeof(fpsStr), " | FPS: %.1f", fps);
        infoStr += fpsStr;
    }

    cv::Point textPos(10, 30);
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(infoStr, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseline);
    cv::Rect bgRect(textPos.x - 5, textPos.y - textSize.height - 5, textSize.width + 10, textSize.height + 10);
    
    cv::rectangle(frame, bgRect, cv::Scalar(0, 0, 0, 128), cv::FILLED);
    cv::putText(frame, infoStr, textPos, cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
}

cv::Scalar HandOverlayRenderer::getLandmarkColor(int landmarkIndex) const {
    if (landmarkIndex == HandLandmarkIndex::WRIST) {
        return cv::Scalar(255, 255, 255); // White
    }
    
    // Group colors (BGR format)
    cv::Scalar colors[] = {
        cv::Scalar(0, 0, 255),    // Thumb: Red
        cv::Scalar(0, 165, 255),  // Index: Orange
        cv::Scalar(0, 255, 255),  // Middle: Yellow
        cv::Scalar(0, 255, 0),    // Ring: Green
        cv::Scalar(255, 255, 0)   // Pinky: Cyan
    };

    int fingerIndex = (landmarkIndex - 1) / 4;
    if (fingerIndex >= 0 && fingerIndex < 5) {
        return colors[fingerIndex];
    }
    
    return cv::Scalar(255, 255, 255); // Default
}

} // namespace livevel
