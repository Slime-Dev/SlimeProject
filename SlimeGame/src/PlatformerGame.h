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
	void InitializeGameObjects(VulkanContext& vulkanContext, ModelManager& modelManager, Material* material);
	void SetupShaders(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager);

	// Update methods
	void UpdatePlayer(float dt, const InputManager* inputManager);
	void UpdateCamera(float dt, const InputManager* inputManager);
	void CheckCollisions();
	void CheckWinCondition();

	// Helper methods
	void ResetGame();

	// Game objects
	Model m_player;
	Model m_obstacle;
	Model m_ground;
	Model m_lightCube;

	// Game state
	struct GameState
	{
		glm::vec3 playerPosition;
		glm::vec3 playerVelocity;
		float playerRotation;
		bool isJumping;
		bool gameOver;
	} m_gameState;

	// Camera state
	struct CameraState
	{
		float distance;
		float yaw;
		float pitch;
		glm::vec3 position;
	};
	CameraState m_cameraState;

    bool m_cameraMouseControl = false;
    float m_manualYaw = 0.0f;
    float m_manualPitch = 10.0f;
    float m_manualDistance = 10.0f;

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

	Material m_tempMaterial;
	SlimeWindow* m_window;
};
