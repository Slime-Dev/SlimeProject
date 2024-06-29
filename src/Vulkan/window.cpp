#include "vulkan/window.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace SlimeEngine {
	GLFWwindow* CreateWindow(const char* window_name, bool resize)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		if (!resize) glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		return glfwCreateWindow(1024, 1024, window_name, nullptr, nullptr);
	}

	void DestroyWindow(GLFWwindow* window) {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	VkSurfaceKHR CreateSurface(const VkInstance& instance, GLFWwindow* window, const VkAllocationCallbacks* allocator)
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkResult err = glfwCreateWindowSurface(instance, window, allocator, &surface);
		if (err) {
			const char* error_msg;
			int ret = glfwGetError(&error_msg);
			if (ret != 0)
			{
				spdlog::error("GLFW error: {}", error_msg);
			}
			else
			{
				spdlog::error("Failed to create window surface");
			}
			surface = VK_NULL_HANDLE;
		}
		return surface;
	}

	void PollEvents()
	{
		glfwPollEvents();
	}

	bool ShouldClose(GLFWwindow* window)
	{
		return glfwWindowShouldClose(window);
	}

	bool ShouldRecreateSwapchain(GLFWwindow* window) {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		return width == 0 || height == 0;
	}
}