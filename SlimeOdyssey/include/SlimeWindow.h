#pragma once
#include <chrono>
#include <deque>
#include <functional>
#include <GLFW/glfw3.h>
#include <string>
#include <thread>

#include "InputManager.h"

class SlimeWindow
{
public:
	struct WindowProps
	{
		std::string title;
		int width;
		int height;
		bool resizable;
		bool decorated;
		bool fullscreen;
	};

	SlimeWindow(const WindowProps& props);
	~SlimeWindow();

	float Update();

	bool ShouldClose() const;
	void Close();

	GLFWwindow* GetGLFWWindow() const;

	int GetWidth() const;
	int GetHeight() const;

	void SetFullscreen(bool fullscreen);
	void SetTitle(const std::string& title);

	void SetCursorMode(int mode);
	void SetResizeCallback(std::function<void(int, int)> callback);

	bool WindowSuspended();

	InputManager* GetInputManager();

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

	std::deque<float> m_fpsHistory;
	const size_t m_maxFpsSamples = 60;
	const float m_fpsUpdateInterval = 0.5f;
	float m_timeSinceLastFpsUpdate = 0.0f;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	GLFWmonitor* GetPrimaryMonitor() const;
};
