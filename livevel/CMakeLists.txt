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

# Находим папку с DLL файлами OpenCV и копируем их в папку с программой
get_filename_component(OPENCV_BIN_DIR "${OpenCV_LIB_PATH}/../bin" ABSOLUTE)
file(GLOB OPENCV_DLLS "${OPENCV_BIN_DIR}/*.dll")
add_custom_command(TARGET livevel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${OPENCV_DLLS}
    $<TARGET_FILE_DIR:livevel>
)
