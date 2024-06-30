#include "vulkanwindow.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace SlimeEngine
{

// Global variables to store the last known window size
int lastWidth  = -1;
int lastHeight = -1;

// Callback function for window size change
void windowSizeCallback(GLFWwindow* window, int width, int height)
{
	// Check if the window has been resized
	if (lastWidth != width || lastHeight != height)
	{
		// Window has been resized
		spdlog::info("Window resized to {}x{}", width, height);

		// Update the last known window size
		lastWidth  = width;
		lastHeight = height;
	}
}

GLFWwindow* CreateWindow(const char* window_name, bool resize)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	if (!resize)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	return glfwCreateWindow(1024, 1024, window_name, nullptr, nullptr);
}

void DestroyWindow(GLFWwindow* window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

VkSurfaceKHR CreateSurface(const VkInstance& instance, GLFWwindow* window, const VkAllocationCallbacks* allocator)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult err         = glfwCreateWindowSurface(instance, window, allocator, &surface);
	if (err)
	{
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

	// Register the window size callback
	glfwSetWindowSizeCallback(window, windowSizeCallback);

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

bool ShouldRecreateSwapchain(GLFWwindow* window)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return width == 0 || height == 0;
}

bool WindowSuspended(GLFWwindow* window)
{
	return glfwGetWindowAttrib(window, GLFW_ICONIFIED) || !glfwGetWindowAttrib(window, GLFW_VISIBLE);
}
}