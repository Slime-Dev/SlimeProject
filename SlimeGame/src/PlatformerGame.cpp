#include "PlatformerGame.h"

#include <Camera.h>
#include <DescriptorManager.h>
#include <imgui.h>
#include <Light.h>
#include <ModelManager.h>
#include <SlimeWindow.h>
#include <spdlog/spdlog.h>
#include <VulkanContext.h>
#include <ResourcePathManager.h>

struct Velocity : public Component
{
	glm::vec3 value;

	void ImGuiDebug()
	{
		ImGui::SliderFloat3("Velocity", &value.x, -10.0f, 10.0f);
	};
};

struct JumpState : public Component
{
	bool isJumping;

	void ImGuiDebug()
	{
		ImGui::Checkbox("Is Jumping", &isJumping);
	};
};

PlatformerGame::PlatformerGame(SlimeWindow* window)
      : Scene(), m_window(window)
{
	Entity mainCamera = Entity("MainCamera");
	mainCamera.AddComponent<Camera>(90.0f, 800.0f / 600.0f, 0.001f, 100.0f);
	m_entityManager.AddEntity(mainCamera);

	// Initialize game parameters
	m_gameParams = { .moveSpeed = 2.0f, .rotationSpeed = 20.0f, .jumpForce = 5.0f, .gravity = 9.8f, .dragCoefficient = 2.0f, .frictionCoefficient = 0.9f };

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
	
	std::shared_ptr<PBRMaterialResource> pbrMaterialResource = descriptorManager.CreatePBRMaterial(vulkanContext, modelManager, "PBR Material", "albedo.png", "normal.png", "metallic.png", "roughness.png", "ao.png");
	std::shared_ptr<BasicMaterialResource> basicMaterialResource = descriptorManager.CreateBasicMaterial(vulkanContext, modelManager, "Basic Material");

	m_pbrMaterials.push_back(pbrMaterialResource);
	m_basicMaterials.push_back(basicMaterialResource);
	m_basicMaterials.push_back(descriptorManager.CopyBasicMaterial(vulkanContext, modelManager, "Basic Material 2", basicMaterialResource));

	InitializeGameObjects(vulkanContext, modelManager);

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

	// Toggle fly cam with 'F' key
	if (inputManager->IsKeyJustPressed(GLFW_KEY_F))
	{
		m_flyCamEnabled = !m_flyCamEnabled;
		if (m_flyCamEnabled)
		{
			m_window->SetCursorMode(GLFW_CURSOR_DISABLED);
		}
		else
		{
			m_window->SetCursorMode(GLFW_CURSOR_NORMAL);
		}
	}

	PointLight& light = m_entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>().light;
	auto& lightCubeTransform = m_lightCube->AddComponent<Transform>();
	lightCubeTransform.position = light.pos;

	if (m_flyCamEnabled)
	{
		UpdateFlyCam(dt, inputManager);
	}
	else
	{
		UpdatePlayer(dt, inputManager);
		UpdateCamera(dt, inputManager);
		CheckCollisions();
		CheckWinCondition();
	}

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
	ImGui::Text("F to toggle fly cam");
	ImGui::Text("Escape to quit");
	ImGui::Separator();
	// Toggle fly cam
	ImGui::Checkbox("Fly Cam Enabled", &m_flyCamEnabled);
	if (m_flyCamEnabled)
	{
		ImGui::Text("Fly Cam Controls:");
		ImGui::Text("WASD - Move, E/Q - Up/Down");
		ImGui::Text("Mouse - Look around");
	}
	ImGui::Separator();
	// Existing camera control toggle
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
	for (auto& material: m_pbrMaterials)
	{
		vmaDestroyBuffer(vulkanContext.GetAllocator(), material->configBuffer, material->configAllocation);
	}

	// Cleanup basic material
	for (auto& material: m_basicMaterials)
	{
		vmaDestroyBuffer(vulkanContext.GetAllocator(), material->configBuffer, material->configAllocation);
	}

	// Clean up lights
	std::vector<std::shared_ptr<Entity>> lightEntities = m_entityManager.GetEntitiesWithComponents<PointLightObject>();
	for (const auto& entity: lightEntities)
	{
		PointLightObject& light = entity->GetComponent<PointLightObject>();
		vmaDestroyBuffer(vulkanContext.GetAllocator(), light.buffer, light.allocation);
	}

	// Clean up cameras
	std::vector<std::shared_ptr<Entity>> cameraEntities = m_entityManager.GetEntitiesWithComponents<Camera>();
	for (const auto& entity: cameraEntities)
	{
		Camera& camera = entity->GetComponent<Camera>();
		camera.DestroyCameraUBOBuffer(vulkanContext.GetAllocator());
	}

	auto basicPipeline = modelManager.GetPipelines()["basic"];
	vulkanContext.GetDispatchTable().destroyPipeline(basicPipeline.pipeline, nullptr);
	vulkanContext.GetDispatchTable().destroyPipelineLayout(basicPipeline.pipelineLayout, nullptr);

	auto debugPipeline = modelManager.GetPipelines()["debug_wire"];
	vulkanContext.GetDispatchTable().destroyPipeline(debugPipeline.pipeline, nullptr);
	vulkanContext.GetDispatchTable().destroyPipelineLayout(debugPipeline.pipelineLayout, nullptr);
	
	auto debugSolidPipeline = modelManager.GetPipelines()["debug_solid"];
	vulkanContext.GetDispatchTable().destroyPipeline(debugSolidPipeline.pipeline, nullptr);
	vulkanContext.GetDispatchTable().destroyPipelineLayout(debugSolidPipeline.pipelineLayout, nullptr);

	auto infiniteGridPipeline = modelManager.GetPipelines()["InfiniteGrid"];
	vulkanContext.GetDispatchTable().destroyPipeline(infiniteGridPipeline.pipeline, nullptr);
	vulkanContext.GetDispatchTable().destroyPipelineLayout(infiniteGridPipeline.pipelineLayout, nullptr);
}

void PlatformerGame::InitializeGameObjects(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	VmaAllocator allocator = vulkanContext.GetAllocator();
	std::string pipelineName = "basic";

	// Create Model Resources
	auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *bunnyMesh);

	auto suzanneMesh = modelManager.LoadModel("suzanne.obj", pipelineName);
	modelManager.CreateBuffersForMesh(allocator, *suzanneMesh);

	auto debugMesh = modelManager.CreateCube(allocator);
	modelManager.CreateBuffersForMesh(allocator, *debugMesh);
	debugMesh->pipeLineName = "debug_wire";

	auto lightCube = modelManager.CreateCube(allocator, 0.1f);
	modelManager.CreateBuffersForMesh(allocator, *lightCube);
	lightCube->pipeLineName = "debug_solid";

	// Initialize player
	m_player->AddComponent<Model>(bunnyMesh);
	m_player->AddComponent<PBRMaterial>(m_pbrMaterials[0]);
	m_player->AddComponent<Velocity>();
	m_player->AddComponent<JumpState>();
	auto& playerTransform = m_player->AddComponent<Transform>();
	playerTransform.scale = glm::vec3(10.5f);

	// Initialize obstacle
	m_obstacle->AddComponent<Model>(suzanneMesh);
	m_obstacle->AddComponent<PBRMaterial>(m_pbrMaterials[0]);
	auto& obstacleTransform = m_obstacle->AddComponent<Transform>();
	obstacleTransform.position = glm::vec3(5.0f, 4.0f, 5.0f);
	obstacleTransform.scale = glm::vec3(0.75f);

	// Light cube
	m_lightCube->AddComponent<Model>(lightCube);
	m_lightCube->AddComponent<BasicMaterial>(m_basicMaterials[0]);

	PointLight& light = m_entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>().light;
	auto& lightCubeTransform = m_lightCube->AddComponent<Transform>();
	lightCubeTransform.position = light.pos;

	// Debug ground
	Entity debugWireFrameCube = Entity("WireFrame Cube");
	debugWireFrameCube.AddComponent<Model>(debugMesh);
	debugWireFrameCube.AddComponent<BasicMaterial>(m_basicMaterials[1]);
	auto& debugGroundTransform = debugWireFrameCube.AddComponent<Transform>();
	m_entityManager.AddEntity(debugWireFrameCube);
	debugGroundTransform.position = glm::vec3(-4.0f, 0.0f, -4.0f);
	debugGroundTransform.scale = glm::vec3(4.0f, 1.0f, 4.0f);

	m_entityManager.AddEntity(m_player);
	m_entityManager.AddEntity(m_obstacle);
	m_entityManager.AddEntity(m_lightCube);
}

void PlatformerGame::SetupShaders(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	ResourcePathManager resourcePaths;

	// Set up a basic pipeline
	modelManager.CreatePipeline("basic", vulkanContext, shaderManager, descriptorManager, resourcePaths.GetShaderPath("basic.vert.spv"), resourcePaths.GetShaderPath("basic.frag.spv"), true);

	// Set up the shared descriptor set pair (Grabbing it from the basic descriptors)
	descriptorManager.CreateSharedDescriptorSet(modelManager.GetPipelines()["basic"].descriptorSetLayouts[0]);

	// Set up a debug_wire pipeline
	modelManager.CreatePipeline("debug_wire", vulkanContext, shaderManager, descriptorManager, resourcePaths.GetShaderPath("wire.vert.spv"), resourcePaths.GetShaderPath("wire.frag.spv"), true, VK_CULL_MODE_NONE, VK_POLYGON_MODE_LINE);
	modelManager.CreatePipeline("debug_solid", vulkanContext, shaderManager, descriptorManager, resourcePaths.GetShaderPath("wire.vert.spv"), resourcePaths.GetShaderPath("wire.frag.spv"), true);

	// Set up InfiniteGrid pipeline
	modelManager.CreatePipeline("InfiniteGrid", vulkanContext, shaderManager, descriptorManager, resourcePaths.GetShaderPath("grid.vert.spv"), resourcePaths.GetShaderPath("grid.frag.spv"), false, VK_CULL_MODE_NONE);
}

void PlatformerGame::UpdatePlayer(float dt, const InputManager* inputManager)
{
	auto& playerTransform = m_player->GetComponent<Transform>();
	auto& playerVelocity = m_player->GetComponent<Velocity>().value;
	bool& isJumping = m_player->GetComponent<JumpState>().isJumping;

	// Get camera
	auto cameraEntity = m_entityManager.GetEntityByName("MainCamera");
	auto& camera = cameraEntity->GetComponent<Camera>();

	// Get camera forward and right vectors
	glm::vec3 cameraForward = glm::vec3(camera.GetCameraUBO().viewPos) - camera.GetPosition();
	cameraForward.y = 0.0f;                  // Project onto xz-plane
	if (glm::length(cameraForward) > 0.001f) // Prevent division by zero
	{
		cameraForward = glm::normalize(cameraForward);
	}
	else
	{
		cameraForward = glm::vec3(0.0f, 0.0f, -1.0f); // Default forward direction
	}
	glm::vec3 cameraRight = glm::cross(cameraForward, glm::vec3(0.0f, 1.0f, 0.0f));
	if (glm::length(cameraRight) > 0.001f) // Prevent division by zero
	{
		cameraRight = glm::normalize(cameraRight);
	}
	else
	{
		cameraRight = glm::vec3(1.0f, 0.0f, 0.0f); // Default right direction
	}

	// Movement
	glm::vec3 moveDirection(0.0f);
	if (inputManager->IsKeyPressed(GLFW_KEY_W))
		moveDirection += cameraForward;
	if (inputManager->IsKeyPressed(GLFW_KEY_S))
		moveDirection -= cameraForward;
	if (inputManager->IsKeyPressed(GLFW_KEY_A))
		moveDirection -= cameraRight;
	if (inputManager->IsKeyPressed(GLFW_KEY_D))
		moveDirection += cameraRight;

	// Normalize and apply move speed
	float moveDirLength = glm::length(moveDirection);
	if (moveDirLength > 0.001f) // Prevent division by zero
	{
		moveDirection = glm::normalize(moveDirection) * m_gameParams.moveSpeed;
	}

	// Smoothly interpolate current velocity towards desired velocity
	float smoothness = 10.0f;
	glm::vec3 targetVelocity = moveDirection;
	playerVelocity = glm::mix(playerVelocity, targetVelocity, glm::clamp(smoothness * dt, 0.0f, 1.0f));

	// Rotation (now based on movement direction)
	if (moveDirLength > 0.001f)
	{
		float targetRotation = atan2(-moveDirection.x, -moveDirection.z);
		float currentRotation = playerTransform.rotation.y;
		float rotationDifference = glm::mod(targetRotation - currentRotation + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
		playerTransform.rotation.y += rotationDifference * m_gameParams.rotationSpeed * dt;
	}

	// Jumping
	if (inputManager->IsKeyJustPressed(GLFW_KEY_SPACE) && !isJumping)
	{
		playerVelocity.y = m_gameParams.jumpForce;
		isJumping = true;
	}

	// Apply gravity
	playerVelocity.y -= m_gameParams.gravity * dt;

	// Apply drag (only to horizontal movement)
	glm::vec2 horizontalVelocity(playerVelocity.x, playerVelocity.z);
	horizontalVelocity *= (1.0f / (1.0f + m_gameParams.dragCoefficient * dt));
	playerVelocity.x = horizontalVelocity.x;
	playerVelocity.z = horizontalVelocity.y;

	// Clamp velocity
	float maxHorizontalSpeed = 5.0f;
	float maxVerticalSpeed = 10.0f;
	horizontalVelocity = glm::vec2(playerVelocity.x, playerVelocity.z);
	float horizontalSpeed = glm::length(horizontalVelocity);
	if (horizontalSpeed > maxHorizontalSpeed)
	{
		horizontalVelocity = (horizontalVelocity / horizontalSpeed) * maxHorizontalSpeed;
		playerVelocity.x = horizontalVelocity.x;
		playerVelocity.z = horizontalVelocity.y;
	}
	playerVelocity.y = glm::clamp(playerVelocity.y, -maxVerticalSpeed, maxVerticalSpeed);

	// Update position
	playerTransform.position += playerVelocity * dt;

	// Sanity check for NaN values
	if (glm::any(glm::isnan(playerVelocity)) || glm::any(glm::isnan(playerTransform.position)))
	{
		// Reset to safe values if NaN is detected
		playerVelocity = glm::vec3(0.0f);
		playerTransform.position = glm::vec3(0.0f, 1.0f, 0.0f); // Adjust as needed
		isJumping = false;
	}

	// Update camera target
	camera.SetTarget(playerTransform.position + glm::vec3(0.0f, 1.0f, 0.0f));
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

void PlatformerGame::UpdateFlyCam(float dt, const InputManager* inputManager)
{
	float moveSpeed = 10.0f * dt;
	float mouseSensitivity = 0.1f;

	// Check if right mouse button is pressed
	bool currentRightMouseState = inputManager->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

	// Mouse look only when right mouse button is pressed
	if (currentRightMouseState)
	{
		auto [mouseX, mouseY] = inputManager->GetMouseDelta();
		m_flyCamYaw += mouseX * mouseSensitivity;
		m_flyCamPitch -= mouseY * mouseSensitivity;
		m_flyCamPitch = glm::clamp(m_flyCamPitch, -89.0f, 89.0f);
	}

	// Handle mouse cursor visibility
	if (currentRightMouseState && !m_rightMousePressed)
	{
		// Right mouse button just pressed
		m_window->SetCursorMode(GLFW_CURSOR_DISABLED);
	}
	else if (!currentRightMouseState && m_rightMousePressed)
	{
		// Right mouse button just released
		m_window->SetCursorMode(GLFW_CURSOR_NORMAL);
	}

	// Update right mouse button state
	m_rightMousePressed = currentRightMouseState;

	// Calculate front, right, and up vectors
	glm::vec3 front;
	front.x = cos(glm::radians(m_flyCamYaw)) * cos(glm::radians(m_flyCamPitch));
	front.y = sin(glm::radians(m_flyCamPitch));
	front.z = sin(glm::radians(m_flyCamYaw)) * cos(glm::radians(m_flyCamPitch));
	glm::vec3 flyCamFront = glm::normalize(front);
	glm::vec3 flyCamRight = glm::normalize(glm::cross(flyCamFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 flyCamUp = glm::normalize(glm::cross(flyCamRight, flyCamFront));

	// Movement (can be done regardless of right mouse button state)
	if (inputManager->IsKeyPressed(GLFW_KEY_W))
		m_flyCamPosition += flyCamFront * moveSpeed;
	if (inputManager->IsKeyPressed(GLFW_KEY_S))
		m_flyCamPosition -= flyCamFront * moveSpeed;
	if (inputManager->IsKeyPressed(GLFW_KEY_A))
		m_flyCamPosition -= flyCamRight * moveSpeed;
	if (inputManager->IsKeyPressed(GLFW_KEY_D))
		m_flyCamPosition += flyCamRight * moveSpeed;
	if (inputManager->IsKeyPressed(GLFW_KEY_E))
		m_flyCamPosition += flyCamUp * moveSpeed;
	if (inputManager->IsKeyPressed(GLFW_KEY_Q))
		m_flyCamPosition -= flyCamUp * moveSpeed;

	// Update camera
	auto cameraEntity = m_entityManager.GetEntityByName("MainCamera");
	if (cameraEntity)
	{
		auto& camera = cameraEntity->GetComponent<Camera>();
		camera.SetPosition(m_flyCamPosition);
		camera.SetTarget(m_flyCamPosition + flyCamFront);
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
