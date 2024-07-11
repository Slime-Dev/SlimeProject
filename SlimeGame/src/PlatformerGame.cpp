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
	m_ground->model = glm::scale(m_ground->model, glm::vec3(20.0f, 0.5f, 20.0f));
	m_ground->model = glm::translate(m_ground->model, glm::vec3(0.0f, -0.25f, 0.0f));

	m_player = m_bunny; // Use the bunny as our player
	m_player->model = glm::scale(m_player->model, glm::vec3(10.5f));

	m_obstacle = m_suzanne; // Use Suzanne as our obstacle
	m_obstacle->model = glm::scale(m_obstacle->model, glm::vec3(0.25f));
	m_obstacle->model = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 2.0f, 5.0f));

	// Initialize game state
	m_playerPosition = glm::vec3(0.0f, 0.25f, 0.0f);
	m_playerRotation = 0.0f;
	m_playerVelocity = glm::vec3(0.0f);
	m_isJumping = false;
	m_gameOver = false;

	// Set up camera
	m_cameraDistance = 10.0f;
	m_cameraHeight = 5.0f;
	UpdateCamera();

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
	float moveSpeed = 2.0f;
	float rotationSpeed = 2.0f;

	glm::vec3 moveDirection(0.0f);
	if (inputManager->IsKeyPressed(GLFW_KEY_W))
	{
		moveDirection += -glm::vec3(sin(m_playerRotation), 0, cos(m_playerRotation));
	}
	else if (inputManager->IsKeyPressed(GLFW_KEY_S))
	{
		moveDirection += glm::vec3(sin(m_playerRotation), 0, cos(m_playerRotation));
	}

	// Normalize move direction to prevent diagonal movement from being faster
	if (glm::length(moveDirection) > 0.0f)
	{
		moveDirection = glm::normalize(moveDirection) * moveSpeed;
	}

	// Apply acceleration instead of directly setting velocity
	float acceleration = 10.0f;
	m_playerVelocity += moveDirection * acceleration * dt;

	if (inputManager->IsKeyPressed(GLFW_KEY_A))
	{
		m_playerRotation += rotationSpeed * dt;
	}
	else if (inputManager->IsKeyPressed(GLFW_KEY_D))
	{
		m_playerRotation -= rotationSpeed * dt;
	}

	// Jumping
	if (inputManager->IsKeyJustPressed(GLFW_KEY_SPACE) && !m_isJumping)
	{
		m_playerVelocity.y = 5.0f;
		m_isJumping = true;
	}

	// Apply gravity
	m_playerVelocity.y -= 9.8f * dt;

	// Apply drag to slow down the player
	float dragCoefficient = 2.0f;
	m_playerVelocity *= (1.0f / (1.0f + dragCoefficient * dt));

	// Clamp velocity
	float maxSpeed = 5.0f;
	if (glm::length(m_playerVelocity) > maxSpeed)
	{
		m_playerVelocity = glm::normalize(m_playerVelocity) * maxSpeed;
	}

	// Update position
	m_playerPosition += m_playerVelocity * dt;

	// Ground collision
	if (m_playerPosition.y < 1.2f)
	{
		m_playerPosition.y = 1.2f;
		m_playerVelocity.y = 0.0f;
		m_isJumping = false;

		// Apply friction
		float frictionCoefficient = 0.9f;
		m_playerVelocity.x *= frictionCoefficient;
		m_playerVelocity.z *= frictionCoefficient;
	}

	// Update player model
	m_player->model = glm::translate(glm::mat4(1.0f), m_playerPosition);
	m_player->model = glm::rotate(m_player->model, m_playerRotation, glm::vec3(0, 1, 0));
	m_player->model = glm::scale(m_player->model, glm::vec3(10.5f));

	// Check for win condition (reaching the obstacle)
	if (glm::distance(m_playerPosition, glm::vec3(5.0f, 0.5f, 5.0f)) < 1.0f)
	{
		spdlog::info("You win! Press R to play again.");
		m_gameOver = true;
	}

	// Update camera
	UpdateCamera();

	// Rotate obstacle
	m_obstacle->model = glm::rotate(m_obstacle->model, dt, glm::vec3(0.0f, 1.0f, 0.0f));

	// Handle escape key
	if (inputManager->IsKeyJustPressed(GLFW_KEY_ESCAPE))
	{
		m_window->Close();
	}
}

void PlatformerGame::UpdateCamera()
{
	// update the orbiting camera
	glm::vec3 camPos = m_camera.GetPosition();
	camPos.x = m_playerPosition.x + m_cameraDistance * sin(m_playerRotation);
	camPos.z = m_playerPosition.z + m_cameraDistance * cos(m_playerRotation);
	camPos.y = m_playerPosition.y + m_cameraHeight;
	m_camera.SetPosition(camPos);
	m_camera.SetTarget(m_playerPosition);
}

void PlatformerGame::ResetGame()
{
	m_playerPosition = glm::vec3(0.0f, 0.25f, 0.0f);
	m_playerRotation = 0.0f;
	m_playerVelocity = glm::vec3(0.0f);
	m_isJumping = false;
	m_gameOver = false;
}
