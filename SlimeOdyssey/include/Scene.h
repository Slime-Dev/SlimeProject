#pragma once
#include "Light.h"
#include "Camera.h"
#include <array>

constexpr int MAX_POINT_LIGHTS = 10;

class ModelManager;
class ShaderManager;
class DescriptorManager;
class InputManager;
class Engine;

class Scene
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual int Enter(Engine& engine, ModelManager& modelManager , ShaderManager& shaderManager, DescriptorManager& descriptorManager) = 0;
	virtual void Update(float dt, Engine& engine, const InputManager* inputManager) = 0;
	virtual void Render(Engine& engine, ModelManager& modelManager) = 0;

	virtual void Exit(Engine& engine, ModelManager& modelManager) = 0;

	std::array<PointLightObject, MAX_POINT_LIGHTS>& GetPointLights() { return m_pointLights; }
	const DirectionalLightObject& GetDirectionalLight() const { return m_directionalLight; }

	[[nodiscard]] Camera& GetCamera() { return m_camera; }

protected:
	std::array<PointLightObject, MAX_POINT_LIGHTS> m_pointLights;
	DirectionalLightObject m_directionalLight;

	Camera m_camera = Camera(90.0f, 800.0f / 600.0f, 0.001f, 100.0f);
};
