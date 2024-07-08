#pragma once
#include <GLFW/glfw3.h>
#include <array>

class SlimeWindow;

class InputManager
{
public:
	InputManager() = default;
	explicit InputManager(GLFWwindow* window);

	void Update();

	bool IsKeyPressed(int key) const;
	bool IsKeyJustPressed(int key) const;
	bool IsKeyJustReleased(int key) const;

	bool IsMouseButtonPressed(int button) const;
	bool IsMouseButtonJustPressed(int button) const;
	bool IsMouseButtonJustReleased(int button) const;

	std::pair<double, double> GetMousePosition() const;
	std::pair<double, double> GetMouseDelta() const;
	double GetScrollDelta() const;

private:
	enum class KeyState {
		Released,
		JustPressed,
		Pressed,
		JustReleased
	};

	GLFWwindow* m_window = nullptr;
	std::array<KeyState, GLFW_KEY_LAST> m_keyStates;
	std::array<KeyState, GLFW_MOUSE_BUTTON_LAST> m_mouseButtonStates;

	double m_lastMouseX = 0.0;
	double m_lastMouseY = 0.0;
	double m_mouseDeltaX = 0.0;
	double m_mouseDeltaY = 0.0;
	double m_scrollDelta = 0.0;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};