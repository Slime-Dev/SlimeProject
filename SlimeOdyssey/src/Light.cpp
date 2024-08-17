#include "Light.h"

#include "imgui.h"

void PointLight::ImGuiDebug()
{
	ImGui::Text("Point Light");

	glm::vec3 position = GetPosition();
	if (ImGui::DragFloat3("Position", &position.x, 0.1f))
	{
		SetPosition(position);
	}

	LightData data = GetData();

	if (ImGui::ColorEdit3("Colour", &data.color.x))
	{
		SetData(data);
	}

	if (ImGui::DragFloat("Ambient Strength", &data.ambientStrength, 0.01f, 0.0f, 1.0f))
	{
		SetData(data);
	}

	if (ImGui::DragFloat("Specular Strength", &data.specularStrength, 0.01f, 0.0f, 1.0f))
	{
		SetData(data);
	}

	float radius = GetRadius();
	if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 1000.0f))
	{
		SetRadius(radius);
	}

	ImGui::Spacing();
	ImGui::Text("Light Space Matrix");
	for (int i = 0; i < 4; i++)
	{
		ImGui::Text("%f %f %f %f", data.lightSpaceMatrix[i][0], data.lightSpaceMatrix[i][1], data.lightSpaceMatrix[i][2], data.lightSpaceMatrix[i][3]);
	}
}

void DirectionalLight::ImGuiDebug()
{
	ImGui::Text("Directional Light");

	glm::vec3 direction = GetDirection();
	if (ImGui::DragFloat3("Direction", &direction.x, 0.01f))
	{
		SetDirection(glm::normalize(direction));
	}

	LightData data = GetData();

	if (ImGui::ColorEdit3("Colour", &data.color.x))
	{
		SetData(data);
	}

	if (ImGui::DragFloat("Ambient Strength", &data.ambientStrength, 0.01f, 0.0f, 1.0f))
	{
		SetData(data);
	}

	ImGui::Spacing();
	ImGui::Text("Light Space Matrix");
	for (int i = 0; i < 4; i++)
	{
		ImGui::Text("%f %f %f %f", data.lightSpaceMatrix[i][0], data.lightSpaceMatrix[i][1], data.lightSpaceMatrix[i][2], data.lightSpaceMatrix[i][3]);
	}
}

DirectionalLight::DirectionalLight(glm::vec3 dir)
	: m_direction(dir)
{
	m_lightType = LightType::Directional;
	m_data.ambientStrength = 0.075f;
}

glm::vec3 DirectionalLight::GetDirection() const
{
	return m_direction;
}

void DirectionalLight::SetDirection(const glm::vec3& direction)
{
	m_direction = direction;
}

const LightData& DirectionalLight::GetData() const
{
	return m_data;
}

void DirectionalLight::SetData(const LightData& data)
{
	m_data = data;
}

DirectionalLight::BindingData DirectionalLight::GetBindingData() const
{
	return { m_data, m_direction, m_padding3 };
}

size_t DirectionalLight::GetBindingDataSize() const
{
	return sizeof(BindingData);
}

PointLight::PointLight()
{
	m_lightType = LightType::Point;
}

glm::vec3 PointLight::GetPosition() const
{
	return m_position;
}

void PointLight::SetPosition(const glm::vec3& position)
{
	m_position = position;
}

float PointLight::GetRadius() const
{
	return m_radius;
}

void PointLight::SetRadius(float radius)
{
	m_radius = radius;
}

const LightData& PointLight::GetData() const
{
	return m_data;
}

void PointLight::SetData(const LightData& data)
{
	m_data = data;
}

PointLight::BindingData PointLight::GetBindingData() const
{
	return { m_data, m_position, m_radius };
}

size_t PointLight::GetBindingDataSize() const
{
	return sizeof(BindingData);
}
