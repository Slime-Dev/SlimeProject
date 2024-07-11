#include "SlimeWindow.h"

#include <numeric>
#include <sstream>

#include "spdlog/spdlog.h"

SlimeWindow::SlimeWindow(const WindowProps& props)
      : m_Width(props.width), m_Height(props.height), m_Props(props)
{
	if (!glfwInit())
	{
		throw std::runtime_error("Failed to initialize GLFW");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, props.resizable ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_DECORATED, props.decorated ? GLFW_TRUE : GLFW_FALSE);

	GLFWmonitor* monitor = props.fullscreen ? GetPrimaryMonitor() : nullptr;

	m_Window = glfwCreateWindow(props.width, props.height, props.title.c_str(), monitor, nullptr);

	if (!m_Window)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}

	glfwSetWindowUserPointer(m_Window, this);
	glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);

	m_LastFrameTime = std::chrono::steady_clock::now();
	m_InputManager = InputManager(m_Window);
}

SlimeWindow::~SlimeWindow()
{
	if (m_Window)
	{
		glfwDestroyWindow(m_Window);
	}
	glfwTerminate();
}

bool SlimeWindow::ShouldClose() const
{
	return glfwWindowShouldClose(m_Window) || m_closeNow;
}

void SlimeWindow::Close()
{
	m_closeNow = true;
}

GLFWwindow* SlimeWindow::GetGLFWWindow() const
{
	return m_Window;
}

int SlimeWindow::GetWidth() const
{
	return m_Width;
}

int SlimeWindow::GetHeight() const
{
	return m_Height;
}

void SlimeWindow::SetCursorMode(int mode)
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, mode);
}

void SlimeWindow::SetResizeCallback(std::function<void(int, int)> callback)
{
	m_ResizeCallback = std::move(callback);
}

InputManager* SlimeWindow::GetInputManager()
{
	return &m_InputManager;
}

float SlimeWindow::Update()
{
	m_InputManager.Update();

	glfwPollEvents();

	auto currentTime = std::chrono::steady_clock::now();
	float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_LastFrameTime).count();
	m_LastFrameTime = currentTime;

	// FPS calculation and update
	float fps = 1.0f / dt;
	m_fpsHistory.push_back(fps);
	if (m_fpsHistory.size() > m_maxFpsSamples)
	{
		m_fpsHistory.pop_front();
	}

	m_timeSinceLastFpsUpdate += dt;
	if (m_timeSinceLastFpsUpdate >= m_fpsUpdateInterval)
	{
		m_timeSinceLastFpsUpdate = 0.0f;

		float averageFps = std::accumulate(m_fpsHistory.begin(), m_fpsHistory.end(), 0.0f) / m_fpsHistory.size();

		std::stringstream stream;
		stream << std::fixed << averageFps << " FPS ";

		// Add more detailed information
		float minFps = *std::min_element(m_fpsHistory.begin(), m_fpsHistory.end());
		float maxFps = *std::max_element(m_fpsHistory.begin(), m_fpsHistory.end());

		stream << " | Min: " << std::fixed << minFps << " | Max: " << std::fixed << maxFps;

		// Add to window title
		SetTitle(stream.str());
	}

	return dt;
}

void SlimeWindow::SetFullscreen(bool fullscreen)
{
	if (fullscreen == m_Props.fullscreen)
		return;

	m_Props.fullscreen = fullscreen;
	if (fullscreen)
	{
		GLFWmonitor* monitor = GetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	else
	{
		glfwSetWindowMonitor(m_Window, nullptr, 100, 100, m_Props.width, m_Props.height, 0);
	}
}

void SlimeWindow::SetTitle(const std::string& title)
{
	m_Props.title = title;
	glfwSetWindowTitle(m_Window, title.c_str());
}

void SlimeWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto* windowInstance = static_cast<SlimeWindow*>(glfwGetWindowUserPointer(window));
	windowInstance->m_Width = width;
	windowInstance->m_Height = height;
	if (windowInstance->m_ResizeCallback)
	{
		windowInstance->m_ResizeCallback(width, height);
	}
}

GLFWmonitor* SlimeWindow::GetPrimaryMonitor() const
{
	int monitorCount;
	GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
	return monitorCount > 0 ? monitors[0] : nullptr;
}
