#pragma once

#include "scene.h"
#include "spdlog/spdlog.h"
#include "PipelineGenerator.h"
#include "ModelManager.h"
#include <glm/gtx/rotate_vector.hpp>

class PlatformerGame : public Scene {
public:
    PlatformerGame(Engine& engine) : Scene(engine) {}

    int Setup() override;

	void Update(float dt, const InputManager* inputManager) override;

private:
    void ResetGame();

	ModelManager::ModelResource* m_ground;
    ModelManager::ModelResource* m_player;
    ModelManager::ModelResource* m_obstacle;

    glm::vec3 m_playerPosition;
    glm::vec3 m_playerVelocity;
    bool m_isJumping;
    bool m_gameOver;
};