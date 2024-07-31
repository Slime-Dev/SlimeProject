#pragma once
#include <glm/fwd.hpp>
#include "Component.h"
#include <vk_mem_alloc.h>

struct PointLight
{
	glm::vec3 pos = glm::vec3(-6.0f, 6.0f, 6.0f);
	float ambientStrength = 0.0f;
	glm::vec3 colour = glm::vec3(1.0f, 0.8f, 0.95f);
	float specularStrength = 0.0f;
	glm::vec3 direction = glm::vec3(0.0f);
	float shininess = 0.0f;
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
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
	glm::vec3 direction = glm::vec3(-20.0f, 15.0f, 20.0f);
	float ambientStrength = 0.01f;
	glm::vec3 color = glm::vec3(1.0f);
	float padding;
	glm::mat4 lightSpaceMatrix = glm::mat4();
};

struct DirectionalLightObject : public Component
{
	DirectionalLight light;

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;

	void ImGuiDebug();
};