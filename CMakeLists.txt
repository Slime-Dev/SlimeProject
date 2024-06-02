cmake_minimum_required(VERSION 3.6)

project(SlimeOdyssey)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCES "src/*.cpp")

# enable testing
enable_testing()

add_executable(SlimeOdyssey ${SOURCES})

# if on windows, pass exe directory to the bat script
if(WIN32)
  add_test(NAME OpenClose COMMAND ../../tests/open_close.bat $<TARGET_FILE:SlimeOdyssey>)
endif()


# using CPM to download and build GLFW
include(cmake/CPM.cmake)
CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.4
  OPTIONS "GLFW_BUILD_DOCS OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_EXAMPLES OFF"
)


# if we didnt find vulkan then use Vulkan_LIBRARY and Vulkan_INCLUDE_DIR environment variables C:\VulkanSDK\1.3.283.0\Lib
if(NOT Vulkan_INCLUDE_DIRS OR NOT Vulkan_LIBRARIES)
  message("Vulkan not found")
  find_path(Vulkan_INCLUDE_DIRS NAMES vulkan/vulkan.h)
  find_library(Vulkan_LIBRARIES NAMES vulkan-1 vulkan PATH_SUFFIXES Lib)
endif()

# vulkan
find_package(Vulkan REQUIRED)

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

# Link libraries
target_link_libraries(SlimeOdyssey glfw Vulkan::Vulkan vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator)
