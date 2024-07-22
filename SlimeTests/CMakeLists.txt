cmake_minimum_required(VERSION 3.20)
project(SlimeGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set test time limit
set(CTEST_TEST_TIMEOUT 300)

# Enable
enable_testing()

# Add the SlimeOdyssey library
if(NOT DEFINED SHADER_DIR)
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/../SlimeOdyssey/resources/shaders")
endif()

# check if SlimeOdyssey is already defined
if(NOT TARGET SlimeOdyssey)
    message(STATUS "Adding SlimeOdyssey library")
    add_subdirectory(../SlimeOdyssey ${CMAKE_BINARY_DIR}/SlimeOdyssey)
endif()

# Source files for SlimeGame
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Create executable
add_executable(ModelLoading test_main.cpp)

# Link SlimeOdyssey library
target_link_libraries(ModelLoading PRIVATE SlimeOdyssey)

# Add include directories for SlimeOdyssey
target_include_directories(ModelLoading PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../SlimeOdyssey/include
)

# Set output directories
set_target_properties(ModelLoading PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME ModelTest COMMAND ModelLoading)

# Create executable
add_executable(CameraInitalizing second_test.cpp)

# Link SlimeOdyssey library
target_link_libraries(CameraInitalizing PRIVATE SlimeOdyssey)

# Add include directories for SlimeOdyssey
target_include_directories(CameraInitalizing PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../SlimeOdyssey/include
)

# Set output directories
set_target_properties(CameraInitalizing PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME CameraTest COMMAND CameraInitalizing)

