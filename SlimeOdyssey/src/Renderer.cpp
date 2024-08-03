#include "Renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <Camera.h>
#include <functional>
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
}

void Renderer::CleanUp(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	m_shadowSystem.Cleanup(disp, allocator);
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
	if (SlimeUtil::BeginCommandBuffer(disp, cmd) != 0)
		return -1;

	// Generate shadow map
	std::vector<std::shared_ptr<Light>> lights;
	scene->m_entityManager.ForEachEntityWith<DirectionalLight>(
	        [&lights](Entity& entity)
	        {
		        if (auto light = entity.GetComponentShrPtr<DirectionalLight>())
		        {
			        lights.push_back(std::move(light));
		        }
	        });

	scene->m_entityManager.ForEachEntityWith<PointLight>(
	        [&lights](Entity& entity)
	        {
		        if (auto light = entity.GetComponentShrPtr<PointLight>())
		        {
			        lights.push_back(std::move(light));
		        }
	        });

	std::shared_ptr<Camera> camera = scene->m_entityManager.GetEntityByName("MainCamera")->GetComponentShrPtr<Camera>();

	std::function<void(vkb::DispatchTable, VulkanDebugUtils&, VkCommandBuffer&, ModelManager&, Scene*)> DrawModelsForShadowMap = std::bind(&Renderer::DrawModelsForShadowMap, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, scene);

	m_shadowSystem.UpdateShadowMaps(disp, cmd, modelManager, allocator, commandPool, graphicsQueue, debugUtils, scene, DrawModelsForShadowMap, lights, camera);

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

	DirectionalLight& light = lightEntity->GetComponent<DirectionalLight>();
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();

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

		pushConstants.lightSpaceMatrix = light.GetLightSpaceMatrix();
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
	m_shadowSystem.RenderShadowMapInspector(disp, allocator, commandPool, graphicsQueue, modelManager, debugUtils);

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

	DirectionalLight& light = lightEntity->GetComponent<DirectionalLight>();
	if (light.buffer == VK_NULL_HANDLE)
	{
		SlimeUtil::CreateBuffer("Light Buffer", allocator, light.GetBindingDataSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, light.buffer, light.allocation);
	}

	auto bindingData = light.GetBindingData();
	SlimeUtil::CopyStructToBuffer(bindingData, allocator, light.allocation);
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
	auto lightEntity = entityManager.GetEntityByName("Light");
	if (!lightEntity)
	{
		spdlog::critical("Light entity not found in UpdatePBRMaterialDescriptors.");
		return;
	}

	auto light = lightEntity->GetComponentShrPtr<DirectionalLight>();
	if (setIndex == 1) // Light
	{
		descriptorManager.BindBuffer(descSet, 0, light->buffer, 0, light->GetBindingDataSize());
	}
	else if (setIndex == 2) // Material
	{
		PBRMaterial& pbrMaterial = entity->GetComponent<PBRMaterial>();
		PBRMaterialResource& materialResource = *pbrMaterial.materialResource;

		SlimeUtil::CopyStructToBuffer(materialResource.config, allocator, materialResource.configAllocation);
		descriptorManager.BindBuffer(descSet, 0, materialResource.configBuffer, 0, sizeof(PBRMaterialResource::Config));

		// TODO add support for multiple shadow maps
		auto shadowMap = m_shadowSystem.GetShadowMap(light);

		descriptorManager.BindImage(descSet, 1, shadowMap.imageView, shadowMap.sampler);

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
