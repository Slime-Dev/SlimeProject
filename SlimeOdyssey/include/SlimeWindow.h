#pragma once
#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <string>
#include <chrono>
#include <functional>
#include <thread>

class SlimeWindow {
public:
	struct WindowProps {
		std::string title;
		int width;
		int height;
		bool resizable;
		bool decorated;
		bool fullscreen;
	};

	SlimeWindow(const WindowProps& props);
	~SlimeWindow();

	bool ShouldClose() const;
	void Close() { m_closeNow = true; }

	float Update();

	GLFWwindow* GetGLFWWindow() const { return m_Window; }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }

	void SetFullscreen(bool fullscreen);
	void SetTitle(const std::string& title);
	void SetCursorMode(int mode) { glfwSetInputMode(m_Window, GLFW_CURSOR, mode); }

	void SetResizeCallback(std::function<void(int, int)> callback) { m_ResizeCallback = std::move(callback); }

	InputManager* GetInputManager() { return &m_InputManager; }

	SlimeWindow(const SlimeWindow&) = delete;
	SlimeWindow& operator=(const SlimeWindow&) = delete;
	SlimeWindow(SlimeWindow&&) = delete;
	SlimeWindow& operator=(SlimeWindow&&) = delete;

private:
	GLFWwindow* m_Window = nullptr;
	int m_Width;
	int m_Height;
	bool m_closeNow = false;
	WindowProps m_Props;
	std::chrono::steady_clock::time_point m_LastFrameTime;
	std::function<void(int, int)> m_ResizeCallback;
	InputManager m_InputManager;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	GLFWmonitor* GetPrimaryMonitor() const;
};