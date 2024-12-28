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
#include <VulkanContext.h>

DebugScene::DebugScene(SlimeWindow* window)
      : Scene(), m_window(window)
{
	// Create main camera
	entt::entity mainCamera = m_entityRegistry.create();
	m_entityRegistry.emplace<Camera>(mainCamera, 90.0f, 1920.0f / 1080.0f, 0.01f, 1000.0f);
}

int DebugScene::Enter(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	DescriptorManager& descriptorManager = *vulkanContext.GetDescriptorManager();
	SetupShaders(vulkanContext, modelManager, *vulkanContext.GetShaderManager(), descriptorManager);

	MaterialManager& materialManager = *vulkanContext.GetMaterialManager();

	// Create basic PBR material
	std::shared_ptr<PBRMaterialResource> pbrMaterial = materialManager.CreatePBRMaterial();
	materialManager.SetAllTextures(pbrMaterial, "albedo.png", "normal.png", "metallic.png", "roughness.png", "ao.png");
	m_pbrMaterials.push_back(pbrMaterial);

	InitializeDebugObjects(vulkanContext, modelManager);

	return 0;
}

void DebugScene::SetupShaders(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	// Setup shadow map pipeline
	modelManager.CreateShadowMapPipeline(vulkanContext, shaderManager, descriptorManager);

	// Setup basic PBR pipeline
	std::vector<std::pair<std::string, VkShaderStageFlagBits>> meshShaderPaths = {
		{ ResourcePathManager::GetShaderPath("basic.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT },
        { ResourcePathManager::GetShaderPath("basic.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT }
	};

	modelManager.CreatePipeline("pbr", vulkanContext, shaderManager, descriptorManager, meshShaderPaths, true);
	descriptorManager.CreateSharedDescriptorSet(modelManager.GetPipelines()["pbr"].descriptorSetLayouts[0]);
}

void DebugScene::InitializeDebugObjects(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	// Create directional light
	entt::entity lightEntity = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(lightEntity, glm::vec3(0.0f, 10.0f, 0.0f));
	m_entityRegistry.emplace<DirectionalLight>(lightEntity, glm::vec3(-0.98f, 0.506f, 0.365f));

	VmaAllocator allocator = vulkanContext.GetAllocator();

	// Create basic test meshes
	auto cubeMesh = modelManager.CreateCube(allocator);
	modelManager.CreateBuffersForMesh(allocator, *cubeMesh);
	cubeMesh->pipelineName = "pbr";

	auto groundPlane = modelManager.CreatePlane(allocator, 50.0f, 10);
	modelManager.CreateBuffersForMesh(allocator, *groundPlane);
	groundPlane->pipelineName = "pbr";

	// Create ground plane
	entt::entity planeEntity = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(planeEntity, glm::vec3(0.0f, 0.1f, 0.0f));
	m_entityRegistry.emplace<Model>(planeEntity, groundPlane);
	m_entityRegistry.emplace<PBRMaterial>(planeEntity, m_pbrMaterials[0]);

	// Create test cube
	CreateCube(cubeMesh, glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f), m_pbrMaterials[0]);
}

void DebugScene::CreateCube(ModelResource* mesh, const glm::vec3& position, const glm::vec3& scale, std::shared_ptr<PBRMaterialResource> material)
{
	entt::entity cubeEntity = m_entityRegistry.create();
	m_entityRegistry.emplace<Transform>(cubeEntity, position, glm::vec3(0.0f), scale);
	m_entityRegistry.emplace<Model>(cubeEntity, mesh);
	m_entityRegistry.emplace<PBRMaterial>(cubeEntity, material);
	m_cubeTransforms.push_back(&m_entityRegistry.get<Transform>(cubeEntity));
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
	if (ImGui::Begin("Entity Inspector", nullptr, ImGuiWindowFlags_NoCollapse))
	{
		static entt::entity selectedEntity = entt::null;
		static char searchBuffer[128] = "";
		static std::string lastSearch = "";

		// Persistent height for the top section
		static float hierarchyHeight = 300.0f; // Initial height for the hierarchy section
		const float splitterThickness = 8.0f;

		// Calculate available space dynamically
		float totalAvailableHeight = ImGui::GetContentRegionAvail().y;
		hierarchyHeight = std::clamp(hierarchyHeight, 100.0f, totalAvailableHeight - 100.0f);

		// Search bar
		ImGui::InputTextWithHint("##search", "Search Entity...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
		std::string currentSearch = std::string(searchBuffer);
		if (lastSearch != currentSearch)
			lastSearch = currentSearch;

		// Entity Hierarchy Section
		ImGui::Text("Entity Hierarchy");
		ImGui::Separator();
		ImGui::BeginChild("Entity Hierarchy", ImVec2(0, hierarchyHeight), true);
		{
			std::function<void(entt::entity)> renderEntityHierarchy = [&](entt::entity entity)
			{
				if (entity == entt::null)
					return;

				auto& transform = m_entityRegistry.get<Transform>(entity);
				auto* model = m_entityRegistry.try_get<Model>(entity);

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
				if (selectedEntity == entity)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool hasChildren = !transform.children.empty();
				if (!hasChildren)
					flags |= ImGuiTreeNodeFlags_Leaf;

				std::string entityName = fmt::format("Entity {}", static_cast<int>(entity));
				if (model)
					entityName += fmt::format(": Model {}", model->modelResource->pipelineName);

				if (!lastSearch.empty() && entityName.find(lastSearch) == std::string::npos)
					return;

				bool nodeOpen = ImGui::TreeNodeEx((void*) (intptr_t) entity, flags, entityName.c_str());

				// Context menu
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete Entity"))
					{
						m_entityRegistry.destroy(entity);
						if (selectedEntity == entity)
							selectedEntity = entt::null;
					}
					if (ImGui::MenuItem("Duplicate Entity"))
					{
						// Add duplication logic here
					}
					ImGui::EndPopup();
				}

				if (ImGui::IsItemClicked())
					selectedEntity = entity;

				if (nodeOpen)
				{
					for (auto& child: transform.children)
						renderEntityHierarchy(child);

					ImGui::TreePop();
				}
			};

			// Render root entities
			m_entityRegistry.view<Transform>().each(
			        [&](auto entity, const Transform& transform)
			        {
				        if (transform.parent == entt::null)
					        renderEntityHierarchy(entity);
			        });
		}
		ImGui::EndChild();

		// Resizable Splitter
		ImGui::InvisibleButton("Splitter", ImVec2(-1, splitterThickness));
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 splitterMin = ImGui::GetItemRectMin();
		ImVec2 splitterMax = ImGui::GetItemRectMax();

		// Change color based on state
		ImU32 splitterColor;
		if (ImGui::IsItemActive())
		{
			float delta = ImGui::GetIO().MouseDelta.y;
			hierarchyHeight += delta;

			// Recalculate limits dynamically to avoid out-of-bounds errors
			totalAvailableHeight = ImGui::GetContentRegionAvail().y + hierarchyHeight;
			hierarchyHeight = std::clamp(hierarchyHeight, 100.0f, totalAvailableHeight - 100.0f);

			splitterColor = ImGui::GetColorU32(ImGuiCol_ButtonActive); // Active color
		}
		else if (ImGui::IsItemHovered())
		{
			splitterColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered); // Hover color
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		}
		else
		{
			splitterColor = ImGui::GetColorU32(ImGuiCol_Button); // Default color
		}

		// Draw the splitter highlight with rounded corners
		float cornerRadius = 4.0f; // Adjust the radius as needed
		drawList->AddRectFilled(splitterMin, splitterMax, splitterColor, cornerRadius);

		// Entity Properties Section
		ImGui::Text("Entity Properties");
		ImGui::Separator();
		ImGui::BeginChild("Entity Properties", ImVec2(0, 0), true); // Remaining space
		{
			if (selectedEntity != entt::null)
			{
				ImGui::Text("Selected Entity: %d", static_cast<int>(selectedEntity));
				ImGui::Separator();

				auto* transform = m_entityRegistry.try_get<Transform>(selectedEntity);
				if (transform)
				{
					std::vector<entt::entity> breadcrumb;
					auto current = transform->parent;

					while (current != entt::null)
					{
						breadcrumb.push_back(current);
						current = m_entityRegistry.get<Transform>(current).parent;
					}

					ImGui::Text("Path:");
					for (auto it = breadcrumb.rbegin(); it != breadcrumb.rend(); ++it)
					{
						if (it != breadcrumb.rbegin())
							ImGui::SameLine();

						if (ImGui::SmallButton(fmt::format("Entity {}", static_cast<int>(*it)).c_str()))
							selectedEntity = *it;

						if (std::next(it) != breadcrumb.rend())
							ImGui::SameLine();
						ImGui::Text(">");
					}
				}

				ComponentInspector::Render(m_entityRegistry, selectedEntity);
			}
			else
			{
				ImGui::Text("No Entity Selected");
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

void DebugScene::Exit(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	// Cleanup lights
	m_entityRegistry.view<DirectionalLight>().each([&](auto entity, DirectionalLight& light) { vmaDestroyBuffer(vulkanContext.GetAllocator(), light.buffer, light.allocation); });

	// Cleanup cameras
	m_entityRegistry.view<Camera>().each([&](auto entity, Camera& camera) { camera.DestroyCameraUBOBuffer(vulkanContext.GetAllocator()); });

	modelManager.CleanUpAllPipelines(vulkanContext.GetDispatchTable());
}

void DebugScene::UpdateFlyCam(float dt, const InputManager* inputManager)
{
	// Handle camera speed adjustment
	if (inputManager->GetScrollDelta())
	{
		double multiplier = inputManager->IsKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 10.0 : 1.0;
		m_cameraSpeed += inputManager->GetScrollDelta() * dt * multiplier;
		m_cameraSpeed = glm::max(m_cameraSpeed, 0.00001f);
	}

	float moveSpeed = m_cameraSpeed * dt;
	float mouseSensitivity = 0.1f;

	// Handle mouse look
	bool currentRightMouseState = inputManager->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
	if (currentRightMouseState)
	{
		auto [mouseX, mouseY] = inputManager->GetMouseDelta();
		m_flyCamYaw += mouseX * mouseSensitivity;
		m_flyCamPitch -= mouseY * mouseSensitivity;
		m_flyCamPitch = glm::clamp(m_flyCamPitch, -89.0f, 89.0f);
	}

	// Handle cursor visibility
	if (currentRightMouseState != m_rightMousePressed)
	{
		m_window->SetCursorMode(currentRightMouseState ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		m_rightMousePressed = currentRightMouseState;
	}

	// Calculate camera vectors
	glm::vec3 front;
	front.x = cos(glm::radians(m_flyCamYaw)) * cos(glm::radians(m_flyCamPitch));
	front.y = sin(glm::radians(m_flyCamPitch));
	front.z = sin(glm::radians(m_flyCamYaw)) * cos(glm::radians(m_flyCamPitch));
	glm::vec3 flyCamFront = glm::normalize(front);
	glm::vec3 flyCamRight = glm::normalize(glm::cross(flyCamFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 flyCamUp = glm::normalize(glm::cross(flyCamRight, flyCamFront));

	// Handle movement
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

	// Update camera entity
	entt::entity cameraEntity = m_entityRegistry.view<Camera>().front();
	Camera& camera = m_entityRegistry.get<Camera>(cameraEntity);
	camera.SetPosition(m_flyCamPosition);
	camera.SetTarget(m_flyCamPosition + flyCamFront);
}
