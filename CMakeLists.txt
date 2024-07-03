cmake_minimum_required(VERSION 3.6)

project(SlimeOdyssey)
set(CMAKE_CXX_STANDARD 20)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/external)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "external/*")

# Create executable
add_executable(SlimeOdyssey ${SOURCES})

# Call shader compilation function
include(cmake/compile_shaders.cmake)
compile_shaders()

# Enable testing
enable_testing()

# If on Windows, configure test script
if (WIN32)
    add_test(NAME OpenClose COMMAND ../../tests/open_close.bat $<TARGET_FILE:SlimeOdyssey>)
endif ()

# LIBRARIES AND DEPENDENCIES ################################################

# CPM setup for external libraries (example with Vulkan, GLFW, etc.)
include(cmake/CPM.cmake)

# Define warning suppression flags for MSVC
if(MSVC)
    set(MSVC_WARNING_SUPPRESSION "/W0")
else()
    set(WARNING_SUPPRESSION "-w")
endif()

CPMAddPackage(
        NAME Vulkan-Headers
        GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
        GIT_TAG v1.3.286
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for Vulkan-Headers
)

CPMAddPackage(
        NAME Vulkan-Loader
        GITHUB_REPOSITORY KhronosGroup/Vulkan-Loader
        GIT_TAG v1.3.285
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for Vulkan-Loader
)

CPMAddPackage(
        NAME SPIRV-Headers
        GITHUB_REPOSITORY KhronosGroup/SPIRV-Headers
        GIT_TAG vulkan-sdk-1.3.283.0
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for SPIRV-Cross
)

CPMAddPackage(
        NAME SPIRV-Tools
        GITHUB_REPOSITORY KhronosGroup/SPIRV-Tools
        GIT_TAG v2024.3
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for SPIRV-Cross
)

CPMAddPackage(
        NAME SPIRV-Cross
        GITHUB_REPOSITORY KhronosGroup/SPIRV-Cross
        GIT_TAG vulkan-sdk-1.3.283.0
        OPTIONS
        "SPIRV_CROSS_CLI OFF"
        "SPIRV_CROSS_ENABLE_TESTS OFF"
        "SPIRV_CROSS_ENABLE_HLSL OFF"
        "SPIRV_CROSS_ENABLE_MSL OFF"
        "SPIRV_CROSS_ENABLE_CPP OFF"
        "SPIRV_CROSS_ENABLE_REFLECT OFF"
        "SPIRV_CROSS_ENABLE_C_API OFF"
        "SPIRV_CROSS_ENABLE_UTIL OFF"
        "SPIRV_CROSS_STATIC ON"
        OPTIONS "${WARNING_SUPPRESSION}"
)

CPMAddPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS "GLFW_BUILD_DOCS OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_EXAMPLES OFF" "${WARNING_SUPPRESSION}" # Suppress warnings for glfw
)

CPMAddPackage(
        NAME vk-bootstrap
        GITHUB_REPOSITORY charles-lunarg/vk-bootstrap
        GIT_TAG v1.3.285
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for vk-bootstrap
)

CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        GIT_TAG v1.14.1
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for spdlog
)

CPMAddPackage(
        NAME fastgltf
        GITHUB_REPOSITORY spnda/fastgltf
        GIT_TAG v0.7.2
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for fastgltf
)

CPMAddPackage(
        NAME glm
        GITHUB_REPOSITORY g-truc/glm
        GIT_TAG 1.0.1
        OPTIONS "${WARNING_SUPPRESSION}" # Suppress warnings for glm
)

# Link libraries
target_link_libraries(SlimeOdyssey
        PRIVATE
        Vulkan::Loader
        Vulkan::Headers
        glfw
        vk-bootstrap::vk-bootstrap
        spdlog::spdlog
        fastgltf::fastgltf
        glm::glm
        spirv-cross-core
        spirv-cross-glsl
)

# Include SPIRV-Cross headers
target_include_directories(SlimeOdyssey PRIVATE ${SPIRV-Cross_SOURCE_DIR})
