# link vulkan first to avoid errors
include(cmake/link_vulkan.cmake)

# CPM setup for external libraries
include(cmake/get_cpm.cmake)
include(${CPM_DOWNLOAD_LOCATION})

CPMFindPackage(
        NAME SPIRV-Cross
        GITHUB_REPOSITORY KhronosGroup/SPIRV-Cross
        GIT_TAG vulkan-sdk-1.3.283.0
        OPTIONS
        "SPIRV_CROSS_ENABLE_TESTS OFF"
)

CPMFindPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
)

CPMFindPackage(
        NAME vk-bootstrap
        GITHUB_REPOSITORY charles-lunarg/vk-bootstrap
        GIT_TAG v1.3.285
)

CPMFindPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        GIT_TAG v1.14.1
)

CPMFindPackage(
        NAME tinyobj
        GITHUB_REPOSITORY tinyobjloader/tinyobjloader
        GIT_TAG v2.0.0rc13
)

CPMFindPackage(
        NAME glm
        GITHUB_REPOSITORY g-truc/glm
        GIT_TAG 1.0.1
)

CPMFindPackage(
        NAME GSL
        GITHUB_REPOSITORY microsoft/GSL
        GIT_TAG v4.0.0
)

CPMFindPackage(
        NAME VulkanMemoryAllocator
        GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        VERSION 3.1.0
)

CPMFINDPACKAGE(
        NAME IMGUI
        GITHUB_REPOSITORY ocornut/imgui
        GIT_TAG v1.90.9-docking
        options
        "IMGUI_IMPL_VULKAN_NO_PROTOTYPES ON"
)

