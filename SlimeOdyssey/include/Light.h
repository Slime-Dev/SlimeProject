#pragma once
#include <glm/fwd.hpp>
#include "Component.h"

struct PointLight
{
	glm::vec3 pos = glm::vec3(-6.0f, 6.0f, 6.0f);
	glm::vec3 colour = glm::vec3(1.0f, 0.8f, 0.95f);
	glm::vec3 view = glm::vec3(0.0f);
	float ambientStrength = 0.0f;
	float specularStrength = 0.0f;
	float shininess = 0.0f;
};

struct PointLightObject : public Component
{
	PointLight light;

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;

	void ImGuiDebug();
};

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 lightColor;
};

struct DirectionalLightObject : public Component
{
	DirectionalLight light;

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;

	void ImGuiDebug();
};