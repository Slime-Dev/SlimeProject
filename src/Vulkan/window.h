#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace SlimeEngine
{
	GLFWwindow* CreateWindow(const char* window_name = "", bool resize = true);
	void DestroyWindow(GLFWwindow* window);
	VkSurfaceKHR CreateSurface(const VkInstance& instance, GLFWwindow* window, const VkAllocationCallbacks* allocator = nullptr);
	void PollEvents();
	bool ShouldClose(GLFWwindow* window);
	bool ShouldRecreateSwapchain(GLFWwindow* window);
}