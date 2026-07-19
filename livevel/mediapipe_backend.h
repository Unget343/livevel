#pragma once
#include "hand_tracker_backend.h"
#include "gesture_classifier.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <string>

namespace livevel {

// MediaPipe backend that loads the MediaPipe Tasks C API DLL at runtime.
// Uses MpHandLandmarkerDetectForVideo for per-frame detection,
// and the shared GestureClassifier for gesture recognition.
//
// Requires:
//   - mediapipe_tasks.dll in PATH or alongside the executable
//   - hand_landmarker.task model file
class MediaPipeBackend : public IHandTrackerBackend {
public:
    MediaPipeBackend();
    ~MediaPipeBackend() override;

    bool initialize(const std::string& modelPath, int maxHands) override;
    HandTrackingResult detect(const cv::Mat& frame, int64_t timestampMs) override;
    std::vector<GestureEvent> recognizeGestures(const HandTrackingResult& result) override;
    std::string backendName() const override { return "MediaPipe"; }

private:
    bool loadDll();
    void unloadDll();

#ifdef _WIN32
    HMODULE dllHandle_ = nullptr;
#else
    void* dllHandle_ = nullptr;
#endif

    // Opaque pointer from MediaPipe C API
    void* landmarker_ = nullptr;

    GestureClassifier gestureClassifier_;
    bool initialized_ = false;

    // ─── Function pointers loaded from DLL ──────────────────────────────

    // Image creation
    using FnMpImageCreateFromUint8Data = int (*)(
        int format, int width, int height, const uint8_t* pixel_data,
        int pixel_data_size, void** out, char** error_msg);
    FnMpImageCreateFromUint8Data fnImageCreate_ = nullptr;

    // Image free
    using FnMpImageFree = void (*)(void* image);
    FnMpImageFree fnImageFree_ = nullptr;

    // Hand landmarker create
    using FnMpHandLandmarkerCreate = int (*)(
        void* options, void** landmarker, char** error_msg);
    FnMpHandLandmarkerCreate fnCreate_ = nullptr;

    // Hand landmarker detect for video
    using FnMpHandLandmarkerDetectForVideo = int (*)(
        void* landmarker, void* image, const void* options,
        int64_t timestamp_ms, void* result, char** error_msg);
    FnMpHandLandmarkerDetectForVideo fnDetectForVideo_ = nullptr;

    // Hand landmarker close result
    using FnMpHandLandmarkerCloseResult = void (*)(void* result);
    FnMpHandLandmarkerCloseResult fnCloseResult_ = nullptr;

    // Hand landmarker close
    using FnMpHandLandmarkerClose = int (*)(void* landmarker, char** error_msg);
    FnMpHandLandmarkerClose fnClose_ = nullptr;

    // Error free
    using FnMpErrorFree = void (*)(char* error_message);
    FnMpErrorFree fnErrorFree_ = nullptr;
};

} // namespace livevel
