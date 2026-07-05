add_executable(livevel livevel.cpp)

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
