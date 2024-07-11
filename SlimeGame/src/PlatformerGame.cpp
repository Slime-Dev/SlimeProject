//
// Created by alexm on 11/07/24.
//

#include "PlatformerGame.h"

int PlatformerGame::Setup()
{
	// Call the base class Setup
	if (Scene::Setup() != 0)
	{
		return -1;
	}

	// Modify the existing models for our game
	m_ground = m_cube; // Use the cube as our ground
	m_ground->model = glm::scale(m_ground->model, glm::vec3(20.0f, 0.5f, 5.5f));
	m_ground->model = glm::translate(m_ground->model, glm::vec3(0.0f, -2.0f, 0.0f));

	m_player = m_bunny; // Use the bunny as our player
	m_player->model = glm::scale(m_player->model, glm::vec3(2.0f));
	m_player->model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

	m_obstacle = m_suzanne; // Use Suzanne as our obstacle
	m_obstacle->model = glm::scale(m_obstacle->model, glm::vec3(0.25f));
	m_obstacle->model = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 1.0f, 0.0f));

	// Initialize game state
	m_playerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	m_playerVelocity = glm::vec3(0.0f);
	m_isJumping = false;
	m_gameOver = false;

	// Set up camera
	m_camera.SetPosition(glm::vec3(0.0f, 3.0f, 5.0f));
	m_camera.SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));

	return 0;
}

void PlatformerGame::Update(float dt, const InputManager* inputManager)
{
	if (m_gameOver)
	{
		if (inputManager->IsKeyJustPressed(GLFW_KEY_R))
		{
			ResetGame();
		}
		return;
	}

	// Player movement
	float moveSpeed = 5.0f;
	if (inputManager->IsKeyPressed(GLFW_KEY_A))
	{
		m_playerVelocity.x = -moveSpeed;
	}
	else if (inputManager->IsKeyPressed(GLFW_KEY_D))
	{
		m_playerVelocity.x = moveSpeed;
	}
	else
	{
		m_playerVelocity.x = 0.0f;
	}

	// Jumping
	if (inputManager->IsKeyJustPressed(GLFW_KEY_SPACE) && !m_isJumping)
	{
		m_playerVelocity.y = 8.0f;
		m_isJumping = true;
	}

	// Apply gravity
	m_playerVelocity.y -= 9.8f * dt;

	// Update position
	m_playerPosition += m_playerVelocity * dt;

	// Ground collision
	if (m_playerPosition.y < 0.0f)
	{
		m_playerPosition.y = 0.0f;
		m_playerVelocity.y = 0.0f;
		m_isJumping = false;
	}

	// Update player model
	m_player->model = glm::translate(glm::mat4(1.0f), m_playerPosition);
	m_player->model = glm::scale(m_player->model, glm::vec3(8.0f));

	// Check for game over (falling off the platform)
	if (std::abs(m_playerPosition.x) > 20.0f || std::abs(m_playerPosition.z) > 10.0f)
	{
		m_gameOver = true;
		spdlog::info("Game Over! Press R to restart.");
	}

	// Check for win condition (reaching the cube)
	if (glm::distance(m_playerPosition, glm::vec3(5.0f, 0.0f, 0.0f)) < 1.0f)
	{
		spdlog::info("You win! Press R to play again.");
		m_gameOver = true;
	}

	// Update camera to follow player
	m_camera.SetPosition(glm::vec3(m_playerPosition.x, 2.0f, 4.0f));
	m_camera.SetTarget(m_playerPosition);

	// Rotate obstacle
	m_obstacle->model = glm::rotate(m_obstacle->model, dt, glm::vec3(0.0f, 1.0f, 0.0f));

	// Handle escape key
	if (inputManager->IsKeyJustPressed(GLFW_KEY_ESCAPE))
	{
		m_window->Close();
	}
}

void PlatformerGame::ResetGame()
{
	m_playerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	m_playerVelocity = glm::vec3(0.0f);
	m_isJumping = false;
	m_gameOver = false;
}
