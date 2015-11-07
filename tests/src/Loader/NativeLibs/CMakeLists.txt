cmake_minimum_required(VERSION 2.6)
project(FromNativePaths_lib)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(SOURCES FromNativePaths_lib.cpp FromNativePaths_lib.def)
add_library(FromNativePaths_lib SHARED ${SOURCES})

# set the library output directory to match the relative zzzzzz
set_target_properties(FromNativePaths_lib PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/FromNativePaths)