#pragma once
#include "EntityManager.h"
#include "Entity.h"

class ModelManager;
class ShaderManager;
class DescriptorManager;
class InputManager;
class VulkanContext;

class Scene
{
public:
	virtual ~Scene() = default;

	virtual int Enter(VulkanContext& vulkanContext, ModelManager& modelManager , ShaderManager& shaderManager, DescriptorManager& descriptorManager) = 0;
	virtual void Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager) = 0;
	virtual void Render(VulkanContext& vulkanContext, ModelManager& modelManager) = 0;

	virtual void Exit(VulkanContext& vulkanContext, ModelManager& modelManager) = 0;

	EntityManager m_entityManager;
};
