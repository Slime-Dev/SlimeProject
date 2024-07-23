#pragma once

class Component
{
public:
	virtual ~Component() = default;

	// Imgui debug
	virtual void ImGuiDebug() = 0;
};