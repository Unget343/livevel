#include "mediapipe_backend.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <cstring>

namespace livevel {

MediaPipeBackend::MediaPipeBackend() = default;

MediaPipeBackend::~MediaPipeBackend() {
    if (landmarker_) {
        if (fnClose_) {
            char* errMsg = nullptr;
            fnClose_(landmarker_, &errMsg);
            if (errMsg && fnErrorFree_) {
                fnErrorFree_(errMsg);
            }
        }
        landmarker_ = nullptr;
    }
    unloadDll();
}

bool MediaPipeBackend::loadDll() {
#ifdef _WIN32
    // Try several DLL names used by MediaPipe releases
    const char* dllNames[] = {
        "mediapipe_tasks.dll",
        "mediapipe_tasks_vision.dll",
        "libmediapipe_tasks.dll",
        nullptr
    };

    for (int i = 0; dllNames[i] != nullptr; ++i) {
        dllHandle_ = LoadLibraryA(dllNames[i]);
        if (dllHandle_) {
            std::cout << "[MediaPipe] Loaded DLL: " << dllNames[i] << "\n";
            break;
        }
    }

    if (!dllHandle_) {
        std::cerr << "[MediaPipe] Could not load MediaPipe Tasks DLL.\n";
        return false;
    }

    // Load function pointers
    fnImageCreate_ = reinterpret_cast<FnMpImageCreateFromUint8Data>(
        GetProcAddress(dllHandle_, "MpImageCreateFromUint8Data"));
    fnImageFree_ = reinterpret_cast<FnMpImageFree>(
        GetProcAddress(dllHandle_, "MpImageFree"));
    fnCreate_ = reinterpret_cast<FnMpHandLandmarkerCreate>(
        GetProcAddress(dllHandle_, "MpHandLandmarkerCreate"));
    fnDetectForVideo_ = reinterpret_cast<FnMpHandLandmarkerDetectForVideo>(
        GetProcAddress(dllHandle_, "MpHandLandmarkerDetectForVideo"));
    fnCloseResult_ = reinterpret_cast<FnMpHandLandmarkerCloseResult>(
        GetProcAddress(dllHandle_, "MpHandLandmarkerCloseResult"));
    fnClose_ = reinterpret_cast<FnMpHandLandmarkerClose>(
        GetProcAddress(dllHandle_, "MpHandLandmarkerClose"));
    fnErrorFree_ = reinterpret_cast<FnMpErrorFree>(
        GetProcAddress(dllHandle_, "MpErrorFree"));

    bool allLoaded = fnImageCreate_ && fnImageFree_ && fnCreate_
                     && fnDetectForVideo_ && fnCloseResult_ && fnClose_;

    if (!allLoaded) {
        std::cerr << "[MediaPipe] Failed to load required functions from DLL.\n";
        unloadDll();
        return false;
    }

    return true;
#else
    std::cerr << "[MediaPipe] DLL loading is only supported on Windows.\n";
    return false;
#endif
}

void MediaPipeBackend::unloadDll() {
#ifdef _WIN32
    if (dllHandle_) {
        FreeLibrary(dllHandle_);
        dllHandle_ = nullptr;
    }
#endif
    fnImageCreate_ = nullptr;
    fnImageFree_ = nullptr;
    fnCreate_ = nullptr;
    fnDetectForVideo_ = nullptr;
    fnCloseResult_ = nullptr;
    fnClose_ = nullptr;
    fnErrorFree_ = nullptr;
}

bool MediaPipeBackend::initialize(const std::string& modelPath, int maxHands) {
    if (!loadDll()) {
        return false;
    }

    // Build the full model file path.
    std::string modelFile = modelPath + "/hand_landmarker.task";

    // MediaPipe C API uses MpHandLandmarkerOptions which is a struct with
    // specific layout. We construct it manually matching the C struct layout
    // from the header. Since we're calling through function pointers into
    // a DLL compiled separately, we need to match the ABI.
    //
    // For simplicity and safety with opaque DLL interfaces, we use
    // the raw C struct layout. The struct MpHandLandmarkerOptions:
    //   MpBaseOptions base_options;  (contains model_asset_path, etc.)
    //   MpRunningMode running_mode;
    //   int num_hands;
    //   float min_hand_detection_confidence;
    //   float min_hand_presence_confidence;
    //   float min_tracking_confidence;
    //   result_callback_fn result_callback;

    // NOTE: Because we're loading the DLL at runtime and the struct layout
    // must match exactly, we include the MediaPipe C headers for type safety.
    // However, since those headers reference each other and require specific
    // build configurations, we use a simplified approach:
    // We'll attempt to call the C API, and if anything fails, we fall back
    // gracefully.

    // For now, mark as not initialized since we need the exact DLL ABI
    // to properly construct the options struct. The GestureClassifier
    // will still work with the OpenCV backend's landmarks.
    std::cout << "[MediaPipe] DLL loaded successfully. "
              << "Model path: " << modelFile << "\n"
              << "[MediaPipe] Note: Full DLL integration requires matching ABI. "
              << "Gesture classification will use the rule-based classifier.\n";

    // In a production scenario, this is where we'd call:
    //   MpHandLandmarkerOptions opts = {};
    //   opts.base_options.model_asset_path = modelFile.c_str();
    //   opts.running_mode = MP_RUNNING_MODE_VIDEO;
    //   opts.num_hands = maxHands;
    //   fnCreate_(&opts, &landmarker_, &errMsg);

    initialized_ = false; // Will fall back to OpenCV
    return false;
}

HandTrackingResult MediaPipeBackend::detect(const cv::Mat& /*frame*/,
                                             int64_t timestampMs) {
    HandTrackingResult result;
    result.timestampMs = timestampMs;

    if (!initialized_ || !landmarker_) {
        return result;
    }

    // In production, this would:
    // 1. Convert cv::Mat BGR → RGB
    // 2. Create MpImagePtr via fnImageCreate_
    // 3. Call fnDetectForVideo_ with the image
    // 4. Convert MpHandLandmarkerResult → HandTrackingResult
    // 5. Free MpImage and result

    return result;
}

std::vector<GestureEvent> MediaPipeBackend::recognizeGestures(
    const HandTrackingResult& result) {
    return gestureClassifier_.classify(result);
}

} // namespace livevel
