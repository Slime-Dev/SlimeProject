#include "DebugScene.h"

#include <Camera.h>
#include <DescriptorManager.h>
#include <imgui.h>
#include <Light.h>
#include <ModelManager.h>
#include <SlimeWindow.h>
#include <spdlog/spdlog.h>
#include <VulkanContext.h>
#include <ResourcePathManager.h>

DebugScene::DebugScene(SlimeWindow* window)
    : Scene(), m_window(window)
{
    Entity mainCamera = Entity("MainCamera");
    mainCamera.AddComponent<Camera>(90.0f, 1920.0f / 1080.0f, 0.01f, 1000.0f);
    m_entityManager.AddEntity(mainCamera);
}

int DebugScene::Enter(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	SetupShaders(vulkanContext, modelManager, shaderManager, descriptorManager);

	std::shared_ptr<PBRMaterialResource> pbrMaterialResource = descriptorManager.CreatePBRMaterial(vulkanContext, modelManager, "PBR Material", "albedo.png", "normal.png", "metallic.png", "roughness.png", "ao.png");
	m_pbrMaterials.push_back(pbrMaterialResource);
	
	pbrMaterialResource = descriptorManager.CreatePBRMaterial(vulkanContext, modelManager, "PBR Planet Material", "planet_surface/albedo.png", "planet_surface/normal.png", "planet_surface/metallic.png", "planet_surface/roughness.png", "planet_surface/ao.png");
	m_pbrMaterials.push_back(pbrMaterialResource);

    InitializeDebugObjects(vulkanContext, modelManager);
    
	return 0;
}

void DebugScene::SetupShaders(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	// Set up the shadow map pipeline
	modelManager.CreateShadowMapPipeline(vulkanContext, shaderManager, descriptorManager);

	// Set up a basic pipeline
	std::vector<std::pair<std::string, VkShaderStageFlagBits>> meshShaderPaths = {
		{ResourcePathManager::GetShaderPath("basic.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT},
        {ResourcePathManager::GetShaderPath("basic.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> gridShaderPaths = {
		{ResourcePathManager::GetShaderPath("grid.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT},
        {ResourcePathManager::GetShaderPath("grid.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	modelManager.CreatePipeline("pbr", vulkanContext, shaderManager, descriptorManager, meshShaderPaths, true);

	// Set up the shared descriptor set pair (Grabbing it from the basic descriptors)
	descriptorManager.CreateSharedDescriptorSet(modelManager.GetPipelines()["pbr"].descriptorSetLayouts[0]);

	// Set up InfiniteGrid pipeline
	modelManager.CreatePipeline("InfiniteGrid", vulkanContext, shaderManager, descriptorManager, gridShaderPaths, false, VK_CULL_MODE_NONE);
}

void DebugScene::InitializeDebugObjects(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	// Light
	auto lightEntity = std::make_shared<Entity>("Light");
	DirectionalLight& light = lightEntity->AddComponent<DirectionalLightObject>().light;
	m_entityManager.AddEntity(lightEntity);

    VmaAllocator allocator = vulkanContext.GetAllocator();
    auto debugMesh = modelManager.CreateCube(allocator);
    modelManager.CreateBuffersForMesh(allocator, *debugMesh);
	debugMesh->pipelineName = "pbr";

	auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", "pbr");
	modelManager.CreateBuffersForMesh(allocator, *bunnyMesh);

	auto groundPlane = modelManager.CreatePlane(allocator, 300.0f, 10);
	modelManager.CreateBuffersForMesh(allocator, *groundPlane);

	// Create the groundPlane
	Entity ground = Entity("Ground");
	ground.AddComponent<Model>(groundPlane);
	ground.AddComponent<PBRMaterial>(m_pbrMaterials[1]);
	auto& groundTransform = ground.AddComponent<Transform>();
	groundTransform.position = glm::vec3(0.0f, 0.2f, 0.0f); // Slightly above the grid
	m_entityManager.AddEntity(ground);

	// Create a bunny
	Entity bunny = Entity("Bunny");
	bunny.AddComponent<Model>(bunnyMesh);
	bunny.AddComponent<PBRMaterial>(m_pbrMaterials[0]);
	auto& bunnyTransform = bunny.AddComponent<Transform>();
	bunnyTransform.position = glm::vec3(10.0f, 3.0f, -10.0f);
	bunnyTransform.scale = glm::vec3(20.0f);
	m_entityManager.AddEntity(bunny);

    // Large cube at Y=1
    CreateLargeCube(debugMesh, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(25.0f, 1.0f, 25.0f), m_pbrMaterials[0]);

    // Grid of cubes
    const int gridSize = 10;
    const float startY = 6.0f;
    const float cubeOffset = 2.0f;
    const float yVariation = 1.5f;

    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            float xPos = (x - (gridSize - 1) / 2.0f) * cubeOffset;
            float zPos = (z - (gridSize - 1) / 2.0f) * cubeOffset;
            float yPos = startY + static_cast<float>(rand()) / RAND_MAX * yVariation;

            int materialIndex = rand() % m_pbrMaterials.size();
            CreateCube(debugMesh, glm::vec3(xPos, yPos, zPos), glm::vec3(1.0f), m_pbrMaterials[materialIndex]);
        }
    }
}

void DebugScene::CreateCube(ModelResource* mesh, const glm::vec3& position, const glm::vec3& scale, std::shared_ptr<PBRMaterialResource> material)
{
    static int cubeCount = 0;
    Entity cube = Entity("Debug Cube " + std::to_string(++cubeCount));
    cube.AddComponent<Model>(mesh);
    cube.AddComponent<PBRMaterial>(material);
    auto& transform = cube.AddComponent<Transform>();
    transform.position = position;
    transform.scale = scale;
    m_entityManager.AddEntity(cube);
}

void DebugScene::CreateLargeCube(ModelResource* mesh, const glm::vec3& position, const glm::vec3& scale, std::shared_ptr<PBRMaterialResource> material)
{
    Entity largeCube = Entity("Large Debug Cube");
    largeCube.AddComponent<Model>(mesh);
    largeCube.AddComponent<PBRMaterial>(material);
    auto& transform = largeCube.AddComponent<Transform>();
    transform.position = position;
    transform.scale = scale;
    m_entityManager.AddEntity(largeCube);
}

void DebugScene::Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager)
{
    UpdateFlyCam(dt, inputManager);

    if (inputManager->IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        m_window->Close();
    }
}

void DebugScene::Render()
{
	// scene Camera info
	ImGui::Begin("Camera Info");
	ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", m_flyCamPosition.x, m_flyCamPosition.y, m_flyCamPosition.z);
	ImGui::Text("Camera Yaw: %.2f", m_flyCamYaw);
	ImGui::Text("Camera Pitch: %.2f", m_flyCamPitch);
	ImGui::Text("Camera Speed: %.2f", m_cameraSpeed);
	ImGui::End();

    m_entityManager.ImGuiDebug();
}

void DebugScene::Exit(VulkanContext& vulkanContext, ModelManager& modelManager)
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

	lightEntities = m_entityManager.GetEntitiesWithComponents<DirectionalLightObject>();
	for (const auto& entity: lightEntities)
	{
		DirectionalLightObject& light = entity->GetComponent<DirectionalLightObject>();
		vmaDestroyBuffer(vulkanContext.GetAllocator(), light.buffer, light.allocation);
	}

	// Clean up cameras
	std::vector<std::shared_ptr<Entity>> cameraEntities = m_entityManager.GetEntitiesWithComponents<Camera>();
	for (const auto& entity: cameraEntities)
	{
		Camera& camera = entity->GetComponent<Camera>();
		camera.DestroyCameraUBOBuffer(vulkanContext.GetAllocator());
	}

	modelManager.CleanUpAllPipelines(vulkanContext.GetDispatchTable());
}

void DebugScene::UpdateFlyCam(float dt, const InputManager* inputManager)
{
	if (inputManager->GetScrollDelta())
	{
		double multiplier = 1.0;

		// If shift is pressed, increase the speed
		if (inputManager->IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
		{
			multiplier = 10.0;
		}

		m_cameraSpeed += inputManager->GetScrollDelta() * dt * multiplier;

		// Clamp it to 0.00001f
		m_cameraSpeed = glm::max(m_cameraSpeed, 0.00001f);
	}

	float moveSpeed = m_cameraSpeed * dt;
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
	if (inputManager->IsKeyPressed(GLFW_KEY_SPACE))
		m_flyCamPosition += flyCamUp * moveSpeed;
	if (inputManager->IsKeyPressed(GLFW_KEY_LEFT_CONTROL))
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
