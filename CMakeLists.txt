cmake_minimum_required(VERSION 3.6)

project(SlimeOdyssey)
set(CMAKE_CXX_STANDARD 20)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/Vulkan)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/Vulkan/*.cpp" "src/Vulkan/*.h")

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

CPMAddPackage(
        NAME Vulkan-Headers
        GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
        GIT_TAG v1.3.286
        OPTIONS "-w"
)

CPMAddPackage(
        NAME Vulkan-Loader
        GITHUB_REPOSITORY KhronosGroup/Vulkan-Loader
        GIT_TAG v1.3.285
        OPTIONS "-w"
)

CPMAddPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS "GLFW_BUILD_DOCS OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_EXAMPLES OFF" "-w"
)

CPMAddPackage(
        NAME vk-bootstrap
        GITHUB_REPOSITORY charles-lunarg/vk-bootstrap
        GIT_TAG v1.3.285
        OPTIONS "-w"
)

CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        GIT_TAG v1.14.1
        OPTIONS "-w"
)

CPMAddPackage(
        NAME fastgltf
        GITHUB_REPOSITORY spnda/fastgltf
        GIT_TAG v0.7.2
        OPTIONS "-w"
)

CPMAddPackage(
        NAME glm
        GITHUB_REPOSITORY g-truc/glm
        GIT_TAG 1.0.1
        OPTIONS "-w"
)

# Link libraries
target_link_libraries(SlimeOdyssey Vulkan::Loader Vulkan::Headers glfw vk-bootstrap::vk-bootstrap spdlog::spdlog fastgltf::fastgltf glm::glm)
