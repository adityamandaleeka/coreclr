cmake_minimum_required (VERSION 2.6)
project (MarshalBoolArrayDLL)
include_directories(${INC_PLATFORM_DIR})
set(SOURCES MarshalBoolArrayDLL.cpp MarshalBoolArrayDLL.def)

# add the executable
add_library (MarshalBoolArrayDLL SHARED ${SOURCES})

# set the library output directory to match the relative zzzzzz
set_target_properties(MarshalBoolArrayDLL PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/MarshalBoolArray)