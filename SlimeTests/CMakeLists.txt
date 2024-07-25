cmake_minimum_required(VERSION 3.20)
project(SlimeGame VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Set test time limit
set(CTEST_TEST_TIMEOUT 300)

# Add the SlimeOdyssey library
if(NOT DEFINED SHADER_DIR)
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/../SlimeOdyssey/resources/shaders")
endif()

if(NOT TARGET SlimeOdyssey)
    message(STATUS "Adding SlimeOdyssey library")
    add_subdirectory(../SlimeOdyssey ${CMAKE_BINARY_DIR}/SlimeOdyssey)
endif()

# Source files for SlimeGame
file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Function to create symlink or copy based on the build type
function(link_or_copy_resources TARGET_NAME)
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        COMMAND ${CMAKE_COMMAND}
        -D SOURCE_DIR=${SLIME_RESOURCES_DIR}
        -D DESTINATION_DIR="$<TARGET_FILE_DIR:${TARGET_NAME}>"
        -D BUILD_TYPE=$<CONFIG>
        -P "${SLIME_CMAKE_DIR}/CopyOrLink.cmake"
        COMMENT "Linking or copying SlimeOdyssey resources to executable directory"
    )
endfunction()

# Function to create a test executable
function(create_test_executable NAME SOURCE_FILE)
    add_executable(${NAME} ${SOURCE_FILE})
    target_link_libraries(${NAME} PRIVATE SlimeOdyssey)
    target_include_directories(${NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../SlimeOdyssey/include
    )
    set_target_properties(${NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    target_precompile_headers(${NAME} REUSE_FROM SlimeOdyssey)
    link_or_copy_resources(${NAME})
    add_test(NAME ${NAME}Test COMMAND ${NAME})
endfunction()

# Create test executables
create_test_executable(ModelLoading ModelLoading.cpp)
create_test_executable(CameraInitializing CameraInitializing.cpp)
