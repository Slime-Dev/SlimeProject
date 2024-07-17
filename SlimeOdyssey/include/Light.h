#pragma once

struct PointLight
{
	glm::vec3 pos;
	glm::vec3 colour;
	glm::vec3 view;
	float ambientStrength; // This now aligns correctly with the GPU struct
	float specularStrength;
	float shininess;
};

struct PointLightObject
{
	PointLight light;

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
};

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 lightColor;
};

struct DirectionalLightObject
{
	DirectionalLight light;

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
};