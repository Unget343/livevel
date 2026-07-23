# livevel

Webcam hand tracking with MediaPipe's real 21-point hand model: wrist, every
finger joint, tips, handedness, and 3D world landmarks when supplied by the
runtime. The overlay draws each landmark and the correct hand skeleton.

## Setup (Windows)

Open **x64 Native Tools Command Prompt for Visual Studio**, then run:

```powershell
cmake --preset x64-debug
cmake --build --preset x64-debug
ctest --test-dir out\build\x64-debug --output-on-failure
```

On the first configure CMake downloads OpenCV and the Hand Landmarker model.
It uses an installed MediaPipe package when available; otherwise it installs
the runtime into the build directory. All required DLLs and the model are then
copied beside `livevel.exe`.

Do not run an old executable built before this setup: a build without OpenCV
now fails at configure time instead of opening a camera app with tracking
disabled.

Press `Esc` to close the camera window.
