#include "PlatformerGame.h"

#include "Camera.h"
#include "DescriptorManager.h"
#include "VulkanContext.h"
#include "ModelManager.h"
#include "SlimeWindow.h"
#include "spdlog/spdlog.h"
#include "imgui.h"
#include "Light.h"

struct Velocity : public Component
{
	glm::vec3 value;
	void ImGuiDebug(){};
};

struct JumpState : public Component
{
    bool isJumping;
	void ImGuiDebug(){};
};

PlatformerGame::PlatformerGame(SlimeWindow* window)
      : Scene(), m_window(window)
{
	Entity mainCamera = Entity("MainCamera");
	mainCamera.AddComponent<Camera>(90.0f, 800.0f / 600.0f, 0.001f, 100.0f);
	m_entityManager.AddEntity(mainCamera);

	// Initialize game parameters
	m_gameParams = { .moveSpeed = 2.0f, .rotationSpeed = 2.0f, .jumpForce = 5.0f, .gravity = 9.8f, .dragCoefficient = 2.0f, .frictionCoefficient = 0.9f };

	// Initialize camera state
	m_cameraState = { .distance = 10.0f, .yaw = 0.0f, .pitch = 0.0f, .position = glm::vec3(0.0f) };

	// Light
	auto lightEntity = std::make_shared<Entity>("Light");
	PointLight& light = lightEntity->AddComponent<PointLightObject>().light;
	m_entityManager.AddEntity(lightEntity);

}

int PlatformerGame::Enter(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	SetupShaders(vulkanContext, modelManager, shaderManager, descriptorManager);
	m_tempMaterial = descriptorManager.CreateMaterial(vulkanContext, modelManager, "TempMaterial", "albedo.png", "normal.png", "metallic.png", "roughness.png", "ao.png");
	InitializeGameObjects(vulkanContext, modelManager, m_tempMaterial);

	//m_window->SetCursorMode(GLFW_CURSOR_DISABLED);

	return 0;
}

void PlatformerGame::Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager)
{
	if (m_gameOver)
	{
		if (inputManager->IsKeyJustPressed(GLFW_KEY_R))
		{
			ResetGame();
		}
		return;
	}

	PointLight& light = m_entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>().light;
	auto& lightCubeTransform = m_lightCube->AddComponent<Transform>();
	lightCubeTransform.position = light.pos;
	
	UpdatePlayer(dt, inputManager);
	UpdateCamera(dt, inputManager);
	CheckCollisions();
	CheckWinCondition();

	// Update obstacle rotation
	m_obstacle->GetComponent<Transform>().rotation = glm::vec3(0.0f, 1.0f, 0.0f) * dt;

	if (inputManager->IsKeyPressed(GLFW_KEY_ESCAPE))
	{
		m_window->Close();
	}
}

void PlatformerGame::Render(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	ImGui::Begin("Scene Controls");
	ImGui::Text("WASD to move, Space to jump");
	ImGui::Text("Escape to quit");
	ImGui::Separator();
	// Toogle camera control mode
	if (ImGui::Checkbox("Camera Mouse Control", &m_cameraMouseControl))
	{
		if (m_cameraMouseControl)
		{
			m_manualYaw = m_cameraState.yaw;
			m_manualPitch = m_cameraState.pitch;
			m_manualDistance = m_cameraState.distance;
		}
	}
	ImGui::End();

	m_entityManager.ImGuiDebug();
}

void PlatformerGame::Exit(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	// Cleanup debug material
	vmaDestroyBuffer(vulkanContext.GetAllocator(), m_tempMaterial->configBuffer, m_tempMaterial->configAllocation);

	// Clean up lights
	std::vector<std::shared_ptr<Entity>> lightEntities = m_entityManager.GetEntitiesWithComponents<PointLightObject>();
	for (const auto& entity : lightEntities)
	{
		PointLightObject& light = entity->GetComponent<PointLightObject>();
		vmaDestroyBuffer(vulkanContext.GetAllocator(), light.buffer, light.allocation);
	}

	// Clean up cameras
	std::vector<std::shared_ptr<Entity>> cameraEntities = m_entityManager.GetEntitiesWithComponents<Camera>();
	for (const auto& entity : cameraEntities)
	{
		Camera& camera = entity->GetComponent<Camera>();
		camera.DestroyCameraUBOBuffer(vulkanContext.GetAllocator());
	} 

	auto basicPipeline = modelManager.GetPipelines()["basic"];
	vulkanContext.GetDispatchTable().destroyPipeline(basicPipeline.pipeline, nullptr);
	vulkanContext.GetDispatchTable().destroyPipelineLayout(basicPipeline.pipelineLayout, nullptr);
}

void PlatformerGame::InitializeGameObjects(VulkanContext& vulkanContext, ModelManager& modelManager, std::shared_ptr<MaterialResource> material)
{
	VmaAllocator allocator = vulkanContext.GetAllocator();
	std::string pipelineName = "basic";

	// Create Model Resources
	auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *bunnyMesh);

	auto suzanneMesh = modelManager.LoadModel("suzanne.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *suzanneMesh);
	
	auto cubeMesh = modelManager.LoadModel("cube.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *cubeMesh);
	
	// Initialize player
	m_player->AddComponent<Model>(bunnyMesh);
	m_player->AddComponent<Material>(material.get());
	m_player->AddComponent<Velocity>();
	m_player->AddComponent<JumpState>();
	auto& playerTransform = m_player->AddComponent<Transform>();
	playerTransform.scale = glm::vec3(10.5f);

	// Initialize obstacle
	m_obstacle->AddComponent<Model>(suzanneMesh);
	m_obstacle->AddComponent<Material>(material.get());
	auto& obstacleTransform = m_obstacle->AddComponent<Transform>();
	obstacleTransform.position = glm::vec3(5.0f, 4.0f, 5.0f);
	obstacleTransform.scale = glm::vec3(0.75f);

	// Initialize ground
	m_ground->AddComponent<Model>(cubeMesh);
	m_ground->AddComponent<Material>(material.get());
	auto& groundTransform = m_ground->AddComponent<Transform>();
	groundTransform.scale = glm::vec3(20.0f, 0.5f, 20.0f);
	groundTransform.position = glm::vec3(0.0f, -0.25f, 0.0f);

	// Light cube
	m_lightCube->AddComponent<Model>(cubeMesh);
	m_lightCube->AddComponent<Material>(material.get());

	PointLight& light = m_entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>().light;
	auto& lightCubeTransform = m_lightCube->AddComponent<Transform>();
	lightCubeTransform.position = light.pos;

	m_entityManager.AddEntity(m_player);
	m_entityManager.AddEntity(m_obstacle);
	m_entityManager.AddEntity(m_ground);
	m_entityManager.AddEntity(m_lightCube);
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
    auto playerTransform = m_player->GetComponent<Transform>();
    auto& playerVelocity = m_player->GetComponent<Velocity>().value;
    bool& isJumping = m_player->GetComponent<JumpState>().isJumping;

    glm::vec3 moveDirection(0.0f);
    if (inputManager->IsKeyPressed(GLFW_KEY_W))
    {
        moveDirection += -glm::vec3(sin(playerTransform.rotation.y), 0, cos(playerTransform.rotation.y));
    }
    else if (inputManager->IsKeyPressed(GLFW_KEY_S))
    {
        moveDirection += glm::vec3(sin(playerTransform.rotation.y), 0, cos(playerTransform.rotation.y));
    }

    // Normalize move direction
    if (glm::length(moveDirection) > 0.0f)
    {
        moveDirection = glm::normalize(moveDirection) * m_gameParams.moveSpeed;
    }

    // Apply acceleration
    float acceleration = 10.0f;
    playerVelocity += moveDirection * acceleration * dt;

    // Handle rotation
    if (inputManager->IsKeyPressed(GLFW_KEY_A))
    {
        playerTransform.rotation.y += m_gameParams.rotationSpeed * dt;
    }
    else if (inputManager->IsKeyPressed(GLFW_KEY_D))
    {
        playerTransform.rotation.y -= m_gameParams.rotationSpeed * dt;
    }

    // Handle jumping
    if (inputManager->IsKeyJustPressed(GLFW_KEY_SPACE) && !isJumping)
    {
        playerVelocity.y = m_gameParams.jumpForce;
        isJumping = true;
    }

    // Apply gravity
    playerVelocity.y -= m_gameParams.gravity * dt;

    // Apply drag
    playerVelocity *= (1.0f / (1.0f + m_gameParams.dragCoefficient * dt));

    // Clamp velocity
    float maxSpeed = 5.0f;
    if (glm::length(playerVelocity) > maxSpeed)
    {
        playerVelocity = glm::normalize(playerVelocity) * maxSpeed;
    }

    // Update position
    playerTransform.position += playerVelocity * dt;
}

void PlatformerGame::UpdateCamera(float dt, const InputManager* inputManager)
{
	// If SPACE is pressed, switch to manual camera control
	if (inputManager->IsKeyJustPressed(GLFW_KEY_SPACE))
	{
		m_cameraMouseControl = !m_cameraMouseControl;
	}

	if (!m_cameraMouseControl)
	{
		return;
	}

	// Existing mouse control logic
	auto [mouseX, mouseY] = inputManager->GetMouseDelta();

	float mouseSensitivity = 0.1f;
	m_cameraState.yaw += mouseX * mouseSensitivity;
	m_cameraState.pitch += -mouseY * mouseSensitivity;

	// Scroll to zoom in/out (keep this for both modes)
	float scrollY = inputManager->GetScrollDelta();
	m_cameraState.distance -= scrollY;

	// Clamp pitch to avoid flipping
	m_cameraState.pitch = glm::clamp(m_cameraState.pitch, -89.0f, 89.0f);

	// Calculate new camera position
	float horizontalDistance = m_cameraState.distance * cos(glm::radians(m_cameraState.pitch));
	float verticalDistance = m_cameraState.distance * sin(glm::radians(m_cameraState.pitch));

	float camX = horizontalDistance * sin(glm::radians(m_cameraState.yaw));
	float camZ = horizontalDistance * cos(glm::radians(m_cameraState.yaw));

	auto& playerTransform = m_player->GetComponent<Transform>();
	glm::vec3 targetCamPos = playerTransform.position + glm::vec3(camX, verticalDistance, camZ);

	// Smoothly interpolate camera position
	float smoothFactor = 1.0f - std::pow(0.001f, dt);
	m_cameraState.position = glm::mix(m_cameraState.position, targetCamPos, smoothFactor);

	// Weight camera yaw towards player rotation
	float weightFactor = 0.2f; // Adjust this value to change how much the camera follows the player's rotation
	float weightedYaw = glm::mix(m_cameraState.yaw, playerTransform.rotation.y, weightFactor);

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
	auto cameraEntity = m_entityManager.GetEntityByName("MainCamera");
	if (cameraEntity)
	{
		auto& camera = cameraEntity->GetComponent<Camera>();
		camera.SetPosition(m_cameraState.position);
		camera.SetTarget(playerTransform.position + glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

void PlatformerGame::CheckCollisions()
{
	auto& playerTransform = m_player->GetComponent<Transform>();
	auto& playerVelocity = m_player->GetComponent<Velocity>().value;
	auto& isJumping = m_player->GetComponent<JumpState>().isJumping;

	// Ground collision
	if (playerTransform.position.y < 1.2f)
	{
		playerTransform.position.y = 1.2f;
		playerVelocity.y = 0.0f;
		isJumping = false;

		// Apply friction
		playerVelocity.x *= m_gameParams.frictionCoefficient;
		playerVelocity.z *= m_gameParams.frictionCoefficient;
	}
}

void PlatformerGame::CheckWinCondition()
{
	auto& playerTransform = m_player->GetComponent<Transform>();

	if (glm::distance(playerTransform.position, glm::vec3(5.0f, 0.5f, 5.0f)) < 1.0f)
	{
		spdlog::info("You win! Press R to play again.");
		m_gameOver = true;
	}
}

void PlatformerGame::ResetGame()
{
	auto& playerTransform = m_player->GetComponent<Transform>();
	auto& playerVelocity = m_player->GetComponent<Velocity>().value;
	auto& isJumping = m_player->GetComponent<JumpState>().isJumping;

	playerTransform.position = glm::vec3(0.0f, 0.25f, 0.0f);
	playerTransform.rotation = glm::vec3(0.0f);
	playerVelocity = glm::vec3(0.0f);
	isJumping = false;

	m_gameOver = false;
}
