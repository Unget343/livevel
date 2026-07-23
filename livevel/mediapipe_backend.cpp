#include "mediapipe_backend.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>

namespace livevel {

namespace {

std::filesystem::path findModelFile(const std::string& modelDirectory) {
    const auto requested = std::filesystem::path(modelDirectory) / "hand_landmarker.task";
    if (std::filesystem::is_regular_file(requested)) return requested;

#ifdef _WIN32
    char executablePath[MAX_PATH]{};
    if (GetModuleFileNameA(nullptr, executablePath, MAX_PATH) != 0) {
        const auto besideExecutable =
            std::filesystem::path(executablePath).parent_path() / requested;
        if (std::filesystem::is_regular_file(besideExecutable)) return besideExecutable;
    }
#endif

    return requested;
}

} // namespace

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
    char configuredDll[MAX_PATH]{};
    const DWORD configuredDllLength = GetEnvironmentVariableA(
        "LIVEVEL_MEDIAPIPE_DLL", configuredDll, MAX_PATH);
    const char* dllNames[] = {
        configuredDllLength > 0 && configuredDllLength < MAX_PATH ? configuredDll : nullptr,
        "libmediapipe.dll",
        "mediapipe_tasks.dll",
        "mediapipe_tasks_vision.dll",
        nullptr
    };

    for (int i = 0; dllNames[i] != nullptr; ++i) {
        if (dllNames[i][0] == '\0') continue;
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
                     && fnDetectForVideo_ && fnCloseResult_ && fnClose_ && fnErrorFree_;

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
    const std::filesystem::path modelFile = findModelFile(modelPath);
    if (!std::filesystem::is_regular_file(modelFile)) {
        std::cerr << "[MediaPipe] Missing model: " << modelFile.string() << "\n";
        return false;
    }

    if (!loadDll()) {
        return false;
    }

    const std::string modelFileString = modelFile.string();
    mediapipe::HandLandmarkerOptions opts{};
    opts.baseOptions.modelAssetPath = modelFileString.c_str();
    opts.baseOptions.delegate = 0; // CPU: available on every supported desktop.
    opts.baseOptions.hostSystem = 3; // Windows
    opts.runningMode = mediapipe::kRunningModeVideo;
    opts.numHands = std::clamp(maxHands, 1, 2);
    opts.minHandDetectionConfidence = 0.5f;
    opts.minHandPresenceConfidence = 0.5f;
    opts.minTrackingConfidence = 0.5f;

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
    std::cout << "[MediaPipe] Initialized in synchronous video mode.\n"
              << "Model path: " << modelFileString << "\n";
    return true;
}

HandTrackingResult MediaPipeBackend::detect(const cv::Mat& frame,
                                             int64_t timestampMs) {
    if (!initialized_ || !landmarker_ || frame.empty()) {
        return HandTrackingResult{{}, timestampMs};
    }

    timestampMs = std::max(timestampMs, lastTimestampMs_ + 1);
    lastTimestampMs_ = timestampMs;

    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

    void* mpImage = nullptr;
    char* errMsg = nullptr;
    
    const auto imageByteCount = rgbFrame.step[0] * static_cast<std::size_t>(rgbFrame.rows);
    if (imageByteCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "[MediaPipe] Frame is too large for the C API.\n";
        return HandTrackingResult{{}, timestampMs};
    }

    // kMpImageFormatSrgb is 1.
    int status = fnImageCreate_(1, rgbFrame.cols, rgbFrame.rows, rgbFrame.data,
                                static_cast<int>(imageByteCount), &mpImage, &errMsg);
    if (status != 0 || !mpImage) {
        if (errMsg) {
            std::cerr << "[MediaPipe] Could not create image: " << errMsg << "\n";
            fnErrorFree_(errMsg);
        }
        return HandTrackingResult{{}, timestampMs};
    }

    mediapipe::HandLandmarkerResult rawResult{};
    status = fnDetectForVideo_(landmarker_, mpImage, nullptr, timestampMs, &rawResult, &errMsg);
    fnImageFree_(mpImage);

    if (status != 0) {
        std::cerr << "[MediaPipe] Detection failed";
        if (errMsg) {
            std::cerr << ": " << errMsg;
            fnErrorFree_(errMsg);
        }
        std::cerr << "\n";
        return HandTrackingResult{{}, timestampMs};
    }

    HandTrackingResult result;
    result.timestampMs = timestampMs;
    for (std::uint32_t i = 0; i < rawResult.handLandmarksCount; ++i) {
        const auto& sourceLandmarks = rawResult.handLandmarks[i];
        if (sourceLandmarks.landmarksCount != HandLandmarkIndex::COUNT) {
            std::cerr << "[MediaPipe] Ignoring incomplete hand: expected 21 landmarks, got "
                      << sourceLandmarks.landmarksCount << "\n";
            continue;
        }

        HandData hand;
        for (int j = 0; j < HandLandmarkIndex::COUNT; ++j) {
            const auto& source = sourceLandmarks.landmarks[j];
            hand.landmarks[j] = {source.x, source.y, source.z,
                                 source.hasVisibility ? source.visibility : 1.0f};
        }

        if (i < rawResult.handWorldLandmarksCount &&
            rawResult.handWorldLandmarks[i].landmarksCount == HandLandmarkIndex::COUNT) {
            hand.hasWorldLandmarks = true;
            for (int j = 0; j < HandLandmarkIndex::COUNT; ++j) {
                const auto& source = rawResult.handWorldLandmarks[i].landmarks[j];
                hand.worldLandmarks[j] = {source.x, source.y, source.z,
                                          source.hasVisibility ? source.visibility : 1.0f};
            }
        }

        if (i < rawResult.handednessCount && rawResult.handedness[i].categoriesCount > 0) {
            const auto& category = rawResult.handedness[i].categories[0];
            hand.confidence = category.score;
            const char* name = category.categoryName ? category.categoryName : category.displayName;
            if (name) {
                const std::string handedness(name);
                if (handedness == "Left") hand.handedness = Handedness::Left;
                if (handedness == "Right") hand.handedness = Handedness::Right;
            }
        }

        result.hands.push_back(hand);
    }

    fnCloseResult_(&rawResult);
    return result;
}

std::vector<GestureEvent> MediaPipeBackend::recognizeGestures(
    const HandTrackingResult& result) {
    return gestureClassifier_.classify(result);
}

} // namespace livevel
