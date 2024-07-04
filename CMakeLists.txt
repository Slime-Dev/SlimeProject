cmake_minimum_required(VERSION 3.20)
project(SlimeOdyssey VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${Vulkan_INCLUDE_DIR}
)

# Source files
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Call shader compilation function
include(cmake/compile_shaders.cmake)
compile_shaders()

# Enable testing
include(CTest)

# If on Windows, configure test script
if(WIN32)
    add_test(NAME OpenClose COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/open_close.bat $<TARGET_FILE:${PROJECT_NAME}>)
endif()

# LIBRARIES AND DEPENDENCIES ################################################

# CPM setup for external libraries
include(cmake/CPM.cmake)

# Add packages
message(STATUS "Attempting auto locate Vulkan using CMake......")
	
# Find Vulkan Path using CMake's Vulkan Module
# This will return Boolean 'Vulkan_FOUND' indicating the status of find as success(ON) or fail(OFF).
# Include directory path - 'Vulkan_INCLUDE_DIRS' and 'Vulkan_LIBRARY' with required libraries.
find_package(Vulkan)

# Try extracting VulkanSDK path from ${Vulkan_INCLUDE_DIRS}
if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
    set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
    STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
    message(STATUS "Vulkan SDK Path: ${VULKAN_PATH}")
endif()
     
if(NOT Vulkan_FOUND)
    # CMake may fail to locate the libraries but could be able to 
    # provide some path in Vulkan SDK include directory variable
    # 'Vulkan_INCLUDE_DIRS', try to extract path from this.
    message(STATUS "Failed to locate Vulkan SDK, retrying again...")
    if(EXISTS "${VULKAN_PATH}")
        message(STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}")
    else()
        message("Error: Unable to locate Vulkan SDK. Please turn off auto locate option by specifying 'AUTO_LOCATE_VULKAN' as 'OFF'")
        message("and specify manually path using 'VULKAN_SDK' and 'VULKAN_VERSION' variables in the CMakeLists.txt.")
        return()
    endif()
endif()

CPMAddPackage(
    NAME SPIRV-Cross
    GITHUB_REPOSITORY KhronosGroup/SPIRV-Cross
    GIT_TAG vulkan-sdk-1.3.283.0
    OPTIONS
        "SPIRV_CROSS_ENABLE_TESTS OFF"
)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.4
    OPTIONS 
         "GLFW_BUILD_DOCS OFF"
         "GLFW_BUILD_TESTS OFF"
         "GLFW_BUILD_EXAMPLES OFF"
)

CPMAddPackage(
    NAME vk-bootstrap
    GITHUB_REPOSITORY charles-lunarg/vk-bootstrap
    GIT_TAG v1.3.285
)

CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.14.1
)

CPMAddPackage(
    NAME fastgltf
    GITHUB_REPOSITORY spnda/fastgltf
    GIT_TAG v0.7.2
)

CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.1
)

CPMAddPackage(
    NAME GSL
    GITHUB_REPOSITORY microsoft/GSL
    GIT_TAG v4.0.0
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        vk-bootstrap::vk-bootstrap
        spdlog::spdlog
        fastgltf::fastgltf
        glm::glm
        Microsoft.GSL::GSL
        
        Vulkan::Vulkan

        spirv-cross-c
        spirv-cross-core
        spirv-cross-cpp
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-util
)

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
