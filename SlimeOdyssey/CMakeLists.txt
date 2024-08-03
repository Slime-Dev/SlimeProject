cmake_minimum_required(VERSION 3.20)
project(SlimeOdyssey VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch libraries
include(cmake/fetch_libs.cmake)

# Source files
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/*.h"
)

# ImGui Setup
set(IMGUI_SOURCES
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
set(SOURCES ${SOURCES} ${IMGUI_SOURCES})

# Create SlimeOdyssey library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Enable Unity Build
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    VK_NO_PROTOTYPES
    IMGUI_IMPL_VULKAN_DYNAMIC_LOADER=glfwGetInstanceProcAddress
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
    IMGUI_DEFINE_MATH_OPERATORS
    GLM_ENABLE_EXPERIMENTAL
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

# Call shader compilation function

# check if ${SHADER_DIR} is set
if(NOT DEFINED SHADER_DIR)
    set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders")
endif()

include(cmake/compile_shaders.cmake)
compile_shaders()

# LIBRARIES AND DEPENDENCIES ################################################

# Link libraries
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        glfw
        vk-bootstrap::vk-bootstrap
        spdlog::spdlog
        glm::glm
        Microsoft.GSL::GSL
        Vulkan::Vulkan
        spirv-cross-glsl
        VulkanMemoryAllocator
        tinyobjloader
)

target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SPIRV_CROSS_INCLUDE_DIRS})

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

set(PCH_HEADERS
    <vector>
    <string>
    <glm/glm.hpp>
    <vulkan/vulkan.h>
)

target_precompile_headers(${PROJECT_NAME} PRIVATE ${PCH_HEADERS})

# set a variable so that any project that includes this project can use the include directories
set(SLIME_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/external ${IMGUI_SOURCE_DIR} PARENT_SCOPE)

# set a path to the resources directory
set(SLIME_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources" PARENT_SCOPE)

# Set a path for slimeodyssey cmake directory
set(SLIME_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" PARENT_SCOPE)