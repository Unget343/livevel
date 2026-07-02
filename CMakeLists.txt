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

option(USE_CPM "Whether to use CPM" OFF)

include(CPM.cmake)

if(USE_CPM)
  cpmaddpackage("gh:OpenCV/opencv@4.8.1")
endif()

set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/opencv/build" CACHE PATH "Path to OpenCV CMake config")
find_package(OpenCV REQUIRED)

add_subdirectory(livevel)

