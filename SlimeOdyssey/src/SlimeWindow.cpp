#include "SlimeWindow.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

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

Window::Window(const char* name, int width, int height, bool resizable)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	if (!resizable)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(width, height, name, nullptr, nullptr);
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

VkSurfaceKHR Window::CreateSurface(const VkInstance& instance, const VkAllocationCallbacks* allocator)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult err         = glfwCreateWindowSurface(instance, m_window, allocator, &surface);
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
	glfwSetWindowSizeCallback(m_window, windowSizeCallback);

	return surface;
}

void Window::PollEvents()
{
	glfwPollEvents();
}

bool Window::ShouldClose()
{
	// Check for escape key press
	if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		return true;
	}

	return glfwWindowShouldClose(m_window);
}

bool Window::ShouldRecreateSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	return width == 0 || height == 0;
}

bool Window::WindowSuspended()
{
	return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) || !glfwGetWindowAttrib(m_window, GLFW_VISIBLE);
}

bool Window::MouseMoved()
{
	// Get the current mouse position
	double x, y;
	glfwGetCursorPos(m_window, &x, &y);

	// Check if the mouse has moved
	if (x != m_mouseX || y != m_mouseY)
	{
		// Update the last known mouse position
		m_mouseX = x;
		m_mouseY = y;

		return true;
	}

	return false;
}
