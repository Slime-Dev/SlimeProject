# link vulkan first to avoid errors
include(cmake/link_vulkan.cmake)

# CPM setup for external libraries
include(cmake/CPM.cmake)

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
        NAME tinyobj
        GITHUB_REPOSITORY tinyobjloader/tinyobjloader
        GIT_TAG v2.0.0rc13
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
        glm::glm
        Microsoft.GSL::GSL
        Vulkan::Vulkan
        spirv-cross-glsl
        VulkanMemoryAllocator
        tinyobjloader
)

target_include_directories(${PROJECT_NAME} PRIVATE ${SPIRV_CROSS_INCLUDE_DIRS})