#include "InputManager.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "SlimeWindow.h"

InputManager::InputManager(GLFWwindow* window)
      : m_window(window)
{
	// Initialize key states
	for (int i = 0; i < GLFW_KEY_LAST; ++i)
	{
		m_keyStates[i] = KeyState::Released;
	}

	// Initialize mouse button states
	for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; ++i)
	{
		m_mouseButtonStates[i] = KeyState::Released;
	}

	// Set up GLFW callbacks
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetScrollCallback(window, ScrollCallback);
}

void InputManager::Update()
{
	// Update key states
	for (int i = 0; i < GLFW_KEY_LAST; ++i)
	{
		int state = glfwGetKey(m_window, i);
		if (state == GLFW_PRESS)
		{
			if (m_keyStates[i] == KeyState::Released || m_keyStates[i] == KeyState::JustReleased)
			{
				m_keyStates[i] = KeyState::JustPressed;
			}
			else
			{
				m_keyStates[i] = KeyState::Pressed;
			}
		}
		else
		{
			if (m_keyStates[i] == KeyState::Pressed || m_keyStates[i] == KeyState::JustPressed)
			{
				m_keyStates[i] = KeyState::JustReleased;
			}
			else
			{
				m_keyStates[i] = KeyState::Released;
			}
		}
	}

	// Update mouse button states
	for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; ++i)
	{
		int state = glfwGetMouseButton(m_window, i);
		if (state == GLFW_PRESS)
		{
			if (m_mouseButtonStates[i] == KeyState::Released || m_mouseButtonStates[i] == KeyState::JustReleased)
			{
				m_mouseButtonStates[i] = KeyState::JustPressed;
			}
			else
			{
				m_mouseButtonStates[i] = KeyState::Pressed;
			}
		}
		else
		{
			if (m_mouseButtonStates[i] == KeyState::Pressed || m_mouseButtonStates[i] == KeyState::JustPressed)
			{
				m_mouseButtonStates[i] = KeyState::JustReleased;
			}
			else
			{
				m_mouseButtonStates[i] = KeyState::Released;
			}
		}
	}

	// Calculate mouse delta
	double currentMouseX, currentMouseY;
	glfwGetCursorPos(m_window, &currentMouseX, &currentMouseY);
	m_mouseDeltaX = currentMouseX - m_lastMouseX;
	m_mouseDeltaY = currentMouseY - m_lastMouseY;
	m_lastMouseX = currentMouseX;
	m_lastMouseY = currentMouseY;

	m_scrollDelta = 0.0;
}

bool InputManager::IsKeyPressed(int key) const
{
	return m_keyStates[key] == KeyState::Pressed || m_keyStates[key] == KeyState::JustPressed;
}

bool InputManager::IsKeyJustPressed(int key) const
{
	return m_keyStates[key] == KeyState::JustPressed;
}

bool InputManager::IsKeyJustReleased(int key) const
{
	return m_keyStates[key] == KeyState::JustReleased;
}

bool InputManager::IsMouseButtonPressed(int button) const
{
	return m_mouseButtonStates[button] == KeyState::Pressed || m_mouseButtonStates[button] == KeyState::JustPressed;
}

bool InputManager::IsMouseButtonJustPressed(int button) const
{
	return m_mouseButtonStates[button] == KeyState::JustPressed;
}

bool InputManager::IsMouseButtonJustReleased(int button) const
{
	return m_mouseButtonStates[button] == KeyState::JustReleased;
}

std::pair<double, double> InputManager::GetMousePosition() const
{
	double x, y;
	glfwGetCursorPos(m_window, &x, &y);
	return { x, y };
}

std::pair<double, double> InputManager::GetMouseDelta() const
{
	return { m_mouseDeltaX, m_mouseDeltaY };
}

double InputManager::GetScrollDelta() const
{
	return m_scrollDelta;
}

void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	InputManager* inputManager = static_cast<SlimeWindow*>(glfwGetWindowUserPointer(window))->GetInputManager();
	if (inputManager)
	{
		// Check if the key is within valid range
		if (key >= 0 && key < GLFW_KEY_LAST)
		{
			if (action == GLFW_PRESS)
			{
				inputManager->m_keyStates[key] = KeyState::JustPressed;
			}
			else if (action == GLFW_RELEASE)
			{
				inputManager->m_keyStates[key] = KeyState::JustReleased;
			}
		}
		else
		{
			// Log an error or handle the invalid key in some way
			spdlog::warn("Invalid key code received: {}", key);
		}
	}
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	InputManager* inputManager = static_cast<SlimeWindow*>(glfwGetWindowUserPointer(window))->GetInputManager();
	if (inputManager)
	{
		if (action == GLFW_PRESS)
		{
			inputManager->m_mouseButtonStates[button] = KeyState::JustPressed;
		}
		else if (action == GLFW_RELEASE)
		{
			inputManager->m_mouseButtonStates[button] = KeyState::JustReleased;
		}
	}
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	InputManager* inputManager = static_cast<SlimeWindow*>(glfwGetWindowUserPointer(window))->GetInputManager();
	if (inputManager)
	{
		inputManager->m_scrollDelta = yoffset;
	}
}
