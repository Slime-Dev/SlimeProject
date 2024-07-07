cmake_minimum_required(VERSION 3.20)
project(SlimeOdyssey VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external
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

# check if ${SHADER_DIR} is set
if(NOT DEFINED SHADER_DIR)
    set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders")
endif()

include(cmake/compile_shaders.cmake)
compile_shaders()

# LIBRARIES AND DEPENDENCIES ################################################
include(cmake/link_libs.cmake)

if(Vulkan_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Vulkan not found. Please install Vulkan SDK.")
endif()

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