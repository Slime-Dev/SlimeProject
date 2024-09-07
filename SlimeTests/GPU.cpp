#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <stdexcept>
#include <iostream>

struct TestResult {
    std::string testName;
    std::string errorMessage;
    bool passed;
};

void logTestResult(const TestResult& result) {
    if (!result.passed) {
        spdlog::error("Test '{}' failed with error: {}", result.testName, result.errorMessage);
    }
}

VkInstance createInstance() {
    VkInstance instance;
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;  // Target the latest Vulkan version

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    return instance;
}

VkPhysicalDevice selectPhysicalDevice(VkInstance instance) {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    return physicalDevice;
}

VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice) {
    VkDevice device;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    if (queueFamilyCount == 0) {
        throw std::runtime_error("Failed to find any queue families");
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0;  // Assume the first queue supports graphics
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }

    return device;
}

void runVulkanInitializationTests() {
    TestResult result;

    try {
        result.testName = "Instance Creation";
        VkInstance instance = createInstance();
        result.passed = true;

        result.testName = "Physical Device Selection";
        VkPhysicalDevice physicalDevice = selectPhysicalDevice(instance);
        result.passed = true;

        result.testName = "Logical Device Creation";
        VkDevice device = createLogicalDevice(physicalDevice);
        result.passed = true;

        // Cleanup resources
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
    } catch (const std::exception& e) {
        result.passed = false;
        result.errorMessage = e.what();
        logTestResult(result);
    }
}

int main() {
    runVulkanInitializationTests();
    return 0;
}
