cmake_minimum_required(VERSION 3.28)

# Ninja + MSVC: skip link step in compiler check when rc.exe is not on PATH.
if(WIN32)
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>"
  )
endif()

project(livevel LANGUAGES C CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# MediaPipe is loaded at runtime, so no import library is required. Point this
# at libmediapipe.dll to copy it next to the executable and enable the runtime
# smoke test.
set(MEDIAPIPE_DLL "" CACHE FILEPATH "Path to MediaPipe C runtime (libmediapipe.dll)")
set(MEDIAPIPE_MODEL "${CMAKE_SOURCE_DIR}/models/hand_landmarker.task" CACHE FILEPATH
    "Path to the MediaPipe Hand Landmarker model")

option(USE_CPM "Download OpenCV with CPM when it is not installed locally" OFF)

if(NOT DEFINED OpenCV_DIR)
  set(_livevel_local_opencv_dir "${CMAKE_SOURCE_DIR}/opencv/build")
  if(EXISTS "${_livevel_local_opencv_dir}/OpenCVConfig.cmake"
      OR EXISTS "${_livevel_local_opencv_dir}/opencv-config.cmake")
    set(OpenCV_DIR "${_livevel_local_opencv_dir}" CACHE PATH "Path to OpenCV CMake config")
  endif()
endif()

set(LIVEVEL_OPENCV_COMPONENTS core videoio video imgproc highgui imgcodecs)
find_package(OpenCV QUIET COMPONENTS ${LIVEVEL_OPENCV_COMPONENTS})

set(LIVEVEL_WITH_OPENCV ${OpenCV_FOUND})
set(LIVEVEL_OPENCV_LIBS ${OpenCV_LIBS})
set(LIVEVEL_OPENCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})

if(NOT LIVEVEL_WITH_OPENCV AND USE_CPM)
  include(CPM.cmake)
  CPMAddPackage(
    NAME opencv
    GITHUB_REPOSITORY opencv/opencv
    GIT_TAG 4.8.1
    OPTIONS
      "BUILD_LIST core,videoio,video,imgproc,highgui,imgcodecs"
      "BUILD_opencv_apps OFF"
      "BUILD_TESTS OFF"
      "BUILD_PERF_TESTS OFF"
      "BUILD_EXAMPLES OFF"
  )

  if(TARGET opencv_core AND TARGET opencv_videoio AND TARGET opencv_video
      AND TARGET opencv_imgproc AND TARGET opencv_highgui AND TARGET opencv_imgcodecs)
    set(LIVEVEL_WITH_OPENCV TRUE)
    set(LIVEVEL_OPENCV_LIBS opencv_core opencv_videoio opencv_video opencv_imgproc opencv_highgui opencv_imgcodecs)
    set(LIVEVEL_OPENCV_INCLUDE_DIRS "${CMAKE_BINARY_DIR}" "${opencv_BINARY_DIR}")
    foreach(component IN LISTS LIVEVEL_OPENCV_COMPONENTS)
      list(APPEND LIVEVEL_OPENCV_INCLUDE_DIRS
        "${opencv_SOURCE_DIR}/modules/${component}/include")
    endforeach()
  endif()
endif()

if(LIVEVEL_WITH_OPENCV)
  message(STATUS "Found OpenCV ${OpenCV_VERSION}")
else()
  message(STATUS "OpenCV was not found. The livevel executable will build with camera tracking disabled.")
endif()

add_subdirectory(livevel)

if(BUILD_TESTING)
  add_executable(hand_tracking_contract_test tests/hand_tracking_contract_test.cpp)
  target_include_directories(hand_tracking_contract_test PRIVATE
    "${CMAKE_SOURCE_DIR}/livevel")
  target_compile_features(hand_tracking_contract_test PRIVATE cxx_std_20)
  add_test(NAME hand_tracking_contract COMMAND hand_tracking_contract_test)

  if(WIN32 AND EXISTS "${MEDIAPIPE_DLL}" AND EXISTS "${MEDIAPIPE_MODEL}")
    add_executable(mediapipe_runtime_test tests/mediapipe_runtime_test.cpp)
    target_include_directories(mediapipe_runtime_test PRIVATE
      "${CMAKE_SOURCE_DIR}/livevel")
    target_compile_features(mediapipe_runtime_test PRIVATE cxx_std_20)
    add_test(NAME mediapipe_runtime
      COMMAND mediapipe_runtime_test "${MEDIAPIPE_DLL}" "${MEDIAPIPE_MODEL}")
  endif()
endif()

