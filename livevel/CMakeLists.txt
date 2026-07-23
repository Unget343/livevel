set(LIVEVEL_SOURCES livevel.cpp)

if(LIVEVEL_WITH_OPENCV)
    list(APPEND LIVEVEL_SOURCES
        camera.cpp
        renderer.cpp
        region_cache.cpp
        thread_manager.cpp
        frame_processor.cpp
        gesture_classifier.cpp
        hand_overlay_renderer.cpp
        mediapipe_backend.cpp
        hand_tracker.cpp
    )
endif()

add_executable(livevel ${LIVEVEL_SOURCES})

target_compile_features(livevel PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(livevel PRIVATE /W4)
endif()

if(LIVEVEL_WITH_OPENCV)
    target_compile_definitions(livevel PRIVATE LIVEVEL_HAVE_OPENCV=1)
    target_include_directories(livevel PRIVATE ${LIVEVEL_OPENCV_INCLUDE_DIRS})
    target_link_libraries(livevel PRIVATE ${LIVEVEL_OPENCV_LIBS})
else()
    target_compile_definitions(livevel PRIVATE LIVEVEL_HAVE_OPENCV=0)
endif()

if(MEDIAPIPE_DLL)
    if(NOT EXISTS "${MEDIAPIPE_DLL}")
        message(FATAL_ERROR "MEDIAPIPE_DLL does not exist: ${MEDIAPIPE_DLL}")
    endif()

    add_custom_command(TARGET livevel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MEDIAPIPE_DLL}" "$<TARGET_FILE_DIR:livevel>")
endif()

if(EXISTS "${MEDIAPIPE_MODEL}")
    add_custom_command(TARGET livevel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:livevel>/models"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MEDIAPIPE_MODEL}" "$<TARGET_FILE_DIR:livevel>/models/hand_landmarker.task")
endif()
