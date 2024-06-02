#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Vulkan (vk_mem_alloc, vk-bootstrap, vk)
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

int main() {
    // Initialize Vulkan with vk-bootstrap
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Vulkan Tutorial")
        .request_validation_layers(true)
        .require_api_version(1, 2, 0)
        .use_default_debug_messenger()
        .build();

    // Check if Vulkan was initialized successfully
    if (!inst_ret) {
        return -1;
    }

    // Get the Vulkan instance
    vkb::Instance vkb_inst = inst_ret.value();
    VkInstance instance = vkb_inst.instance;

    // Create a window with GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Set the window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create the window and pass the Vulkan instance to it
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Tutorial", nullptr, nullptr);

    // Check if the window was created successfully
    if (!window) {
        return -1;
    }

    // Create a Vulkan surface
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        return -1;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll for events
        glfwPollEvents();
    }

    // Destroy the window
    glfwDestroyWindow(window);

    // Terminate GLFW
    glfwTerminate();

    // Exit the program
    return 0;
}
