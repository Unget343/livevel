#include "mediapipe_c_api.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <cstdint>
#include <iostream>

namespace {

using CreateImage = int (*)(int format, int width, int height, const std::uint8_t* pixels,
                            int pixelCount, void** image, char** error);
using FreeImage = void (*)(void* image);
using CreateLandmarker = int (*)(livevel::mediapipe::HandLandmarkerOptions* options,
                                 void** landmarker, char** error);
using DetectImage = int (*)(void* landmarker, void* image, const void* options,
                            livevel::mediapipe::HandLandmarkerResult* result, char** error);
using CloseResult = void (*)(livevel::mediapipe::HandLandmarkerResult* result);
using CloseLandmarker = int (*)(void* landmarker, char** error);
using FreeError = void (*)(char* error);

template <typename T>
T load(HMODULE dll, const char* name) {
    return reinterpret_cast<T>(GetProcAddress(dll, name));
}

int reportFailure(const char* action, char* error, FreeError freeError) {
    std::cerr << action;
    if (error) {
        std::cerr << ": " << error;
        freeError(error);
    }
    std::cerr << "\n";
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: mediapipe_runtime_test <libmediapipe.dll> <hand_landmarker.task>\n";
        return 2;
    }

    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll) return reportFailure("Could not load libmediapipe.dll", nullptr, nullptr);

    const auto createImage = load<CreateImage>(dll, "MpImageCreateFromUint8Data");
    const auto freeImage = load<FreeImage>(dll, "MpImageFree");
    const auto createLandmarker = load<CreateLandmarker>(dll, "MpHandLandmarkerCreate");
    const auto detectImage = load<DetectImage>(dll, "MpHandLandmarkerDetectImage");
    const auto closeResult = load<CloseResult>(dll, "MpHandLandmarkerCloseResult");
    const auto closeLandmarker = load<CloseLandmarker>(dll, "MpHandLandmarkerClose");
    const auto freeError = load<FreeError>(dll, "MpErrorFree");
    if (!createImage || !freeImage || !createLandmarker || !detectImage || !closeResult ||
        !closeLandmarker || !freeError) {
        FreeLibrary(dll);
        std::cerr << "MediaPipe runtime does not provide the required Hand Landmarker C API.\n";
        return 1;
    }

    livevel::mediapipe::HandLandmarkerOptions options{};
    options.baseOptions.modelAssetPath = argv[2];
    options.baseOptions.delegate = 0;
    options.baseOptions.hostSystem = 3; // Windows
    options.runningMode = livevel::mediapipe::kRunningModeImage;
    options.numHands = 2;
    options.minHandDetectionConfidence = 0.5f;
    options.minHandPresenceConfidence = 0.5f;
    options.minTrackingConfidence = 0.5f;

    void* landmarker = nullptr;
    char* error = nullptr;
    if (createLandmarker(&options, &landmarker, &error) != 0 || !landmarker) {
        const int result = reportFailure("Could not initialize Hand Landmarker", error, freeError);
        FreeLibrary(dll);
        return result;
    }

    constexpr int width = 8;
    constexpr int height = 8;
    std::array<std::uint8_t, width * height * 3> blackFrame{};
    void* image = nullptr;
    if (createImage(1, width, height, blackFrame.data(), static_cast<int>(blackFrame.size()),
                    &image, &error) != 0 || !image) {
        closeLandmarker(landmarker, nullptr);
        const int result = reportFailure("Could not create RGB image", error, freeError);
        FreeLibrary(dll);
        return result;
    }

    livevel::mediapipe::HandLandmarkerResult result{};
    const int status = detectImage(landmarker, image, nullptr, &result, &error);
    freeImage(image);
    if (status != 0) {
        closeLandmarker(landmarker, nullptr);
        const int exitCode = reportFailure("Hand Landmarker inference failed", error, freeError);
        FreeLibrary(dll);
        return exitCode;
    }

    // A blank image must not invent a hand. If a hand is ever returned, the
    // runtime contract requires a complete 21-point landmark list.
    for (std::uint32_t i = 0; i < result.handLandmarksCount; ++i) {
        if (result.handLandmarks[i].landmarksCount != 21) {
            closeResult(&result);
            closeLandmarker(landmarker, nullptr);
            FreeLibrary(dll);
            std::cerr << "MediaPipe returned an incomplete hand.\n";
            return 1;
        }
    }
    closeResult(&result);
    closeLandmarker(landmarker, nullptr);
    FreeLibrary(dll);

    std::cout << "MediaPipe model/runtime smoke test passed.\n";
    return 0;
}
