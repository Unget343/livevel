cmake_minimum_required(VERSION 3.28)

project(Livevel LANGUAGES C CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_CPM "Whether to use CPM" OFF)

include(CPM.cmake)
find_package(OpenCV REQUIRED)

if(USE_CPM)
    cpmaddpackage("gh:OpenCV/opencv@4.8.1")
endif()

add_executable(livevel
    src/main.cpp
)

target_link_libraries(livevel PUBLIC ${OpenCV_LIBS})
target_include_directories(livevel PUBLIC ${OpenCV_INCLUDE_DIRS})