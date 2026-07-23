#pragma once

#include <cstdint>
#include <type_traits>

// Minimal MediaPipe Tasks C ABI used by the runtime-loaded backend. These
// declarations match libmediapipe.dll; keeping them local avoids a build-time
// dependency on the MediaPipe source tree.
namespace livevel::mediapipe {

struct BaseOptions {
    const char* modelAssetBuffer;
    unsigned int modelAssetBufferCount;
    const char* modelAssetPath;
    int delegate;
    int hostEnvironment;
    int hostSystem;
    const char* hostVersion;
    const char* caBundlePath;
};

struct Category {
    int index;
    float score;
    char* categoryName;
    char* displayName;
};

struct Categories {
    Category* categories;
    std::uint32_t categoriesCount;
};

struct Landmark {
    float x;
    float y;
    float z;
    bool hasVisibility;
    float visibility;
    bool hasPresence;
    float presence;
    char* name;
};

struct NormalizedLandmark {
    float x;
    float y;
    float z;
    bool hasVisibility;
    float visibility;
    bool hasPresence;
    float presence;
    char* name;
};

template <typename T>
struct LandmarkList {
    T* landmarks;
    std::uint32_t landmarksCount;
};

using Landmarks = LandmarkList<Landmark>;
using NormalizedLandmarks = LandmarkList<NormalizedLandmark>;

struct HandLandmarkerResult {
    Categories* handedness;
    std::uint32_t handednessCount;
    NormalizedLandmarks* handLandmarks;
    std::uint32_t handLandmarksCount;
    Landmarks* handWorldLandmarks;
    std::uint32_t handWorldLandmarksCount;
};

using ResultCallback = void (*)(int status, const HandLandmarkerResult* result,
                                void* image, std::int64_t timestampMs);

struct HandLandmarkerOptions {
    BaseOptions baseOptions;
    int runningMode;
    int numHands;
    float minHandDetectionConfidence;
    float minHandPresenceConfidence;
    float minTrackingConfidence;
    ResultCallback resultCallback;
};

static_assert(std::is_standard_layout_v<BaseOptions>);
static_assert(std::is_standard_layout_v<HandLandmarkerOptions>);
static_assert(std::is_standard_layout_v<HandLandmarkerResult>);
static_assert(sizeof(bool) == 1);

inline constexpr int kRunningModeImage = 1;
inline constexpr int kRunningModeVideo = 2;

} // namespace livevel::mediapipe
