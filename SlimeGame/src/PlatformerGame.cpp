#include "PlatformerGame.h"

#include "Camera.h"
#include "DescriptorManager.h"
#include "VulkanContext.h"
#include "ModelManager.h"
#include "SlimeWindow.h"
#include "spdlog/spdlog.h"

PlatformerGame::PlatformerGame(SlimeWindow* window)
      : Scene(), m_window(window)
{
	m_camera = Camera(90.0f, 800.0f / 600.0f, 0.001f, 100.0f);

	// Initialize game parameters
	m_gameParams = { .moveSpeed = 2.0f, .rotationSpeed = 2.0f, .jumpForce = 5.0f, .gravity = 9.8f, .dragCoefficient = 2.0f, .frictionCoefficient = 0.9f };

	// Initialize camera state
	m_cameraState = { .distance = 10.0f, .yaw = 0.0f, .pitch = 0.0f, .position = glm::vec3(0.0f) };

	// Light
	PointLight& light = m_pointLights.at(0).light;
	light.pos = glm::vec3(-6, 6, 6);
	light.colour = glm::vec3(1, 0.8, 0.95);

	ResetGame();
}

int PlatformerGame::Enter(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	SetupShaders(vulkanContext, modelManager, shaderManager, descriptorManager);
	m_tempMaterial = descriptorManager.CreateMaterial(vulkanContext, modelManager, "TempMaterial", "albedo.png", "normal.png", "metallic.png", "roughness.png", "ao.png");
	InitializeGameObjects(vulkanContext, modelManager, &m_tempMaterial);

	//m_window->SetCursorMode(GLFW_CURSOR_DISABLED);

	return 0;
}

void PlatformerGame::Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager)
{
	if (m_gameState.gameOver)
	{
		if (inputManager->IsKeyJustPressed(GLFW_KEY_R))
		{
			ResetGame();
		}
		return;
	}

	UpdatePlayer(dt, inputManager);
	UpdateCamera(dt, inputManager);
	CheckCollisions();
	CheckWinCondition();

	// Update obstacle rotation
	m_obstacle.modelMat = glm::rotate(m_obstacle.modelMat, dt, glm::vec3(0.0f, 1.0f, 0.0f));

	if (inputManager->IsKeyPressed(GLFW_KEY_ESCAPE))
	{
		m_window->Close();
	}
}

void PlatformerGame::Render(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	//modelManager.DrawModel()
}

void PlatformerGame::Exit(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	for (auto& light: m_pointLights)
	{
		vmaDestroyBuffer(vulkanContext.GetAllocator(), light.buffer, light.allocation);
	}

	m_camera.DestroyCameraUBOBuffer(vulkanContext.GetAllocator());

	auto basicPipeline = modelManager.GetPipelines()["basic"];
	vulkanContext.GetDispatchTable().destroyPipeline(basicPipeline.pipeline, nullptr);
	vulkanContext.GetDispatchTable().destroyPipelineLayout(basicPipeline.pipelineLayout, nullptr);
}

void PlatformerGame::InitializeGameObjects(VulkanContext& vulkanContext, ModelManager& modelManager, Material* material)
{
	VmaAllocator allocator = vulkanContext.GetAllocator();
	std::string pipelineName = "basic";

	// Initialize player
	auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *bunnyMesh);

	m_player.model = bunnyMesh;
	m_player.modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(10.5f));
	m_player.material = material;

	// Initialize obstacle
	auto suzanneMesh = modelManager.LoadModel("suzanne.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *suzanneMesh);

	m_obstacle.model = suzanneMesh;
	m_obstacle.modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.75f));
	m_obstacle.modelMat = glm::translate(m_obstacle.modelMat, glm::vec3(5.0f, 4.0f, 5.0f));
	m_obstacle.material = material;

	// Initialize ground
	auto cubeMesh = modelManager.LoadModel("cube.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *cubeMesh);
	
	m_ground.model = cubeMesh;
	m_ground.modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 0.5f, 20.0f));
	m_ground.modelMat = glm::translate(m_ground.modelMat, glm::vec3(0.0f, -0.25f, 0.0f));
	m_ground.material = material;

	// Light cube
	PointLight& light = m_pointLights.at(0).light;
	m_lightCube.modelMat = glm::translate(glm::mat4(1.0f), light.pos);
	m_lightCube.material = material;
	m_lightCube.model = cubeMesh;

	modelManager.AddModel("Player", &m_player);
	modelManager.AddModel("Obstacle", &m_obstacle);
	modelManager.AddModel("Ground", &m_ground);
	modelManager.AddModel("Light", &m_lightCube);
}

void PlatformerGame::SetupShaders(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	ResourcePathManager resourcePaths;

	// Load and parse shaders
	std::string vertShaderPath = resourcePaths.GetShaderPath("basic.vert.spv");
	std::string fragShaderPath = resourcePaths.GetShaderPath("basic.frag.spv");

	auto vertexShaderModule = shaderManager.LoadShader(vulkanContext.GetDispatchTable(), vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
	auto fragmentShaderModule = shaderManager.LoadShader(vulkanContext.GetDispatchTable(), fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto vertexResources = shaderManager.ParseShader(vertexShaderModule);
	auto fragmentResources = shaderManager.ParseShader(fragmentShaderModule);
	auto combinedResources = shaderManager.CombineResources({ vertexShaderModule, fragmentShaderModule });

	// Set up descriptor set layout
	auto descriptorSetLayouts = shaderManager.CreateDescriptorSetLayouts(vulkanContext.GetDispatchTable(), combinedResources);

	PipelineGenerator pipelineGenerator(vulkanContext);
	pipelineGenerator.SetName("Basic");
	pipelineGenerator.SetShaderModules(vertexShaderModule, fragmentShaderModule);
	pipelineGenerator.SetVertexInputState(combinedResources.attributeDescriptions, combinedResources.bindingDescriptions);
	pipelineGenerator.SetDescriptorSetLayouts(descriptorSetLayouts);
	pipelineGenerator.SetPushConstantRanges(combinedResources.pushConstantRanges);
	pipelineGenerator.Generate();

	// Descriptor set layout
	descriptorManager.AddDescriptorSetLayouts(descriptorSetLayouts);

	std::vector<VkDescriptorSet> descriptorSets;
	for (int i = descriptorSetLayouts.size() - 1; i >= 0; i--)
	{
		descriptorSets.push_back(descriptorManager.AllocateDescriptorSet(i));
	}

	// Sort the descriptor sets in the order of the layout
	std::reverse(descriptorSets.begin(), descriptorSets.end());

	pipelineGenerator.SetDescriptorSets(descriptorSets);

	modelManager.GetPipelines()["basic"] = pipelineGenerator.GetPipelineContainer();
}

void PlatformerGame::UpdatePlayer(float dt, const InputManager* inputManager)
{
	glm::vec3 moveDirection(0.0f);
	if (inputManager->IsKeyPressed(GLFW_KEY_W))
	{
		moveDirection += -glm::vec3(sin(m_gameState.playerRotation), 0, cos(m_gameState.playerRotation));
	}
	else if (inputManager->IsKeyPressed(GLFW_KEY_S))
	{
		moveDirection += glm::vec3(sin(m_gameState.playerRotation), 0, cos(m_gameState.playerRotation));
	}

	// Normalize move direction
	if (glm::length(moveDirection) > 0.0f)
	{
		moveDirection = glm::normalize(moveDirection) * m_gameParams.moveSpeed;
	}

	// Apply acceleration
	float acceleration = 10.0f;
	m_gameState.playerVelocity += moveDirection * acceleration * dt;

	// Handle rotation
	if (inputManager->IsKeyPressed(GLFW_KEY_A))
	{
		m_gameState.playerRotation += m_gameParams.rotationSpeed * dt;
	}
	else if (inputManager->IsKeyPressed(GLFW_KEY_D))
	{
		m_gameState.playerRotation -= m_gameParams.rotationSpeed * dt;
	}

	// Handle jumping
	if (inputManager->IsKeyJustPressed(GLFW_KEY_SPACE) && !m_gameState.isJumping)
	{
		m_gameState.playerVelocity.y = m_gameParams.jumpForce;
		m_gameState.isJumping = true;
	}

	// Apply gravity
	m_gameState.playerVelocity.y -= m_gameParams.gravity * dt;

	// Apply drag
	m_gameState.playerVelocity *= (1.0f / (1.0f + m_gameParams.dragCoefficient * dt));

	// Clamp velocity
	float maxSpeed = 5.0f;
	if (glm::length(m_gameState.playerVelocity) > maxSpeed)
	{
		m_gameState.playerVelocity = glm::normalize(m_gameState.playerVelocity) * maxSpeed;
	}

	// Update position
	m_gameState.playerPosition += m_gameState.playerVelocity * dt;

	// Update player model matrix
	m_player.modelMat = glm::translate(glm::mat4(1.0f), m_gameState.playerPosition);
	m_player.modelMat = glm::rotate(m_player.modelMat, m_gameState.playerRotation, glm::vec3(0, 1, 0));
	m_player.modelMat = glm::scale(m_player.modelMat, glm::vec3(10.5f));
}

void PlatformerGame::UpdateCamera(float dt, const InputManager* inputManager)
{
	// Get mouse delta
	auto [mouseX, mouseY] = inputManager->GetMouseDelta();

	// Scroll to zoom in/out
	float scrollY = inputManager->GetScrollDelta();
	m_cameraState.distance -= scrollY;

	// Update camera angles
	float mouseSensitivity = 0.1f;
	m_cameraState.yaw += mouseX * mouseSensitivity;
	m_cameraState.pitch += -mouseY * mouseSensitivity;

	// Clamp pitch to avoid flipping
	m_cameraState.pitch = glm::clamp(m_cameraState.pitch, -89.0f, 89.0f);

	// Calculate new camera position
	float horizontalDistance = m_cameraState.distance * cos(glm::radians(m_cameraState.pitch));
	float verticalDistance = m_cameraState.distance * sin(glm::radians(m_cameraState.pitch));

	float camX = horizontalDistance * sin(glm::radians(m_cameraState.yaw));
	float camZ = horizontalDistance * cos(glm::radians(m_cameraState.yaw));

	glm::vec3 targetCamPos = m_gameState.playerPosition + glm::vec3(camX, verticalDistance, camZ);

	// Smoothly interpolate camera position
	float smoothFactor = 1.0f - std::pow(0.001f, dt);
	m_cameraState.position = glm::mix(m_cameraState.position, targetCamPos, smoothFactor);

	// Weight camera yaw towards player rotation
	float weightFactor = 0.2f; // Adjust this value to change how much the camera follows the player's rotation
	float weightedYaw = glm::mix(m_cameraState.yaw, m_gameState.playerRotation, weightFactor);

	// Calculate camera front vector
	glm::vec3 front;
	front.x = cos(glm::radians(weightedYaw)) * cos(glm::radians(m_cameraState.pitch));
	front.y = sin(glm::radians(m_cameraState.pitch));
	front.z = sin(glm::radians(weightedYaw)) * cos(glm::radians(m_cameraState.pitch));
	glm::vec3 cameraFront = glm::normalize(front);

	// Make sure camera is not below the ground
	if (m_cameraState.position.y < 1.0f)
	{
		m_cameraState.position.y = 1.0f;
	}

	// Set camera position and target
	m_camera.SetPosition(m_cameraState.position);
	m_camera.SetTarget(m_gameState.playerPosition + glm::vec3(0.0f, 1.0f, 0.0f));
}

void PlatformerGame::CheckCollisions()
{
	// Ground collision
	if (m_gameState.playerPosition.y < 1.2f)
	{
		m_gameState.playerPosition.y = 1.2f;
		m_gameState.playerVelocity.y = 0.0f;
		m_gameState.isJumping = false;

		// Apply friction
		m_gameState.playerVelocity.x *= m_gameParams.frictionCoefficient;
		m_gameState.playerVelocity.z *= m_gameParams.frictionCoefficient;
	}
}

void PlatformerGame::CheckWinCondition()
{
	if (glm::distance(m_gameState.playerPosition, glm::vec3(5.0f, 0.5f, 5.0f)) < 1.0f)
	{
		spdlog::info("You win! Press R to play again.");
		m_gameState.gameOver = true;
	}
}

void PlatformerGame::ResetGame()
{
	m_gameState = { .playerPosition = glm::vec3(0.0f, 0.25f, 0.0f), .playerVelocity = glm::vec3(0.0f), .playerRotation = 0.0f, .isJumping = false, .gameOver = false };
}
