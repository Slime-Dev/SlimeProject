#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include "Model.h"
#include "Scene.h"

class Engine;
class SlimeWindow;

class PlatformerGame : public Scene
{
public:
	PlatformerGame(SlimeWindow* window);

	int Enter(Engine& engine, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager) override;

	void Update(float dt, Engine& engine, const InputManager* inputManager) override;
	void UpdateCamera(Engine& engine, const InputManager* inputManager, float deltaTime);

	void Render(Engine& engine, ModelManager& modelManager) override;
	void Exit(Engine& engine, ModelManager& modelManager) override;

private:
	void CreateDebugMaterials(Engine& engine, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager);
	Material tempMaterial;

	void ResetGame();

	Model m_player;
	Model m_obstacle;

	glm::vec3 m_playerPosition;
	glm::vec3 m_playerVelocity;

	float m_playerRotation = 0.0f;
	float m_cameraDistance = 10.0f;
	float m_cameraYaw = 0.0f;
	float m_cameraPitch = 0.0f;
	glm::vec3 m_cameraPosition;

	bool m_isJumping;
	bool m_gameOver;

	SlimeWindow* m_window = nullptr;
};
