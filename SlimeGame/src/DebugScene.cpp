#include "DebugScene.h"

#include <Camera.h>
#include <ComponentInspector.h>
#include <DescriptorManager.h>
#include <imgui.h>
#include <Light.h>
#include <MaterialManager.h>
#include <ModelManager.h>
#include <ResourcePathManager.h>
#include <SlimeWindow.h>
#include <spdlog/spdlog.h>
#include <VulkanContext.h>

DebugScene::DebugScene(SlimeWindow* window)
      : Scene(), m_window(window)
{
	entt::entity mainCamera = m_entityRegistry.create();
	m_entityRegistry.emplace<Camera>(mainCamera, 90.0f, 1920.0f / 1080.0f, 0.01f, 1000.0f);
}

int DebugScene::Enter(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	DescriptorManager& descriptorManager = *vulkanContext.GetDescriptorManager();
	SetupShaders(vulkanContext, modelManager, *vulkanContext.GetShaderManager(), descriptorManager);

	MaterialManager& materialManager = *vulkanContext.GetMaterialManager();

	std::shared_ptr<PBRMaterialResource> pbrMaterialResource = materialManager.CreatePBRMaterial();
	materialManager.SetAllTextures(pbrMaterialResource, "albedo.png", "normal.png", "metallic.png", "roughness.png", "ao.png");
	m_pbrMaterials.push_back(pbrMaterialResource);

	pbrMaterialResource = materialManager.CreatePBRMaterial();
	materialManager.SetAllTextures(pbrMaterialResource, "planet_surface/albedo.png", "planet_surface/normal.png", "planet_surface/metallic.png", "planet_surface/roughness.png", "planet_surface/ao.png");
	m_pbrMaterials.push_back(pbrMaterialResource);

	pbrMaterialResource = materialManager.CreatePBRMaterial();
	materialManager.SetAllTextures(pbrMaterialResource, "grass/albedo.png", "grass/normal.png", "planet_surface/metallic.png", "grass/roughness.png", "grass/ao.png");
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
		{ResourcePathManager::GetShaderPath("basic.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT},
        {ResourcePathManager::GetShaderPath("basic.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	modelManager.CreatePipeline("pbr", vulkanContext, shaderManager, descriptorManager, meshShaderPaths, true);

	// Set up the shared descriptor set pair (Grabbing it from the basic descriptors)
	descriptorManager.CreateSharedDescriptorSet(modelManager.GetPipelines()["pbr"].descriptorSetLayouts[0]);
}

void DebugScene::InitializeDebugObjects(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	// Light
	entt::entity lightEntity = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(lightEntity, glm::vec3(0.0f, 10.0f, 0.0f));
	m_entityRegistry.emplace<DirectionalLight>(lightEntity, glm::vec3(-0.98f, 0.506f, 0.365f));

	VmaAllocator allocator = vulkanContext.GetAllocator();
	auto debugMesh = modelManager.CreateCube(allocator);
	modelManager.CreateBuffersForMesh(allocator, *debugMesh);
	debugMesh->pipelineName = "pbr";

	auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", "pbr");
	modelManager.CreateBuffersForMesh(allocator, *bunnyMesh);

	auto groundPlane = modelManager.CreatePlane(allocator, 50.0f, 25);
	modelManager.CreateBuffersForMesh(allocator, *groundPlane);

	// Create the groundPlane
	entt::entity gridEntity = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(gridEntity, glm::vec3(0.0f, 0.2f, 0.0f));
	m_entityRegistry.emplace<Model>(gridEntity, groundPlane);
	m_entityRegistry.emplace<PBRMaterial>(gridEntity, m_pbrMaterials[2]);

	// Create a bunny
	entt::entity bunnyEntity = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(bunnyEntity, glm::vec3(7.5f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(20.0f)); // pos, rot, scale
	m_entityRegistry.emplace<Model>(bunnyEntity, bunnyMesh);
	m_entityRegistry.emplace<PBRMaterial>(bunnyEntity, m_pbrMaterials[0]);

	// Large cube at Y=1
	CreateLargeCube(debugMesh, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(25.0f, 1.0f, 25.0f), m_pbrMaterials[0]);

	// Grid of cubes
	const int gridSize = 6;
	const float startY = 6.0f;
	const float cubeOffset = 2.0f;
	const float yVariation = 1.5f;

	for (int x = 0; x < gridSize; ++x)
	{
		for (int z = 0; z < gridSize; ++z)
		{
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
	entt::entity cubeEntity = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(cubeEntity, position, glm::vec3(0.0f), scale);
	m_entityRegistry.emplace<Model>(cubeEntity, mesh);
	m_entityRegistry.emplace<PBRMaterial>(cubeEntity, material);
	m_cubeTransforms.push_back(&m_entityRegistry.get<Transform>(cubeEntity));
}

void DebugScene::CreateLargeCube(ModelResource* mesh, const glm::vec3& position, const glm::vec3& scale, std::shared_ptr<PBRMaterialResource> material)
{
	entt::entity largeCube = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(largeCube, position, glm::vec3(0.0f), scale);
	m_entityRegistry.emplace<Model>(largeCube, mesh);
	m_entityRegistry.emplace<PBRMaterial>(largeCube, material);
}

void DebugScene::Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager)
{
	UpdateFlyCam(dt, inputManager);

	static float time = 0.0f;
	time += dt;
	int cubeIndex = 0;
	for (auto& cubeTransform: m_cubeTransforms)
	{
		// Move the cubes up and down
		cubeTransform->position.y = 2.25f + sin(time + cubeIndex * 0.5f) * 0.5f;
		cubeIndex++;
	}

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

	// ImGui entity inspector
	static entt::entity selectedEntity = entt::null;

	if (ImGui::Begin("Entity Inspector"))
	{
		bool entitySelected = false;

		// List all entities with their details
		m_entityRegistry.group<Transform>(entt::get<Model>)
		        .each(
		                [&](auto entity, Transform& transform, Model& model)
		                {
			                if (ImGui::Selectable(fmt::format("Entity {}: Model {}", static_cast<int>(entity), model.modelResource->pipelineName).c_str(), selectedEntity == entity))
			                {
				                selectedEntity = entity;
				                entitySelected = true;
			                }

			                // You might want to include this for debugging purposes or if you want to see more than one entity at a time.
			                if (selectedEntity == entity)
			                {
				                ImGui::SameLine();
				                ImGui::Text("(Selected)");
			                }
		                });

		ImGui::Separator();

		// Show details of the selected entity
		if (selectedEntity != entt::null)
		{
			ImGui::Text("Details of Entity %d:", selectedEntity);

			// Fetch the components of the selected entity
			auto& transform = m_entityRegistry.get<Transform>(selectedEntity);
			auto& model = m_entityRegistry.get<Model>(selectedEntity);

			ImGui::Text("Position: (%.2f, %.2f, %.2f)", transform.position.x, transform.position.y, transform.position.z);
			ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", transform.rotation.x, transform.rotation.y, transform.rotation.z);
			ImGui::Text("Scale: (%.2f, %.2f, %.2f)", transform.scale.x, transform.scale.y, transform.scale.z);

			ImGui::Separator();

			// Render additional components if needed
			ComponentInspector::Render(m_entityRegistry, selectedEntity);
		}
		else
		{
			ImGui::Text("Select an entity to view details.");
		}

		ImGui::End();
	}
}

void DebugScene::Exit(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	// Clean up lights
	m_entityRegistry.view<PointLight>().each([&](auto entity, PointLight& light) { vmaDestroyBuffer(vulkanContext.GetAllocator(), light.buffer, light.allocation); });

	m_entityRegistry.view<DirectionalLight>().each([&](auto entity, DirectionalLight& light) { vmaDestroyBuffer(vulkanContext.GetAllocator(), light.buffer, light.allocation); });

	// Clean up cameras
	m_entityRegistry.view<Camera>().each([&](auto entity, Camera& camera) { camera.DestroyCameraUBOBuffer(vulkanContext.GetAllocator()); });

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
	entt::entity cameraEntity = m_entityRegistry.view<Camera>().front();
	Camera& camera = m_entityRegistry.get<Camera>(cameraEntity);
	camera.SetPosition(m_flyCamPosition);
	camera.SetTarget(m_flyCamPosition + flyCamFront);
}
