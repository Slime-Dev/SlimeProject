#pragma once
#include <glm/gtx/rotate_vector.hpp>
#include <memory>
#include <vector>

#include "Model.h"
#include "Scene.h"

class VulkanContext;
class SlimeWindow;
class InputManager;
class ModelManager;
class ShaderManager;
class DescriptorManager;

class PlatformerGame : public Scene
{
public:
	PlatformerGame(SlimeWindow* window);
	int Enter(VulkanContext& vulkanContext, ModelManager& modelManager) override;
	void Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager) override;
	void Render() override;
	void Exit(VulkanContext& vulkanContext, ModelManager& modelManager) override;
};
