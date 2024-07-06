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

# Create library instead of executable
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Call shader compilation function
include(cmake/compile_shaders.cmake)
compile_shaders()

# LIBRARIES AND DEPENDENCIES ################################################

# CPM setup for external libraries
include(cmake/CPM.cmake)

# Add packages
message(STATUS "Attempting auto locate Vulkan using CMake......")

find_package(Vulkan)

if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
    set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
    STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
    message(STATUS "Vulkan SDK Path: ${VULKAN_PATH}")
endif()

if(NOT Vulkan_FOUND)
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

CPMAddPackage(
        NAME VulkanMemoryAllocator
        GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        VERSION 3.1.0
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
        PUBLIC
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
        VulkanMemoryAllocator
)

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Install the library and headers
target_include_directories(SlimeOdyssey PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${VulkanMemoryAllocator_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(TARGETS SlimeOdyssey
        EXPORT SlimeOdysseyTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)