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

option(USE_CPM "Download OpenCV with CPM when it is not installed locally" OFF)

if(NOT DEFINED OpenCV_DIR)
  set(_livevel_local_opencv_dir "${CMAKE_SOURCE_DIR}/opencv/build")
  if(EXISTS "${_livevel_local_opencv_dir}/OpenCVConfig.cmake"
      OR EXISTS "${_livevel_local_opencv_dir}/opencv-config.cmake")
    set(OpenCV_DIR "${_livevel_local_opencv_dir}" CACHE PATH "Path to OpenCV CMake config")
  endif()
endif()

set(LIVEVEL_OPENCV_COMPONENTS core videoio video imgproc highgui)
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
      "BUILD_LIST core,videoio,video,imgproc,highgui"
      "BUILD_opencv_apps OFF"
      "BUILD_TESTS OFF"
      "BUILD_PERF_TESTS OFF"
      "BUILD_EXAMPLES OFF"
  )

  if(TARGET opencv_core AND TARGET opencv_videoio AND TARGET opencv_video
      AND TARGET opencv_imgproc AND TARGET opencv_highgui)
    set(LIVEVEL_WITH_OPENCV TRUE)
    set(LIVEVEL_OPENCV_LIBS opencv_core opencv_videoio opencv_video opencv_imgproc opencv_highgui)
    set(LIVEVEL_OPENCV_INCLUDE_DIRS)
  endif()
endif()

if(LIVEVEL_WITH_OPENCV)
  message(STATUS "Found OpenCV ${OpenCV_VERSION}")
else()
  message(STATUS "OpenCV was not found. The livevel executable will build with camera tracking disabled.")
endif()

add_subdirectory(livevel)

