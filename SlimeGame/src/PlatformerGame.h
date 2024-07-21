#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

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

	int Enter(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager) override;
	void Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager) override;
	void Render(VulkanContext& vulkanContext, ModelManager& modelManager) override;
	void Exit(VulkanContext& vulkanContext, ModelManager& modelManager) override;

private:
	// Initialization methods
	void InitializeGameObjects(VulkanContext& vulkanContext, ModelManager& modelManager);
	void SetupShaders(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager);

	// Update methods
	void UpdatePlayer(float dt, const InputManager* inputManager);
	void UpdateCamera(float dt, const InputManager* inputManager);
	void CheckCollisions();
	void CheckWinCondition();

	// Helper methods
	void ResetGame();

	// Game objects
	std::shared_ptr<Entity> m_player = std::make_shared<Entity>("Player");
	std::shared_ptr<Entity> m_obstacle = std::make_shared<Entity>("Obstacle");
	std::shared_ptr<Entity> m_ground = std::make_shared<Entity>("Ground");
	std::shared_ptr<Entity> m_lightCube = std::make_shared<Entity>("LightCube");

	// Camera state
	struct CameraState
	{
		float distance;
		float yaw;
		float pitch;
		glm::vec3 position;
	};
	CameraState m_cameraState;

    bool m_cameraMouseControl = true;
    float m_manualYaw = 0.0f;
    float m_manualPitch = 10.0f;
    float m_manualDistance = 10.0f;

	bool m_gameOver = false;

	// Game parameters
	struct GameParameters
	{
		float moveSpeed;
		float rotationSpeed;
		float jumpForce;
		float gravity;
		float dragCoefficient;
		float frictionCoefficient;
	};
	GameParameters m_gameParams;

	std::shared_ptr<MaterialResource> m_tempMaterial;
	SlimeWindow* m_window;
};
