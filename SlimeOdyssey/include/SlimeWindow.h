#pragma once

#include <utility>
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
	bool MouseMoved();

	std::pair<float, float> GetMousePos() { return { m_mouseX, m_mouseY }; }

	GLFWwindow* GetWindow() { return m_window; }
private:
	GLFWwindow* m_window = nullptr;

	// Mouse position
	double m_mouseX = 0.0;
	double m_mouseY = 0.0;
};