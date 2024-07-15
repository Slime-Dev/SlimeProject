#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include "scene.h"

class PlatformerGame : public Scene
{
public:
	PlatformerGame(Engine& engine)
	      : Scene(engine)
	{
	}

	int Setup(ModelManager& modelManager , ShaderManager& shaderManager, DescriptorManager& descriptorManager) override;

	void Update(float dt, const InputManager* inputManager) override;
	void UpdateCamera(const InputManager* inputManager, float deltaTime);

private:
	void ResetGame();

	ModelResource* m_ground;
	ModelResource* m_player;
	ModelResource* m_obstacle;

	glm::vec3 m_playerPosition;
	glm::vec3 m_playerVelocity;

	float m_playerRotation = 0.0f;
	float m_cameraDistance = 0.0f;
	float m_cameraHeight = 0.0f;
	float m_cameraYaw;
	float m_cameraPitch;
	glm::vec3 m_cameraPosition;

	bool m_isJumping;
	bool m_gameOver;
};
