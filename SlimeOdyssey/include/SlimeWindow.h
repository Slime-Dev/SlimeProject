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

	float GetDeltaTime();

	std::pair<float, float> GetMousePos() { return { m_mouseX, m_mouseY }; }
	std::pair<float, float> GetMouseDelta();
	std::pair<int, int> GetWindowSize();

	void LockMouse(bool lock);

	GLFWwindow* GetWindow() { return m_window; }
private:
	GLFWwindow* m_window = nullptr;

	// Time
	float m_lastFrame = 0.0f;
	float m_deltaTime = 0.0f;

	// Mouse position
	double m_mouseX = 0.0;
	double m_mouseY = 0.0;
	double m_mouseDeltaX = 0.0;
	double m_mouseDeltaY = 0.0;
};