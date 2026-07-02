# Загружаем конфигурацию MSVC
include(${CMAKE_CURRENT_SOURCE_DIR}/../MSVCConfig.cmake)

# Установка стандартов C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поддержка MSVC
if(MSVC)
    # Добавляем флаги компиляции для MSVC
    add_compile_options(/W4)
    # Явно указываем Windows SDK версию если нужно
    if(NOT DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
        set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0)
    endif()
endif()

find_package(OpenCV REQUIRED)

add_executable(livevel livevel.cpp)

target_include_directories(livevel PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(livevel PRIVATE ${OpenCV_LIBS})
