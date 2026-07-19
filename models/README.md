# MediaPipe Models

This directory is used to store MediaPipe model files.

To enable the full MediaPipe C API backend for hand tracking, you need to download the `hand_landmarker.task` model file and place it in this directory.

**Download link:**
[hand_landmarker.task](https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/latest/hand_landmarker.task)

If the model file or `mediapipe_tasks.dll` is missing, the application will automatically fall back to the OpenCV skin-color based hand tracker.
