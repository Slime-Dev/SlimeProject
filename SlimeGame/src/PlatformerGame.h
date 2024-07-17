#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include "Model.h"
#include "Scene.h"

class Engine;
class SlimeWindow;
class InputManager;
class ModelManager;
class ShaderManager;
class DescriptorManager;

class PlatformerGame : public Scene
{
public:
	PlatformerGame(SlimeWindow* window);

	int Enter(Engine& engine, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager) override;
	void Update(float dt, Engine& engine, const InputManager* inputManager) override;
	void Render(Engine& engine, ModelManager& modelManager) override;
	void Exit(Engine& engine, ModelManager& modelManager) override;

private:
	// Initialization methods
	void InitializeGameObjects(Engine& engine, ModelManager& modelManager, Material* material);
	void SetupShaders(Engine& engine, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager);

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
