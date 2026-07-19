#include "opencv_gesture_backend.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>

namespace livevel {

namespace {
    constexpr double kPi = 3.14159265358979323846;

    // Calculate the angle between three points (in radians).
    double angleBetween(const cv::Point& a, const cv::Point& b, const cv::Point& c) {
        double ba_x = static_cast<double>(a.x - b.x);
        double ba_y = static_cast<double>(a.y - b.y);
        double bc_x = static_cast<double>(c.x - b.x);
        double bc_y = static_cast<double>(c.y - b.y);

        double dot = ba_x * bc_x + ba_y * bc_y;
        double cross = ba_x * bc_y - ba_y * bc_x;

        return std::atan2(std::abs(cross), dot);
    }

    // Calculate Euclidean distance between two points.
    double pointDistance(const cv::Point& a, const cv::Point& b) {
        double dx = static_cast<double>(a.x - b.x);
        double dy = static_cast<double>(a.y - b.y);
        return std::sqrt(dx * dx + dy * dy);
    }

    // Lerp between two points.
    cv::Point2f lerp(const cv::Point2f& a, const cv::Point2f& b, float t) {
        return cv::Point2f(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }
} // anonymous namespace

OpenCVGestureBackend::OpenCVGestureBackend() = default;

bool OpenCVGestureBackend::initialize(const std::string& /*modelPath*/,
                                       int maxHands) {
    maxHands_ = maxHands;
    initialized_ = true;
    std::cout << "[OpenCV Backend] Initialized with skin color detection.\n";
    return true;
}

bool OpenCVGestureBackend::findHandContour(const cv::Mat& frame,
                                            std::vector<cv::Point>& contour,
                                            cv::Rect& boundingRect) const {
    // Convert to YCrCb color space for skin detection.
    cv::Mat ycrcb;
    cv::cvtColor(frame, ycrcb, cv::COLOR_BGR2YCrCb);

    // Threshold for skin color range.
    cv::Mat skinMask;
    cv::inRange(ycrcb, skinLower_, skinUpper_, skinMask);

    // Morphological operations to clean up the mask.
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::erode(skinMask, skinMask, kernel, cv::Point(-1, -1), 2);
    cv::dilate(skinMask, skinMask, kernel, cv::Point(-1, -1), 3);

    // Optional: Gaussian blur to smooth edges.
    cv::GaussianBlur(skinMask, skinMask, cv::Size(5, 5), 0);
    cv::threshold(skinMask, skinMask, 127, 255, cv::THRESH_BINARY);

    // Find contours.
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(skinMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return false;
    }

    // Find the largest contour (most likely the hand).
    double maxArea = 0;
    int maxIdx = -1;
    double frameArea = static_cast<double>(frame.cols) * frame.rows;

    for (int i = 0; i < static_cast<int>(contours.size()); ++i) {
        double area = cv::contourArea(contours[i]);
        if (area > maxArea && area > frameArea * minContourAreaRatio_) {
            maxArea = area;
            maxIdx = i;
        }
    }

    if (maxIdx < 0) {
        return false;
    }

    contour = std::move(contours[maxIdx]);
    boundingRect = cv::boundingRect(contour);
    return true;
}

int OpenCVGestureBackend::countFingers(const std::vector<cv::Point>& contour) const {
    if (contour.size() < 5) {
        return 0;
    }

    // Compute convex hull indices.
    std::vector<int> hullIndices;
    cv::convexHull(contour, hullIndices, false, false);

    if (hullIndices.size() < 3) {
        return 0;
    }

    // Compute convexity defects.
    std::vector<cv::Vec4i> defects;
    try {
        cv::convexityDefects(contour, hullIndices, defects);
    } catch (...) {
        return 0;
    }

    int fingerCount = 0;

    for (const auto& defect : defects) {
        int startIdx = defect[0];
        int endIdx   = defect[1];
        int farIdx   = defect[2];
        float depth  = defect[3] / 256.0f; // Convert fixed-point to float

        const cv::Point& start = contour[startIdx];
        const cv::Point& end   = contour[endIdx];
        const cv::Point& far   = contour[farIdx];

        // Calculate angle at the defect point.
        double angle = angleBetween(start, far, end);

        // A finger gap has an angle less than ~80 degrees and sufficient depth.
        if (angle < (80.0 * kPi / 180.0) && depth > 20.0f) {
            fingerCount++;
        }
    }

    // Defects count gaps between fingers; number of fingers = gaps + 1.
    // But clamp to [0, 5].
    return std::min(fingerCount + 1, 5);
}

HandData OpenCVGestureBackend::generateLandmarks(
    const std::vector<cv::Point>& contour, const cv::Rect& boundingRect,
    int frameWidth, int frameHeight) const {

    HandData hand;
    hand.handedness = Handedness::Unknown;
    hand.confidence = 0.7f; // Moderate confidence for contour-based detection

    if (contour.empty() || frameWidth <= 0 || frameHeight <= 0) {
        return hand;
    }

    float fw = static_cast<float>(frameWidth);
    float fh = static_cast<float>(frameHeight);

    // Calculate center and extremes of the contour for landmark estimation.
    cv::Moments m = cv::moments(contour);
    cv::Point2f center(0.0f, 0.0f);
    if (m.m00 > 0) {
        center.x = static_cast<float>(m.m10 / m.m00);
        center.y = static_cast<float>(m.m01 / m.m00);
    } else {
        center.x = boundingRect.x + boundingRect.width / 2.0f;
        center.y = boundingRect.y + boundingRect.height / 2.0f;
    }

    // Find extreme points on the contour.
    auto topIt = std::min_element(contour.begin(), contour.end(),
        [](const cv::Point& a, const cv::Point& b) { return a.y < b.y; });
    auto bottomIt = std::max_element(contour.begin(), contour.end(),
        [](const cv::Point& a, const cv::Point& b) { return a.y < b.y; });
    auto leftIt = std::min_element(contour.begin(), contour.end(),
        [](const cv::Point& a, const cv::Point& b) { return a.x < b.x; });
    auto rightIt = std::max_element(contour.begin(), contour.end(),
        [](const cv::Point& a, const cv::Point& b) { return a.x < b.x; });

    cv::Point2f top(static_cast<float>(topIt->x), static_cast<float>(topIt->y));
    cv::Point2f bottom(static_cast<float>(bottomIt->x), static_cast<float>(bottomIt->y));
    cv::Point2f left(static_cast<float>(leftIt->x), static_cast<float>(leftIt->y));
    cv::Point2f right(static_cast<float>(rightIt->x), static_cast<float>(rightIt->y));

    // Compute convex hull points for fingertip approximation.
    std::vector<cv::Point> hull;
    cv::convexHull(contour, hull);

    // Sort hull points by y-coordinate (top to bottom).
    std::vector<cv::Point2f> hullF;
    for (const auto& p : hull) {
        hullF.emplace_back(static_cast<float>(p.x), static_cast<float>(p.y));
    }
    std::sort(hullF.begin(), hullF.end(),
              [](const cv::Point2f& a, const cv::Point2f& b) { return a.y < b.y; });

    // ─── Generate pseudo-landmarks ──────────────────────────────────────
    // We approximate the 21-point hand model based on contour geometry.
    // This is inherently approximate; the purpose is to provide data
    // in the same format as MediaPipe so the gesture classifier works.

    // Wrist (0): bottom of the contour
    cv::Point2f wrist = bottom;

    // Use bounding rect for proportional placement.
    float bx = static_cast<float>(boundingRect.x);
    float by = static_cast<float>(boundingRect.y);
    float bw = static_cast<float>(boundingRect.width);
    float bh = static_cast<float>(boundingRect.height);

    // Helper: compute a landmark from relative position in bounding rect.
    auto relPoint = [&](float rx, float ry) -> cv::Point2f {
        return cv::Point2f(bx + bw * rx, by + bh * ry);
    };

    // Set landmark positions approximating the standard hand model.
    // Layout assumes hand is roughly upright (fingers pointing up).

    // Wrist
    hand.landmarks[0] = {wrist.x / fw, wrist.y / fh, 0.0f};

    // Thumb: from wrist extending to the left/right side
    cv::Point2f thumbBase = relPoint(0.15f, 0.75f);
    cv::Point2f thumbTip  = (left.x < center.x) ? left : relPoint(0.0f, 0.45f);

    hand.landmarks[1] = {thumbBase.x / fw, thumbBase.y / fh, 0.0f};                              // THUMB_CMC
    hand.landmarks[2] = {lerp(thumbBase, thumbTip, 0.33f).x / fw,
                          lerp(thumbBase, thumbTip, 0.33f).y / fh, 0.0f};                         // THUMB_MCP
    hand.landmarks[3] = {lerp(thumbBase, thumbTip, 0.66f).x / fw,
                          lerp(thumbBase, thumbTip, 0.66f).y / fh, 0.0f};                         // THUMB_IP
    hand.landmarks[4] = {thumbTip.x / fw, thumbTip.y / fh, 0.0f};                                 // THUMB_TIP

    // Get fingertip candidates from the top part of the hull.
    // Take up to 4 topmost hull points as finger tip candidates.
    std::vector<cv::Point2f> fingerTips;
    for (const auto& hp : hullF) {
        if (hp.y < center.y) { // Above center
            // Check that this tip is far enough from existing tips.
            bool tooClose = false;
            for (const auto& existing : fingerTips) {
                if (cv::norm(hp - existing) < bw * 0.15f) {
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) {
                fingerTips.push_back(hp);
            }
            if (fingerTips.size() >= 4) break;
        }
    }

    // Sort tips left to right (index, middle, ring, pinky order).
    std::sort(fingerTips.begin(), fingerTips.end(),
              [](const cv::Point2f& a, const cv::Point2f& b) { return a.x < b.x; });

    // Pad to 4 tips if we didn't find enough, using interpolated positions.
    while (fingerTips.size() < 4) {
        float xOff = bx + bw * (0.25f + 0.2f * fingerTips.size());
        fingerTips.emplace_back(xOff, by + bh * 0.05f);
    }

    // Generate landmarks for each of the 4 fingers.
    // Each finger has MCP (base), PIP, DIP, TIP.
    int fingerStartIndices[4] = {5, 9, 13, 17}; // Index, Middle, Ring, Pinky

    // Number of detected fingers from defects (affects landmark positioning)
    int detectedFingers = countFingers(contour);

    for (int fi = 0; fi < 4; ++fi) {
        int baseIdx = fingerStartIndices[fi];
        cv::Point2f tipPt = fingerTips[fi];

        // MCP (base of finger): midway between center and a bit above wrist
        float mcpRelY = 0.55f; // roughly at the palm line
        float mcpRelX = 0.2f + 0.2f * fi;
        cv::Point2f mcpPt = relPoint(mcpRelX, mcpRelY);

        // If this finger is "not extended" (finger count suggests it),
        // bring the tip down near the MCP.
        bool probablyExtended = (fi < detectedFingers);
        if (!probablyExtended && detectedFingers < 4) {
            tipPt = lerp(mcpPt, tipPt, 0.3f); // Curl the fingertip down
        }

        // PIP: 1/3 from MCP to tip
        cv::Point2f pipPt = lerp(mcpPt, tipPt, 0.33f);
        // DIP: 2/3 from MCP to tip
        cv::Point2f dipPt = lerp(mcpPt, tipPt, 0.66f);

        hand.landmarks[baseIdx]     = {mcpPt.x / fw, mcpPt.y / fh, 0.0f}; // MCP
        hand.landmarks[baseIdx + 1] = {pipPt.x / fw, pipPt.y / fh, 0.0f}; // PIP
        hand.landmarks[baseIdx + 2] = {dipPt.x / fw, dipPt.y / fh, 0.0f}; // DIP
        hand.landmarks[baseIdx + 3] = {tipPt.x / fw, tipPt.y / fh, 0.0f}; // TIP
    }

    return hand;
}

HandTrackingResult OpenCVGestureBackend::detect(const cv::Mat& frame,
                                                 int64_t timestampMs) {
    HandTrackingResult result;
    result.timestampMs = timestampMs;

    if (!initialized_ || frame.empty()) {
        return result;
    }

    // Currently we detect only one hand (the largest skin region).
    // For multi-hand detection, we'd need to find multiple contours
    // that are separated spatially.
    std::vector<cv::Point> contour;
    cv::Rect bRect;

    if (findHandContour(frame, contour, bRect)) {
        HandData hand = generateLandmarks(contour, bRect, frame.cols, frame.rows);
        result.hands.push_back(std::move(hand));
    }

    return result;
}

std::vector<GestureEvent> OpenCVGestureBackend::recognizeGestures(
    const HandTrackingResult& result) {
    return gestureClassifier_.classify(result);
}

} // namespace livevel
