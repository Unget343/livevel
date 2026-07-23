#include "mediapipe_backend.h"
#include "mediapipe/tasks/c/vision/hand_landmarker/hand_landmarker.h"
#include "mediapipe/tasks/c/vision/core/image.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace livevel {

static MediaPipeBackend* g_backend_instance = nullptr;

static void GlobalResultCallback(MpStatus status, const MpHandLandmarkerResult* result, MpImagePtr image, int64_t timestamp_ms) {
    if (g_backend_instance) {
        g_backend_instance->processAsyncResult(status, result, image, timestamp_ms);
    }
}

MediaPipeBackend::MediaPipeBackend() {
    g_backend_instance = this;
}

MediaPipeBackend::~MediaPipeBackend() {
    if (g_backend_instance == this) {
        g_backend_instance = nullptr;
    }
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
    fnDetectAsync_ = reinterpret_cast<FnMpHandLandmarkerDetectAsync>(
        GetProcAddress(dllHandle_, "MpHandLandmarkerDetectAsync"));
    fnCloseResult_ = reinterpret_cast<FnMpHandLandmarkerCloseResult>(
        GetProcAddress(dllHandle_, "MpHandLandmarkerCloseResult"));
    fnClose_ = reinterpret_cast<FnMpHandLandmarkerClose>(
        GetProcAddress(dllHandle_, "MpHandLandmarkerClose"));
    fnErrorFree_ = reinterpret_cast<FnMpErrorFree>(
        GetProcAddress(dllHandle_, "MpErrorFree"));

    bool allLoaded = fnImageCreate_ && fnImageFree_ && fnCreate_
                     && fnDetectForVideo_ && fnDetectAsync_ && fnCloseResult_ && fnClose_;

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
    fnDetectAsync_ = nullptr;
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

    MpHandLandmarkerOptions opts = {};
    opts.base_options.model_asset_path = modelFile.c_str();
    opts.running_mode = MP_RUNNING_MODE_LIVE_STREAM;
    opts.num_hands = maxHands;
    opts.result_callback = GlobalResultCallback;

    char* errMsg = nullptr;
    int status = fnCreate_(&opts, &landmarker_, &errMsg);

    if (status != 0 || !landmarker_) {
        std::cerr << "[MediaPipe] Failed to create landmarker";
        if (errMsg) {
            std::cerr << ": " << errMsg;
            fnErrorFree_(errMsg);
        }
        std::cerr << "\n";
        return false;
    }

    initialized_ = true;
    std::cout << "[MediaPipe] DLL loaded successfully and initialized in Live Stream mode.\n"
              << "Model path: " << modelFile << "\n";
    return true;
}

HandTrackingResult MediaPipeBackend::detect(const cv::Mat& frame,
                                             int64_t timestampMs) {
    if (!initialized_ || !landmarker_ || frame.empty()) {
        std::lock_guard<std::mutex> lock(resultMutex_);
        return latestResult_;
    }

    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

    void* mpImage = nullptr;
    char* errMsg = nullptr;
    
    // kMpImageFormatSrgb is 1
    int status = fnImageCreate_(1, rgbFrame.cols, rgbFrame.rows, rgbFrame.data, rgbFrame.step[0] * rgbFrame.rows, &mpImage, &errMsg);
    if (status != 0 || !mpImage) {
        if (errMsg) fnErrorFree_(errMsg);
        std::lock_guard<std::mutex> lock(resultMutex_);
        return latestResult_;
    }

    status = fnDetectAsync_(landmarker_, mpImage, nullptr, timestampMs, &errMsg);
    if (status != 0) {
        if (errMsg) fnErrorFree_(errMsg);
    }

    fnImageFree_(mpImage);

    std::lock_guard<std::mutex> lock(resultMutex_);
    return latestResult_;
}

void MediaPipeBackend::processAsyncResult(int status, const void* result_ptr, void* /*image*/, int64_t timestamp_ms) {
    if (status != 0 || !result_ptr) {
        return;
    }

    const auto* result = static_cast<const MpHandLandmarkerResult*>(result_ptr);

    HandTrackingResult currentResult;
    currentResult.timestampMs = timestamp_ms;

    for (uint32_t i = 0; i < result->hand_landmarks_count; ++i) {
        HandData handData;
        const auto& landmarks = result->hand_landmarks[i];

        int count = std::min(static_cast<int>(landmarks.landmarks_count), static_cast<int>(HandLandmarkIndex::COUNT));
        for (int j = 0; j < count; ++j) {
            handData.landmarks[j].x = landmarks.landmarks[j].x;
            handData.landmarks[j].y = landmarks.landmarks[j].y;
            handData.landmarks[j].z = landmarks.landmarks[j].z;
            handData.landmarks[j].visibility = landmarks.landmarks[j].has_visibility ? landmarks.landmarks[j].visibility : 1.0f;
        }

        if (result->handedness_count > i) {
            const auto& handedness = result->handedness[i];
            if (handedness.categories_count > 0) {
                const auto& cat = handedness.categories[0];
                handData.confidence = cat.score;
                if (cat.category_name) {
                    std::string name(cat.category_name);
                    if (name == "Left") handData.handedness = Handedness::Left;
                    else if (name == "Right") handData.handedness = Handedness::Right;
                } else if (cat.display_name) {
                    std::string name(cat.display_name);
                    if (name == "Left") handData.handedness = Handedness::Left;
                    else if (name == "Right") handData.handedness = Handedness::Right;
                }
            }
        }

        currentResult.hands.push_back(handData);
    }

    {
        std::lock_guard<std::mutex> lock(resultMutex_);
        latestResult_ = currentResult;
    }
}

std::vector<GestureEvent> MediaPipeBackend::recognizeGestures(
    const HandTrackingResult& result) {
    return gestureClassifier_.classify(result);
}

} // namespace livevel
