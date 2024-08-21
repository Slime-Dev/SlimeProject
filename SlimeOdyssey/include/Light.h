#pragma once

#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

constexpr float PADDING = 420.0f;

enum class LightType
{
	Undefined,
	Directional,
	Point
};

// Base Light struct
struct LightData
{
	// Split by 16 byte chunks
	glm::vec3 color = glm::vec3(1.0f);
	float padding1 = PADDING;

	float ambientStrength = 0.0f;
	float specularStrength = 0.0f;
	float padding2[2] = { PADDING, PADDING };
	
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
};

// Base Light class
class Light
{
public:
	virtual ~Light() = default;

	LightType GetType() const
	{
		return m_lightType;
	}

	void SetLightSpaceMatrix(glm::mat4 lightSpaceMatric)
	{
		m_data.lightSpaceMatrix = lightSpaceMatric;
	}

	glm::mat4 GetLightSpaceMatrix() const
	{
		return m_data.lightSpaceMatrix;
	}

	glm::vec3 GetColor() const
	{
		return m_data.color;
	}

	void SetColor(glm::vec3 color)
	{
		m_data.color = color;
	}

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;

protected:
	LightType m_lightType = LightType::Undefined;
	LightData m_data;
};

// Directional Light
class DirectionalLight : public Light
{
public:
	DirectionalLight(glm::vec3 dir);

	glm::vec3 GetDirection() const;
	void SetDirection(const glm::vec3& direction);

	const LightData& GetData() const;
	void SetData(const LightData& data);

	// Get the binding data
	struct BindingData
	{
		LightData data;
		glm::vec3 direction;
		float padding1 = PADDING;
	};
	BindingData GetBindingData() const;
	size_t GetBindingDataSize() const;

	void ImGuiDebug();

private:
	glm::vec3 m_direction = glm::normalize(glm::vec3(-20.0f, 15.0f, 20.0f));
	float m_padding3 = PADDING;
};

// Point Light
class PointLight : public Light
{
public:
	PointLight();

	glm::vec3 GetPosition() const;
	void SetPosition(const glm::vec3& position);

	float GetRadius() const;
	void SetRadius(float radius);

	const LightData& GetData() const;
	void SetData(const LightData& data);

	// Get the binding data
	struct BindingData
	{
		LightData data;
		glm::vec3 position;
		float radius;
	};
	BindingData GetBindingData() const;
	size_t GetBindingDataSize() const;

	void ImGuiDebug();
private:
	glm::vec3 m_position = glm::vec3(-6.0f, 6.0f, 6.0f);
	float m_radius = 50.0f; // Light's influence radius
};
