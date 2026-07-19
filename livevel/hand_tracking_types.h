#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace livevel {

// ─── Landmark indices (MediaPipe Hand model, 21 points) ─────────────────────
namespace HandLandmarkIndex {
    inline constexpr int WRIST          = 0;
    inline constexpr int THUMB_CMC      = 1;
    inline constexpr int THUMB_MCP      = 2;
    inline constexpr int THUMB_IP       = 3;
    inline constexpr int THUMB_TIP      = 4;
    inline constexpr int INDEX_MCP      = 5;
    inline constexpr int INDEX_PIP      = 6;
    inline constexpr int INDEX_DIP      = 7;
    inline constexpr int INDEX_TIP      = 8;
    inline constexpr int MIDDLE_MCP     = 9;
    inline constexpr int MIDDLE_PIP     = 10;
    inline constexpr int MIDDLE_DIP     = 11;
    inline constexpr int MIDDLE_TIP     = 12;
    inline constexpr int RING_MCP       = 13;
    inline constexpr int RING_PIP       = 14;
    inline constexpr int RING_DIP       = 15;
    inline constexpr int RING_TIP       = 16;
    inline constexpr int PINKY_MCP      = 17;
    inline constexpr int PINKY_PIP      = 18;
    inline constexpr int PINKY_DIP      = 19;
    inline constexpr int PINKY_TIP      = 20;
    inline constexpr int COUNT          = 21;
} // namespace HandLandmarkIndex

// Connections between landmarks for skeleton drawing (pair of indices).
inline constexpr std::array<std::array<int, 2>, 21> kHandConnections = {{
    // Palm
    {0, 1}, {0, 5}, {5, 9}, {9, 13}, {13, 17}, {0, 17},
    // Thumb
    {1, 2}, {2, 3}, {3, 4},
    // Index finger
    {5, 6}, {6, 7}, {7, 8},
    // Middle finger
    {9, 10}, {10, 11}, {11, 12},
    // Ring finger
    {13, 14}, {14, 15}, {15, 16},
    // Pinky finger
    {17, 18}, {18, 19}, {19, 20}
}};

// ─── Enums ──────────────────────────────────────────────────────────────────

enum class HandGesture {
    None,
    Fist,
    OpenPalm,
    PointingUp,
    ThumbUp,
    ThumbDown,
    Victory,
    ILoveYou
};

enum class Handedness {
    Unknown,
    Left,
    Right
};

// ─── Structs ────────────────────────────────────────────────────────────────

// A single 3D landmark in normalized image coordinates.
// x, y are in [0, 1] relative to image width/height.
// z represents relative depth (smaller = closer to camera).
// visibility indicates how likely the point is visible (not occluded), range [0, 1].
struct HandLandmark {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float visibility = 1.0f;
};

// ─── Zone-based Analysis Types ──────────────────────────────────────────────

enum class HandZoneId {
    PalmBase = 0,
    PalmTop,
    ThumbBase,
    ThumbTip,
    IndexBase,
    IndexTip,
    MiddleBase,
    MiddleTip,
    RingBase,
    RingTip,
    PinkyBase,
    PinkyTip,
    Count
};

// State of a specific independent zone of the hand.
struct ZoneState {
    bool isVisible = false;      // True if the zone is not occluded
    bool isExtended = false;     // True if this part of the finger is extended
    float confidence = 0.0f;     // Confidence score for this state [0, 1]
};


// Complete data for a single detected hand.
struct HandData {
    std::array<HandLandmark, HandLandmarkIndex::COUNT> landmarks{};
    Handedness handedness = Handedness::Unknown;
    float confidence = 0.0f;
};

// Result of hand tracking for a single frame.
struct HandTrackingResult {
    std::vector<HandData> hands;
    int64_t timestampMs = 0;
};

// A recognized gesture event.
struct GestureEvent {
    HandGesture gesture = HandGesture::None;
    Handedness hand = Handedness::Unknown;
    float confidence = 0.0f;
    int handIndex = -1; // Index into HandTrackingResult::hands
};

// ─── Utility ────────────────────────────────────────────────────────────────

inline const char* gestureToString(HandGesture g) {
    switch (g) {
        case HandGesture::None:       return "None";
        case HandGesture::Fist:       return "Fist";
        case HandGesture::OpenPalm:   return "Open Palm";
        case HandGesture::PointingUp: return "Pointing Up";
        case HandGesture::ThumbUp:    return "Thumb Up";
        case HandGesture::ThumbDown:  return "Thumb Down";
        case HandGesture::Victory:    return "Victory";
        case HandGesture::ILoveYou:   return "I Love You";
        default:                      return "Unknown";
    }
}

inline const char* handednessToString(Handedness h) {
    switch (h) {
        case Handedness::Left:    return "Left";
        case Handedness::Right:   return "Right";
        case Handedness::Unknown: return "Unknown";
        default:                  return "Unknown";
    }
}

} // namespace livevel
