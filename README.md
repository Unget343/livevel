# livevel

Webcam hand tracking with MediaPipe's real 21-point hand model: wrist, every
finger joint, tips, handedness, and 3D world landmarks when supplied by the
runtime. The overlay draws each landmark and the correct hand skeleton.

## Setup (Windows)

Open **x64 Native Tools Command Prompt for Visual Studio**, then run:

```powershell
.\models\download_model.ps1
$mpDll = python -c "import mediapipe, pathlib; print(pathlib.Path(mediapipe.__file__).parent / 'tasks' / 'c' / 'libmediapipe.dll')"
cmake --preset x64-debug -DUSE_CPM=ON -DMEDIAPIPE_DLL="$mpDll"
cmake --build --preset x64-debug
ctest --test-dir out\build\x64-debug --output-on-failure
```

`MEDIAPIPE_DLL` and the model are copied next to `livevel.exe`. To run a
manually deployed binary, put `libmediapipe.dll` beside it or set
`LIVEVEL_MEDIAPIPE_DLL` to its full path. The tracker reports an error instead
of fabricating finger joints if the model or runtime is unavailable.

Do not run an old executable built before this setup: a build without OpenCV
now fails at configure time instead of opening a camera app with tracking
disabled.

Press `Esc` to close the camera window.
