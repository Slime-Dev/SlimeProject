#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>

#include <spdlog/spdlog.h>

struct VulkanHolder {
    vkb::Instance vkb_instance;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    vkb::Device vkb_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkSurfaceKHR surface;
    vkb::Swapchain swapchain;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
};

VkBool32 DebugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void *pUserData) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            spdlog::debug("{}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            spdlog::info("{}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn("{}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error("{}", pCallbackData->pMessage);
            break;
        default:
            break;
    }
    return VK_FALSE;
}

VulkanHolder InitVulkan (GLFWwindow* window) {
    spdlog::info("Initializing Vulkan");

    vkb::InstanceBuilder builder;
    builder.set_app_name ("Example Vulkan Application");
    builder.require_api_version(1,3,200);

#ifndef DNDEBUG
    builder.request_validation_layers();
    builder.set_debug_callback (DebugCallback);
#endif

    auto inst_ret = builder.build ();
    if (!inst_ret) {
        spdlog::error("Failed to create Vulkan instance: {}", inst_ret.error ().message ());
        assert (false);
    }
    vkb::Instance vkb_inst = inst_ret.value ();

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface (vkb_inst.instance, window, nullptr, &surface) != VK_SUCCESS) {
        spdlog::error("Failed to create window surface");
        assert (false);
    }

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    auto phys_ret = selector.set_surface (surface)
                        .set_minimum_version (1, 3)
                        .require_dedicated_transfer_queue ()
                        .select ();
    if (!phys_ret) {
        spdlog::error("Failed to select Vulkan physical device: {}", phys_ret.error ().message ());
        assert (false);
    }

    // Log the physical device name and details
    spdlog::info("Selected physical device: {}", phys_ret.value ().properties.deviceName);
    spdlog::info("API Version: {}.{}.{}", VK_VERSION_MAJOR(phys_ret.value ().properties.apiVersion),
        VK_VERSION_MINOR(phys_ret.value ().properties.apiVersion), VK_VERSION_PATCH(phys_ret.value ().properties.apiVersion));

    vkb::DeviceBuilder device_builder{ phys_ret.value () };
    // automatically propagate needed data from instance & physical device
    auto dev_ret = device_builder.build ();
    if (!dev_ret) {
        spdlog::error("Failed to create Vulkan device: {}", dev_ret.error ().message ());
        assert (false);
    }
    vkb::Device vkb_device = dev_ret.value ();

    // Get the VkDevice handle used in the rest of a vulkan application
    VkDevice device = vkb_device.device;

    // Get the graphics queue
    auto graphics_queue_ret = vkb_device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        spdlog::error("Failed to get graphics queue: {}", graphics_queue_ret.error ().message ());
        assert (false);
    }
    VkQueue graphics_queue = graphics_queue_ret.value ();

    // Get the present queue
    auto present_queue_ret = vkb_device.get_queue (vkb::QueueType::present);
    if (!present_queue_ret) {
        spdlog::error("Failed to get present queue: {}", present_queue_ret.error ().message ());
        assert (false);
    }

    // create a swapchain
    vkb::SwapchainBuilder swapchain_builder{ vkb_device };
    auto swapchain_ret = swapchain_builder.use_default_format_selection ()
                            .set_desired_present_mode (VK_PRESENT_MODE_FIFO_KHR)
                            .set_desired_extent (800, 600)
                            .build ();

    // Get the swapchain
    if (!swapchain_ret) {
        spdlog::error("Failed to create swapchain: {}", swapchain_ret.error ().message ());
        assert (false);
    }

    return {
        .vkb_instance = vkb_inst,
        .instance = vkb_inst.instance,
        .physical_device = vkb_device.physical_device,
        .vkb_device = vkb_device,
        .device = device,
        .graphics_queue = graphics_queue,
        .present_queue = present_queue_ret.value (),
        .surface = surface,
        .swapchain = swapchain_ret.value(),
        .swapchain_image_format = swapchain_ret.value ().image_format,
        .swapchain_extent = swapchain_ret.value ().extent
    };
}

int main() {
    // Create glfw window
    spdlog::info("Creating window");
    glfwInit ();
    glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow (1920, 1080, "Vulkan Window", nullptr, nullptr);

    // Initialize Vulkan
    VulkanHolder vulkan = InitVulkan (window);

    // main loop
    spdlog::info("Starting main loop");
    while (!glfwWindowShouldClose (window)) {
        glfwPollEvents ();

        // If escape key is pressed, close the window
        if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            spdlog::info("Escape key pressed, closing window");
            glfwSetWindowShouldClose (window, GLFW_TRUE);
        }
    }

    // Destroy the window
    spdlog::info("Destroying window");
    glfwDestroyWindow (window);
    glfwTerminate ();

    // Cleanup
    spdlog::info("Cleaning up");
    vkb::destroy_swapchain(vulkan.swapchain);
    vkb::destroy_surface (vulkan.instance, vulkan.surface);
    vkb::destroy_device(vulkan.vkb_device);
    vkb::destroy_instance(vulkan.vkb_instance);

    // Exit the program
    return 0;
}
