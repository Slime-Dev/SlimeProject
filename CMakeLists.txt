cmake_minimum_required(VERSION 3.6)

project(SlimeOdyssey)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/Vulkan)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/Vulkan/*.cpp" "src/Vulkan/*.h")

# enable testing
enable_testing()

add_executable(SlimeOdyssey ${SOURCES})

# if on windows, pass exe directory to the bat script
if(WIN32)
  add_test(NAME OpenClose COMMAND ../../tests/open_close.bat $<TARGET_FILE:SlimeOdyssey>)
endif()

# LIBRARIES AND DEPENDENCIES ################################################
include(cmake/CPM.cmake)

# use the vulkan loader from cpm
CPMAddPackage(
  NAME Vulkan-Headers
  GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
  GIT_TAG v1.3.286
)

CPMAddPackage(
  NAME Vulkan-Loader
  GITHUB_REPOSITORY KhronosGroup/Vulkan-Loader
  GIT_TAG v1.3.285
)

# using CPM to download and build GLFW
CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.4
  OPTIONS "GLFW_BUILD_DOCS OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_EXAMPLES OFF"
)

# vulkan bootstrap
CPMAddPackage(
        NAME vk-bootstrap
        GITHUB_REPOSITORY charles-lunarg/vk-bootstrap
        GIT_TAG v1.3.285
)

# Add the Vulkan Memory Allocator package
CPMAddPackage(
        NAME VulkanMemoryAllocator
        GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG v3.1.0
)

# SPDLOG
CPMAddPackage(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  GIT_TAG v1.14.1
)

# Link libraries
target_link_libraries(SlimeOdyssey Vulkan::Loader Vulkan::Headers glfw vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator spdlog::spdlog)
