#include "Renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <Camera.h>
#include <Light.h>

#include "DescriptorManager.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "Scene.h"
#include "vk_mem_alloc.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"

void Renderer::SetUp(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils& debugUtils)
{
	CreateDepthImage(disp, allocator, swapchain, debugUtils);
	CreateShadowMap(disp, allocator, debugUtils);
	m_shadowMapId = ImGui_ImplVulkan_AddTexture(m_shadowMap.sampler, m_shadowMap.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderer::CleanUp(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	CleanUpShadowMap(disp, allocator);
	CleanupDepthImage(disp, allocator);
}

int Renderer::Draw(vkb::DispatchTable& disp,
        VkCommandBuffer& cmd,
        ModelManager& modelManager,
        DescriptorManager& descriptorManager,
        VmaAllocator allocator,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VulkanDebugUtils& debugUtils,
        vkb::Swapchain swapchain,
        std::vector<VkImage>& swapchainImages,
        std::vector<VkImageView>& swapchainImageViews,
        uint32_t imageIndex,
        Scene* scene)
{
	// Before we begin we want to see if we have changed the shadow map size
	if ((m_newShadowMapWidth > 0 && m_shadowMapHeight > 0) && (m_shadowMapWidth != m_newShadowMapWidth || m_shadowMapHeight != m_newShadowMapHeight))
	{
		disp.deviceWaitIdle();
		m_shadowMapWidth = m_newShadowMapWidth;
		m_shadowMapHeight = m_newShadowMapHeight;
		CreateShadowMap(disp, allocator, debugUtils);
		ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet) m_shadowMapId);
		m_shadowMapId = ImGui_ImplVulkan_AddTexture(m_shadowMap.sampler, m_shadowMap.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	if (SlimeUtil::BeginCommandBuffer(disp, cmd) != 0)
		return -1;

	// Generate shadow map
	GenerateShadowMap(disp, cmd, modelManager, descriptorManager, allocator, commandPool, graphicsQueue, debugUtils, scene);

	SetupViewportAndScissor(swapchain, disp, cmd);
	SlimeUtil::SetupDepthTestingAndLineWidth(disp, cmd);

	// Transition color image to color attachment optimal
	modelManager.TransitionImageLayout(disp, graphicsQueue, commandPool, swapchainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	// Transition depth image to depth attachment optimal
	modelManager.TransitionImageLayout(disp, graphicsQueue, commandPool, m_depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfo colorAttachmentInfo = {};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachmentInfo.imageView = swapchainImageViews[imageIndex];
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.clearValue = {
		.color = {m_clearColour.r, m_clearColour.g, m_clearColour.b, m_clearColour.a}
	};

	VkRenderingAttachmentInfo depthAttachmentInfo = {};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = m_depthImageView;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue.depthStencil.depth = 1.f;

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = {
		.offset = {0, 0},
          .extent = swapchain.extent
	};
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	disp.cmdBeginRendering(cmd, &renderingInfo);

	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create a full screen docking space for ImGui
	ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport(), dockFlags);

	if (scene)
	{
		DrawModels(disp, cmd, modelManager, descriptorManager, allocator, commandPool, graphicsQueue, debugUtils, scene);

		debugUtils.BeginDebugMarker(cmd, "Draw ImGui", debugUtil_BindDescriptorSetColour);
		scene->Render();
	}

	DrawImguiDebugger(disp, allocator, commandPool, graphicsQueue, modelManager, debugUtils);

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	const bool isMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);
	if (!isMinimized)
	{
		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
	}

	debugUtils.EndDebugMarker(cmd);

	disp.cmdEndRendering(cmd);

	// Transition color image to present src layout
	modelManager.TransitionImageLayout(disp, graphicsQueue, commandPool, swapchainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	if (SlimeUtil::EndCommandBuffer(disp, cmd) != 0)
		return -1;

	// Handle multi-viewport rendering here, outside of the main command buffer
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	return 0;
}

void Renderer::SetupViewportAndScissor(vkb::Swapchain swapchain, vkb::DispatchTable disp, VkCommandBuffer& cmd)
{
	VkViewport viewport = { .x = 0.0f, .y = 0.0f, .width = static_cast<float>(swapchain.extent.width), .height = static_cast<float>(swapchain.extent.height), .minDepth = 0.0f, .maxDepth = 1.0f };

	VkRect2D scissor = {
		.offset = {0, 0},
          .extent = swapchain.extent
	};

	disp.cmdSetViewport(cmd, 0, 1, &viewport);
	disp.cmdSetScissor(cmd, 0, 1, &scissor);
}

std::vector<glm::vec3> calculateFrustumCorners(float fov, float aspect, float near, float far, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, const glm::vec3& right)
{
	float tanHalfFov = tan(glm::radians(fov * 0.5f));
	glm::vec3 nearCenter = position + forward * near;
	glm::vec3 farCenter = position + forward * far;

	float nearHeight = 2.0f * tanHalfFov * near;
	float nearWidth = nearHeight * aspect;
	float farHeight = 2.0f * tanHalfFov * far;
	float farWidth = farHeight * aspect;

	std::vector<glm::vec3> corners(8);
	corners[0] = nearCenter + up * (nearHeight * 0.5f) - right * (nearWidth * 0.5f);
	corners[1] = nearCenter + up * (nearHeight * 0.5f) + right * (nearWidth * 0.5f);
	corners[2] = nearCenter - up * (nearHeight * 0.5f) - right * (nearWidth * 0.5f);
	corners[3] = nearCenter - up * (nearHeight * 0.5f) + right * (nearWidth * 0.5f);
	corners[4] = farCenter + up * (farHeight * 0.5f) - right * (farWidth * 0.5f);
	corners[5] = farCenter + up * (farHeight * 0.5f) + right * (farWidth * 0.5f);
	corners[6] = farCenter - up * (farHeight * 0.5f) - right * (farWidth * 0.5f);
	corners[7] = farCenter - up * (farHeight * 0.5f) + right * (farWidth * 0.5f);

	return corners;
}

void Renderer::calculateFrustumSphere(const std::vector<glm::vec3>& frustumCorners, glm::vec3& center, float& radius)
{
	center = glm::vec3(0.0f);
	for (const auto& corner: frustumCorners)
	{
		center += corner;
	}
	center /= frustumCorners.size();

	radius = 0.0f;
	for (const auto& corner: frustumCorners)
	{
		float distance = glm::length(corner - center);
		radius = glm::max(radius, distance);
	}
}

void Renderer::calculateDirectionalLightMatrix(DirectionalLight& dirLight, const Camera& camera)
{
	float fov = camera.GetFOV();
	float aspect = camera.GetAspectRatio();
	float near = m_shadowNear;
	float far = m_shadowFar;
	glm::vec3 lightDir = glm::normalize(-dirLight.direction);
	glm::vec3 cameraPos = camera.GetPosition();

	// Calculate the corners of the view frustum in world space
	std::vector<glm::vec3> frustumCorners = calculateFrustumCorners(fov, aspect, near, far, cameraPos, camera.GetForward(), camera.GetUp(), camera.GetRight());

	// Calculate the bounding sphere of the frustum
	glm::vec3 frustumCenter;
	float frustumRadius;
	calculateFrustumSphere(frustumCorners, frustumCenter, frustumRadius);

	// Check if recalculation is necessary
	bool needsRecalculation = m_firstCalculation;
	if (!m_firstCalculation)
	{
		float distanceMove = glm::length(frustumCenter - m_lastFrustumCenter);
		float radiusChange = std::abs(frustumRadius - m_lastFrustumRadius);

		// Recalculate if the frustum has moved more than 20% of its radius
		// or if the radius has changed by more than 10%
		needsRecalculation = (distanceMove > 0.2f * m_lastFrustumRadius) || (radiusChange > 0.1f * m_lastFrustumRadius);

		// If the light has changed dir also recalculate
		needsRecalculation |= m_lastLightDir != lightDir;
	}

	if (!needsRecalculation)
	{
		// If recalculation is not needed, use the last calculated matrix
		dirLight.lightSpaceMatrix = m_lastLightSpaceMatrix;
		return;
	}

	// Update last frustum data
	m_lastFrustumCenter = frustumCenter;
	m_lastFrustumRadius = frustumRadius;
	m_firstCalculation = false;

	// Calculate the light's position
	m_lastLightDir = lightDir;
	glm::vec3 dirLightPosition = frustumCenter - lightDir * frustumRadius;

	// Create the light's view matrix
	glm::mat4 lightView = glm::lookAt(dirLightPosition, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

	// Calculate the bounding box of the frustum in light space
	glm::vec3 minBounds(std::numeric_limits<float>::max());
	glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

	for (const auto& corner: frustumCorners)
	{
		glm::vec3 lightSpaceCorner = glm::vec3(lightView * glm::vec4(corner, 1.0f));
		minBounds = glm::min(minBounds, lightSpaceCorner);
		maxBounds = glm::max(maxBounds, lightSpaceCorner);
	}

	// Add some padding to the bounding box
	float padding = frustumRadius * 0.1f; // 10% of the frustum radius as padding
	minBounds -= glm::vec3(padding);
	maxBounds += glm::vec3(padding);

	// Adjust the Z range for Vulkan's depth range [0, 1]
	float zNear = -maxBounds.z;
	float zFar = -minBounds.z;

	// Create the light's orthographic projection matrix
	glm::mat4 lightProjection = glm::ortho(minBounds.x, maxBounds.x, minBounds.y, maxBounds.y, zNear, zFar);

	glm::mat4 vulkanNdcAdjustment = glm::mat4(1.0f);
	vulkanNdcAdjustment[1][1] = -1.0f;
	vulkanNdcAdjustment[2][2] = 0.5f;
	vulkanNdcAdjustment[3][2] = 0.5f;

	// Combine view and projection matrices
	m_lastLightSpaceMatrix = vulkanNdcAdjustment * lightProjection * lightView;
	dirLight.lightSpaceMatrix = m_lastLightSpaceMatrix;
}

void Renderer::DrawModelsForShadowMap(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VkCommandBuffer& cmd, ModelManager& modelManager, Scene* scene)
{
	EntityManager& entityManager = scene->m_entityManager;
	auto modelEntities = entityManager.GetEntitiesWithComponents<Model, Transform>();

	// Get the light entity and its transform
	auto lightEntity = entityManager.GetEntityByName("Light");

	if (!lightEntity)
	{
		spdlog::error("Light entity not found, skipping shadow mapping.");
		return;
	}

	DirectionalLight& light = lightEntity->GetComponent<DirectionalLightObject>().light;
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	calculateDirectionalLightMatrix(light, camera);

	// Bind the shadow map pipeline
	PipelineConfig* shadowMapPipeline = BindPipeline(disp, cmd, modelManager, "ShadowMap", debugUtils);
	if (!shadowMapPipeline)
	{
		debugUtils.EndDebugMarker(cmd);
		return;
	}

	VkBool32 depthTestEnable = VK_TRUE;
	disp.cmdSetDepthTestEnable(cmd, depthTestEnable);

	VkBool32 depthWriteEnable = VK_TRUE;
	disp.cmdSetDepthWriteEnable(cmd, depthWriteEnable);

	VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
	disp.cmdSetDepthCompareOp(cmd, depthCompareOp);

	for (const auto& entity: modelEntities)
	{
		ModelResource* model = entity->GetComponent<Model>().modelResource;
		Transform& transform = entity->GetComponent<Transform>();

		debugUtils.BeginDebugMarker(cmd, ("Process Model for Shadow: " + entity->GetName()).c_str(), debugUtil_StartDrawColour);

		// Update push constants for shadow mapping
		struct ShadowMapPushConstants
		{
			glm::mat4 lightSpaceMatrix;
			glm::mat4 modelMatrix;
		} pushConstants;

		pushConstants.lightSpaceMatrix = light.lightSpaceMatrix;
		pushConstants.modelMatrix = transform.GetModelMatrix();

		disp.cmdPushConstants(cmd, shadowMapPipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowMapPushConstants), &pushConstants);

		debugUtils.BeginDebugMarker(cmd, "Draw Model for Shadow", debugUtil_DrawModelColour);
		modelManager.DrawModel(disp, cmd, *model);
		debugUtils.EndDebugMarker(cmd);

		debugUtils.EndDebugMarker(cmd);
	}
}

void Renderer::DrawModels(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, VulkanDebugUtils& debugUtils, Scene* scene)
{
	debugUtils.BeginDebugMarker(cmd, "Draw Models", debugUtil_BeginColour);

	UpdateCommonBuffers(debugUtils, allocator, cmd, scene);

	EntityManager& entityManager = scene->m_entityManager;
	auto pbrModelEntities = entityManager.GetEntitiesWithComponents<Model, PBRMaterial, Transform>();
	auto basicModelEntities = entityManager.GetEntitiesWithComponents<Model, BasicMaterial, Transform>();

	std::vector<std::shared_ptr<Entity>> modelEntities = pbrModelEntities;
	modelEntities.insert(modelEntities.end(), basicModelEntities.begin(), basicModelEntities.end());

	std::sort(modelEntities.begin(), modelEntities.end(), [](const auto& a, const auto& b) { return a->template GetComponent<Model>().modelResource->pipelineName < b->template GetComponent<Model>().modelResource->pipelineName; });

	// Get the shared descriptor set
	std::pair<VkDescriptorSet, VkDescriptorSetLayout> sharedDescriptorSet = descriptorManager.GetSharedDescriptorSet();

	// Update shared descriptors before the loop
	UpdateSharedDescriptors(descriptorManager, sharedDescriptorSet.first, sharedDescriptorSet.second, entityManager, allocator);

	std::unordered_map<std::string, PipelineConfig*> pipelineCache;
	PipelineConfig* pipelineConfig = nullptr;
	std::string lastUsedPipeline;

	for (const auto& entity: modelEntities)
	{
		ModelResource* model = entity->GetComponent<Model>().modelResource;
		Transform& transform = entity->GetComponent<Transform>();

		debugUtils.BeginDebugMarker(cmd, ("Process Model: " + entity->GetName()).c_str(), debugUtil_StartDrawColour);

		std::string pipelineName = model->pipelineName;

		// Bind pipeline if it's different from the last one
		if (lastUsedPipeline != pipelineName)
		{
			auto pipelineIt = pipelineCache.find(pipelineName);
			if (pipelineIt == pipelineCache.end())
			{
				pipelineConfig = BindPipeline(disp, cmd, modelManager, pipelineName, debugUtils);
				pipelineCache[pipelineName] = pipelineConfig;
			}
			else
			{
				pipelineConfig = pipelineIt->second;
				disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineConfig->pipeline);
			}

			if (!pipelineConfig)
			{
				debugUtils.EndDebugMarker(cmd);
				continue;
			}

			lastUsedPipeline = pipelineName;

			// Bind shared descriptor set after binding the pipeline
			disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineConfig->pipelineLayout, 0, 1, &sharedDescriptorSet.first, 0, nullptr);
		}

		// Bind descriptor sets
		for (size_t i = 1; i < pipelineConfig->descriptorSetLayouts.size(); i++)
		{
			VkDescriptorSet currentDescriptorSet = GetOrUpdateDescriptorSet(entityManager, entity.get(), pipelineConfig, descriptorManager, allocator, debugUtils, i);
			disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineConfig->pipelineLayout, i, 1, &currentDescriptorSet, 0, nullptr);
		}

		UpdatePushConstants(disp, cmd, *pipelineConfig, transform, debugUtils);

		debugUtils.BeginDebugMarker(cmd, "Draw Model", debugUtil_DrawModelColour);
		modelManager.DrawModel(disp, cmd, *model);
		debugUtils.EndDebugMarker(cmd);

		debugUtils.EndDebugMarker(cmd);
	}

	PipelineConfig& infiniteGridPipeline = modelManager.GetPipelines()["InfiniteGrid"];
	if (infiniteGridPipeline.pipeline != VK_NULL_HANDLE)
	{
		Camera& camera = scene->m_entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
		DrawInfiniteGrid(disp, cmd, camera, infiniteGridPipeline.pipeline, infiniteGridPipeline.pipelineLayout);
	}

	debugUtils.EndDebugMarker(cmd);
}

void Renderer::DrawImguiDebugger(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, VulkanDebugUtils& debugUtils)
{
	RenderShadowMapInspector(disp, allocator, commandPool, graphicsQueue, modelManager, debugUtils);

	// Render the ImGui debugger
	ImGui::Begin("Renderer Debugger");

	// clear colour
	ImGui::ColorEdit4("Clear Colour", &m_clearColour.r);

	ImGui::End();
}

void Renderer::UpdateCommonBuffers(VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, Scene* scene)
{
	debugUtils.BeginDebugMarker(cmd, "Update Common Buffers", debugUtil_UpdateLightBufferColour);

	EntityManager& entityManager = scene->m_entityManager;

	// Light Buffer
	UpdateLightBuffer(entityManager, allocator);

	// Camera Buffer
	UpdateCameraBuffer(entityManager, allocator);

	debugUtils.EndDebugMarker(cmd);
}

void Renderer::UpdateLightBuffer(EntityManager& entityManager, VmaAllocator allocator)
{
	auto lightEntity = entityManager.GetEntityByName("Light");
	if (!lightEntity)
	{
		spdlog::critical("Light entity not found in UpdateLightBuffer.");
		return;
	}

	DirectionalLightObject& light = lightEntity->GetComponent<DirectionalLightObject>();
	if (light.buffer == VK_NULL_HANDLE)
	{
		SlimeUtil::CreateBuffer("Light Buffer", allocator, sizeof(light.light), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, light.buffer, light.allocation);
	}
	SlimeUtil::CopyStructToBuffer(light.light, allocator, light.allocation);
}

void Renderer::UpdateCameraBuffer(EntityManager& entityManager, VmaAllocator allocator)
{
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	camera.UpdateCameraUBO(allocator);
	SlimeUtil::CopyStructToBuffer(camera.GetCameraUBO(), allocator, camera.GetCameraUBOAllocation());
}

PipelineConfig* Renderer::BindPipeline(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, const std::string& pipelineName, VulkanDebugUtils& debugUtils)
{
	auto pipelineIt = modelManager.GetPipelines().find(pipelineName);
	if (pipelineIt == modelManager.GetPipelines().end())
	{
		spdlog::error("Pipeline not found: {}", pipelineName);
		return nullptr;
	}

	PipelineConfig* pipelineConfig = &pipelineIt->second;
	disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineConfig->pipeline);
	debugUtils.InsertDebugMarker(cmd, "Bind Pipeline", debugUtil_White);

	return pipelineConfig;
}

void Renderer::UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineConfig& pipelineConfig, Transform& transform, VulkanDebugUtils& debugUtils)
{
	m_mvp.model = transform.GetModelMatrix();
	m_mvp.normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_mvp.model)));
	disp.cmdPushConstants(cmd, pipelineConfig.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_mvp), &m_mvp);
	debugUtils.InsertDebugMarker(cmd, "Update Push Constants", debugUtil_White);
}

void Renderer::DrawInfiniteGrid(vkb::DispatchTable& disp, VkCommandBuffer commandBuffer, const Camera& camera, VkPipeline gridPipeline, VkPipelineLayout gridPipelineLayout)
{
	disp.cmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline);

	struct GridPushConstants
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec3 pos;
	} pushConstants;

	pushConstants.view = camera.GetViewMatrix();
	pushConstants.projection = camera.GetProjectionMatrix();
	pushConstants.pos = camera.GetPosition();

	disp.cmdPushConstants(commandBuffer, gridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GridPushConstants), &pushConstants);

	disp.cmdDraw(commandBuffer, 6, 1, 0, 0);
}

// Right now it is only camera ubo
void Renderer::UpdateSharedDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet sharedSet, VkDescriptorSetLayout setLayout, EntityManager& entityManager, VmaAllocator allocator)
{
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	descriptorManager.BindBuffer(sharedSet, 0, camera.GetCameraUBOBuffer(), 0, sizeof(CameraUBO));
}

//
/// MATERIALS ///////////////////////////////////
//
VkDescriptorSet Renderer::GetOrUpdateDescriptorSet(EntityManager& entityManager, Entity* entity, PipelineConfig* pipelineConfig, DescriptorManager& descriptorManager, VmaAllocator allocator, VulkanDebugUtils& debugUtils, int setIndex)
{
	size_t descriptorHash = GenerateDescriptorHash(entity, setIndex);

	if (m_forceInvalidateDecriptorSets)
	{
		m_forceInvalidateDecriptorSets = false;

		for (auto& entry: m_materialDescriptorCache)
		{
			descriptorManager.FreeDescriptorSet(entry.second->descriptorSet);
		}

		m_materialDescriptorCache.clear();
		m_lruList.clear();
	}

	auto it = m_materialDescriptorCache.find(descriptorHash);
	if (it != m_materialDescriptorCache.end())
	{
		// Move the accessed item to the front of the LRU list
		m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
		return it->second->descriptorSet;
	}

	// If not found in cache, create a new descriptor set
	VkDescriptorSet newDescriptorSet = descriptorManager.AllocateDescriptorSet(pipelineConfig->descriptorSetLayouts[setIndex]);
	debugUtils.SetObjectName(newDescriptorSet, pipelineConfig->name + " Material Descriptor Set");

	// Update the new descriptor set
	if (entity->HasComponent<BasicMaterial>())
	{
		UpdateBasicMaterialDescriptors(entityManager, descriptorManager, newDescriptorSet, entity, allocator, setIndex);
	}
	else if (entity->HasComponent<PBRMaterial>())
	{
		UpdatePBRMaterialDescriptors(entityManager, descriptorManager, newDescriptorSet, entity, allocator, setIndex);
	}

	// If cache is full, remove the least recently used item
	if (m_materialDescriptorCache.size() >= MAX_CACHE_SIZE)
	{
		auto last = m_lruList.back(); // This is the least recently used item
		m_materialDescriptorCache.erase(last.hash);
		descriptorManager.FreeDescriptorSet(last.descriptorSet);
		m_lruList.pop_back();
	}

	// Add the new item to the front of the LRU list (most recently used)
	m_lruList.push_front({ descriptorHash, newDescriptorSet });
	m_materialDescriptorCache[descriptorHash] = m_lruList.begin();

	return newDescriptorSet;
}

void Renderer::UpdateBasicMaterialDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet materialSet, Entity* entity, VmaAllocator allocator, int setIndex)
{
	if (setIndex == 1) // Material
	{
		BasicMaterialResource& materialResource = *entity->GetComponent<BasicMaterial>().materialResource;
		SlimeUtil::CopyStructToBuffer(materialResource.config, allocator, materialResource.configAllocation);
		descriptorManager.BindBuffer(materialSet, 0, materialResource.configBuffer, 0, sizeof(BasicMaterialResource::Config));
	}
}

void Renderer::UpdatePBRMaterialDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet descSet, Entity* entity, VmaAllocator allocator, int setIndex)
{
	if (setIndex == 1) // Light
	{
		auto lightEntity = entityManager.GetEntityByName("Light");
		if (!lightEntity)
		{
			spdlog::critical("Light entity not found in UpdatePBRMaterialDescriptors.");
			return;
		}

		DirectionalLightObject& light = lightEntity->GetComponent<DirectionalLightObject>();
		descriptorManager.BindBuffer(descSet, 0, light.buffer, 0, sizeof(light.light));
	}
	else if (setIndex == 2) // Material
	{
		PBRMaterial& pbrMaterial = entity->GetComponent<PBRMaterial>();
		PBRMaterialResource& materialResource = *pbrMaterial.materialResource;

		SlimeUtil::CopyStructToBuffer(materialResource.config, allocator, materialResource.configAllocation);
		descriptorManager.BindBuffer(descSet, 0, materialResource.configBuffer, 0, sizeof(PBRMaterialResource::Config));

		descriptorManager.BindImage(descSet, 1, m_shadowMap.imageView, m_shadowMap.sampler);

		if (materialResource.albedoTex)
			descriptorManager.BindImage(descSet, 2, materialResource.albedoTex->imageView, materialResource.albedoTex->sampler);
		if (materialResource.normalTex)
			descriptorManager.BindImage(descSet, 3, materialResource.normalTex->imageView, materialResource.normalTex->sampler);
		if (materialResource.metallicTex)
			descriptorManager.BindImage(descSet, 4, materialResource.metallicTex->imageView, materialResource.metallicTex->sampler);
		if (materialResource.roughnessTex)
			descriptorManager.BindImage(descSet, 5, materialResource.roughnessTex->imageView, materialResource.roughnessTex->sampler);
		if (materialResource.aoTex)
			descriptorManager.BindImage(descSet, 6, materialResource.aoTex->imageView, materialResource.aoTex->sampler);
	}
}

//
/// SHADOWS ///////////////////////////////////
//
void Renderer::CreateShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils)
{
	m_forceInvalidateDecriptorSets = true;

	spdlog::debug("Creating shadow maps");
	if (m_shadowMap.image != VK_NULL_HANDLE)
	{
		vmaDestroyImage(allocator, m_shadowMap.image, m_shadowMap.allocation);
		disp.destroyImageView(m_shadowMap.imageView, nullptr);
		disp.destroySampler(m_shadowMap.sampler, nullptr);
		vmaDestroyBuffer(allocator, m_shadowMapStagingBuffer, m_shadowMapStagingBufferAllocation);
	}

	// Create Shadow map image
	VkImageCreateInfo shadowMapImageInfo = {};
	shadowMapImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	shadowMapImageInfo.imageType = VK_IMAGE_TYPE_2D;
	shadowMapImageInfo.extent.width = m_shadowMapWidth;
	shadowMapImageInfo.extent.height = m_shadowMapHeight;
	shadowMapImageInfo.extent.depth = 1;
	shadowMapImageInfo.mipLevels = 1;
	shadowMapImageInfo.arrayLayers = 1;
	shadowMapImageInfo.format = VK_FORMAT_D32_SFLOAT;
	shadowMapImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	shadowMapImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	shadowMapImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	shadowMapImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	shadowMapImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo shadowMapAllocInfo = {};
	shadowMapAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(allocator, &shadowMapImageInfo, &shadowMapAllocInfo, &m_shadowMap.image, &m_shadowMap.allocation, nullptr));
	debugUtils.SetObjectName(m_shadowMap.image, "ShadowMapImage");

	// Create the shadow map image view
	VkImageViewCreateInfo shadowMapImageViewInfo = {};
	shadowMapImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	shadowMapImageViewInfo.image = m_shadowMap.image;
	shadowMapImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	shadowMapImageViewInfo.format = VK_FORMAT_D32_SFLOAT;
	shadowMapImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	shadowMapImageViewInfo.subresourceRange.baseMipLevel = 0;
	shadowMapImageViewInfo.subresourceRange.levelCount = 1;
	shadowMapImageViewInfo.subresourceRange.baseArrayLayer = 0;
	shadowMapImageViewInfo.subresourceRange.layerCount = 1;

	VK_CHECK(disp.createImageView(&shadowMapImageViewInfo, nullptr, &m_shadowMap.imageView));
	debugUtils.SetObjectName(m_shadowMap.imageView, "ShadowMapImageView");

	// Create the shadow map sampler
	m_shadowMap.sampler = SlimeUtil::CreateSampler(disp);
	debugUtils.SetObjectName(m_shadowMap.sampler, "ShadowMapSampler");

	m_shadowMapStagingBufferSize = sizeof(float);
	SlimeUtil::CreateBuffer("ShadowMapPixelStagingBuffer", allocator, m_shadowMapStagingBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_shadowMapStagingBuffer, m_shadowMapStagingBufferAllocation);
}

void Renderer::GenerateShadowMap(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, VulkanDebugUtils& debugUtils, Scene* scene)
{
	debugUtils.BeginDebugMarker(cmd, "Draw Models for Shadow Map", debugUtil_BeginColour);
	// Transition shadow map image to depth attachment optimal
	modelManager.TransitionImageLayout(disp, graphicsQueue, commandPool, m_shadowMap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfo depthAttachmentInfo = {};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = m_shadowMap.imageView;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue.depthStencil.depth = 1.0f;

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = {
		.offset = {               0,                 0},
          .extent = {m_shadowMapWidth, m_shadowMapHeight}
	};
	renderingInfo.layerCount = 1;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	disp.cmdBeginRendering(cmd, &renderingInfo);

	// Set viewport and scissor for shadow map
	VkViewport viewport = { 0, 0, (float) m_shadowMapWidth, (float) m_shadowMapHeight, 0.0f, 1.0f };
	VkRect2D scissor = {
		{		       0,		         0},
        {m_shadowMapWidth, m_shadowMapHeight}
	};
	disp.cmdSetViewport(cmd, 0, 1, &viewport);
	disp.cmdSetScissor(cmd, 0, 1, &scissor);

	// Draw models for shadow map
	DrawModelsForShadowMap(disp, debugUtils, cmd, modelManager, scene);

	disp.cmdEndRendering(cmd);

	// Transition shadow map image to shader read-only optimal
	modelManager.TransitionImageLayout(disp, graphicsQueue, commandPool, m_shadowMap.image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	debugUtils.EndDebugMarker(cmd);
}

void Renderer::CleanUpShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	// Clean up shadow map image and image view
	vmaDestroyImage(allocator, m_shadowMap.image, m_shadowMap.allocation);
	disp.destroyImageView(m_shadowMap.imageView, nullptr);
	disp.destroySampler(m_shadowMap.sampler, nullptr);

	if (m_shadowMapStagingBuffer != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(allocator, m_shadowMapStagingBuffer, m_shadowMapStagingBufferAllocation);
		m_shadowMapStagingBuffer = VK_NULL_HANDLE;
		m_shadowMapStagingBufferAllocation = VK_NULL_HANDLE;
	}
}

float Renderer::GetShadowMapPixelValue(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, int x, int y)
{
	// Ensure x and y are within bounds
	x = std::clamp(x, 0, static_cast<int>(m_shadowMapWidth) - 1);
	y = std::clamp(y, 0, static_cast<int>(m_shadowMapHeight) - 1);

	// Create command buffer for copy operation
	VkCommandBuffer commandBuffer = SlimeUtil::BeginSingleTimeCommands(disp, commandPool);

	// Transition shadow map image layout for transfer
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_shadowMap.image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	disp.cmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	// Copy image to buffer
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { x, y, 0 };
	region.imageExtent = { 1, 1, 1 };

	disp.cmdCopyImageToBuffer(commandBuffer, m_shadowMap.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_shadowMapStagingBuffer, 1, &region);

	// Transition shadow map image layout back
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	disp.cmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	SlimeUtil::EndSingleTimeCommands(disp, graphicsQueue, commandPool, commandBuffer);

	// Read data from staging buffer
	float pixelValue;
	void* data;
	vmaMapMemory(allocator, m_shadowMapStagingBufferAllocation, &data);
	memcpy(&pixelValue, data, sizeof(float));
	vmaUnmapMemory(allocator, m_shadowMapStagingBufferAllocation);

	return pixelValue;
}

void Renderer::RenderShadowMapInspector(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, VulkanDebugUtils& debugUtils)
{
	ImGui::Begin("Shadow Map Inspector");

	// Window size and aspect ratio
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	float shadowmapAspectRatio = m_shadowMapWidth / (float) m_shadowMapHeight;

	// Zoom and pan controls
	static float zoom = 1.0f;
	static ImVec2 pan = ImVec2(0.0f, 0.0f);
	ImGui::SliderFloat("Zoom", &zoom, 0.1f, 10.0f);
	ImGui::DragFloat2("Pan", &pan.x, 0.01f);

	// Change the shadow map size
	ImGui::Text("Shadow Map Size:");
	ImGui::SameLine();
	ImGui::PushItemWidth(100);
	ImGui::DragScalar("Width", ImGuiDataType_U32, &m_newShadowMapWidth);
	ImGui::SameLine();
	ImGui::DragScalar("Height", ImGuiDataType_U32, &m_newShadowMapHeight);
	ImGui::PopItemWidth();

	// Shadow near and far
	ImGui::Text("Shadow Near:");
	ImGui::SameLine();
	ImGui::PushItemWidth(100);
	ImGui::DragFloat("Near", &m_shadowNear, 0.1f);
	ImGui::SameLine();
	ImGui::DragFloat("Far", &m_shadowFar, 0.1f);
	ImGui::PopItemWidth();

	// Contrast control
	static float contrast = 1.0f;
	ImGui::SliderFloat("Contrast", &contrast, 0.1f, 5.0f);

	// Display a depth scale
	ImGui::Text("Depth Scale:");
	ImVec2 scaleSize(200, 20);
	ImVec2 scalePos = ImGui::GetCursorScreenPos();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	for (int i = 0; i < 100; i++)
	{
		float t = i / 99.0f;
		float enhancedT = std::pow((t - 0.5f) * contrast + 0.5f, 2.2f);
		enhancedT = std::clamp(enhancedT, 0.0f, 1.0f);
		ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(enhancedT, enhancedT, enhancedT, 1.0f));
		drawList->AddRectFilled(ImVec2(scalePos.x + t * scaleSize.x, scalePos.y), ImVec2(scalePos.x + (t + 0.01f) * scaleSize.x, scalePos.y + scaleSize.y), color);
	}
	drawList->AddRect(scalePos, ImVec2(scalePos.x + scaleSize.x, scalePos.y + scaleSize.y), IM_COL32_WHITE);

	ImGui::Dummy(scaleSize);
	ImGui::Text("0.0");
	ImGui::SameLine(185);
	ImGui::Text("1.0");

	// Calculate image size
	ImVec2 imageSize;
	if (windowSize.x / windowSize.y > shadowmapAspectRatio)
	{
		imageSize = ImVec2(windowSize.y * shadowmapAspectRatio, windowSize.y);
	}
	else
	{
		imageSize = ImVec2(windowSize.x, windowSize.x / shadowmapAspectRatio);
	}

	// Apply zoom and pan
	imageSize.x *= zoom;
	imageSize.y *= zoom;

	// Display shadow map
	ImGui::BeginChild("ShadowMapRegion", windowSize, true, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::SetCursorPos(ImVec2(pan.x, pan.y));
	ImGui::Image((ImTextureID) m_shadowMapId, imageSize);

	// Pixel info on hover
	if (ImGui::IsItemHovered())
	{
		// Disable window manipulation while hovering
		ImGui::SetWindowFocus();

		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 imagePos = ImGui::GetItemRectMin();
		ImVec2 relativePos = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

		int pixelX = (int) ((relativePos.x / imageSize.x) * m_shadowMapWidth);
		int pixelY = (int) ((relativePos.y / imageSize.y) * m_shadowMapHeight);

		float pixelValue = GetShadowMapPixelValue(disp, allocator, commandPool, graphicsQueue, modelManager, pixelX, pixelY);

		// Apply contrast enhancement
		float enhancedValue = std::pow((pixelValue - 0.5f) * contrast + 0.5f, 2.2f);
		enhancedValue = std::clamp(enhancedValue, 0.0f, 1.0f);

		ImGui::BeginTooltip();
		ImGui::Text("Pixel: (%d, %d)", pixelX, pixelY);
		ImGui::Text("Depth: %.3f", pixelValue);
		ImGui::Text("Enhanced: %.3f", enhancedValue);

		// Display a color swatch representing the depth
		ImVec4 depthColor(enhancedValue, enhancedValue, enhancedValue, 1.0f);
		ImGui::ColorButton("Depth Color", depthColor, 0, ImVec2(40, 20));

		ImGui::EndTooltip();

		float wheel = ImGui::GetIO().MouseWheel;
		if (ImGui::GetIO().KeyShift && wheel != 0)
		{
			const float zoomSpeed = 0.1f;
			zoom *= (1.0f + wheel * zoomSpeed);
			zoom = std::clamp(zoom, 0.1f, 10.0f);

			// Adjust pan to zoom towards mouse position
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 center = ImGui::GetWindowPos() + windowSize * 0.5f;
			pan += (mousePos - center) * (wheel * zoomSpeed);
		}

		// Left click and drag to pan
		if (ImGui::IsMouseDragging(0))
		{
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			pan += delta;
		}
	}

	ImGui::EndChild();

	ImGui::End();
}

//
/// DEPTH TESTING ///////////////////////////////////
//
void Renderer::CreateDepthImage(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils& debugUtils)
{
	spdlog::debug("Creating depth image");

	// Clean up old depth image and image view
	if (m_depthImage)
	{
		CleanupDepthImage(disp, allocator);
	}

	// Create the depth image
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // Or VK_FORMAT_D32_SFLOAT_S8_UINT if you need stencil
	VkImageCreateInfo depthImageInfo = {};
	depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
	depthImageInfo.extent.width = swapchain.extent.width;
	depthImageInfo.extent.height = swapchain.extent.height;
	depthImageInfo.extent.depth = 1;
	depthImageInfo.mipLevels = 1;
	depthImageInfo.arrayLayers = 1;
	depthImageInfo.format = depthFormat;
	depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo depthAllocInfo = {};
	depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(allocator, &depthImageInfo, &depthAllocInfo, &m_depthImage, &m_depthImageAllocation, nullptr));
	debugUtils.SetObjectName(m_depthImage, "DepthImage");

	// Create the depth image view
	VkImageViewCreateInfo depthImageViewInfo = {};
	depthImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthImageViewInfo.image = m_depthImage;
	depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthImageViewInfo.format = depthFormat;
	depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthImageViewInfo.subresourceRange.baseMipLevel = 0;
	depthImageViewInfo.subresourceRange.levelCount = 1;
	depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
	depthImageViewInfo.subresourceRange.layerCount = 1;

	VK_CHECK(disp.createImageView(&depthImageViewInfo, nullptr, &m_depthImageView));
	debugUtils.SetObjectName(m_depthImageView, "DepthImageView");
}

void Renderer::CleanupDepthImage(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	vmaDestroyImage(allocator, m_depthImage, m_depthImageAllocation);
	disp.destroyImageView(m_depthImageView, nullptr);
}

// Helper function to generate a hash for a material
// Helper function to generate a hash for all descriptor types
inline size_t Renderer::GenerateDescriptorHash(const Entity* entity, int setIndex)
{
	size_t hash = std::hash<int>{}(setIndex); // Include setIndex in the hash

	switch (setIndex)
	{
		case 0:
		{
			spdlog::error("You shouldnt of hit this!, This is a shared resource set and should be the same for all calls.");
			break;
		}
		case 1:
		{
			if (entity->HasComponent<BasicMaterial>())
			{
				const auto& material = entity->GetComponent<BasicMaterial>().materialResource;
				hash ^= std::hash<BasicMaterialResource::Config>{}(material->config);
			}
			break;
		}
		case 2:
		{
			if (entity->HasComponent<PBRMaterial>())
			{
				const auto& material = entity->GetComponent<PBRMaterial>().materialResource;
				hash ^= std::hash<PBRMaterialResource::Config>{}(material->config);
				// Include texture pointers in the hash
				hash ^= std::hash<const void*>{}(material->albedoTex);
				hash ^= std::hash<const void*>{}(material->normalTex);
				hash ^= std::hash<const void*>{}(material->metallicTex);
				hash ^= std::hash<const void*>{}(material->roughnessTex);
				hash ^= std::hash<const void*>{}(material->aoTex);
			}
			break;
		}
	}

	return hash;
}
