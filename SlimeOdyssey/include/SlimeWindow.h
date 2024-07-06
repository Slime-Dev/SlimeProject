#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

class Window
{
public:
	Window(const char* name, int width, int height, bool resizable = false);
	~Window();

	VkSurfaceKHR CreateSurface(const VkInstance& instance, const VkAllocationCallbacks* allocator = nullptr);
	void PollEvents();
	bool ShouldClose();
	bool ShouldRecreateSwapchain();
	bool WindowSuspended();

	GLFWwindow* GetWindow() { return m_window; }
private:
	GLFWwindow* m_window = nullptr;
};