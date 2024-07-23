#include "Renderer.h"

#include <Camera.h>
#include <Light.h>

#include "DescriptorManager.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "Scene.h"
#include "vk_mem_alloc.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"

void Renderer::SetupViewportAndScissor(vkb::Swapchain swapchain, vkb::DispatchTable disp, VkCommandBuffer& cmd)
{
	VkViewport viewport = { .x = 0.0f, .y = 0.0f, .width = static_cast<float>(swapchain.extent.width), .height = static_cast<float>(swapchain.extent.height), .minDepth = 0.0f, .maxDepth = 1.0f };

	VkRect2D scissor = {
		.offset = { 0, 0 },
          .extent = swapchain.extent
	};

	disp.cmdSetViewport(cmd, 0, 1, &viewport);
	disp.cmdSetScissor(cmd, 0, 1, &scissor);
}

void Renderer::DrawModels(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene* scene)
{
	debugUtils.BeginDebugMarker(cmd, "Draw Models", debugUtil_BeginColour);

	UpdateCommonBuffers(debugUtils, allocator, cmd, scene);

	EntityManager& entityManager = scene->m_entityManager;
	auto pbrModelEntities = entityManager.GetEntitiesWithComponents<Model, PBRMaterial, Transform>();
	auto basicModelEntities = entityManager.GetEntitiesWithComponents<Model, BasicMaterial, Transform>();

	std::vector<std::shared_ptr<Entity>> modelEntities = pbrModelEntities;
	modelEntities.insert(modelEntities.end(), basicModelEntities.begin(), basicModelEntities.end());

	std::sort(modelEntities.begin(), modelEntities.end(), [](const auto& a, const auto& b) { return a->template GetComponent<Model>().modelResource->pipeLineName < b->template GetComponent<Model>().modelResource->pipeLineName; });

	// Get the shared descriptor set
	std::pair<VkDescriptorSet, VkDescriptorSetLayout> sharedDescriptorSet = descriptorManager.GetSharedDescriptorSet();

	// Update shared descriptors before the loop
	UpdateSharedDescriptors(descriptorManager, sharedDescriptorSet.first, sharedDescriptorSet.second, entityManager, allocator);

	std::unordered_map<std::string, PipelineContainer*> pipelineCache;
	PipelineContainer* pipelineContainer = nullptr;
	std::string lastUsedPipeline;

	for (const auto& entity: modelEntities)
	{
		ModelResource* model = entity->GetComponent<Model>().modelResource;
		Transform& transform = entity->GetComponent<Transform>();

		debugUtils.BeginDebugMarker(cmd, ("Process Model: " + entity->GetName()).c_str(), debugUtil_StartDrawColour);

		std::string pipelineName = model->pipeLineName;

		// Bind pipeline if it's different from the last one
		if (lastUsedPipeline != pipelineName)
		{
			auto pipelineIt = pipelineCache.find(pipelineName);
			if (pipelineIt == pipelineCache.end())
			{
				pipelineContainer = BindPipeline(disp, cmd, modelManager, pipelineName, debugUtils);
				pipelineCache[pipelineName] = pipelineContainer;
			}
			else
			{
				pipelineContainer = pipelineIt->second;
				disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContainer->pipeline);
			}

			if (!pipelineContainer)
			{
				debugUtils.EndDebugMarker(cmd);
				continue;
			}

			lastUsedPipeline = pipelineName;

			// Bind shared descriptor set after binding the pipeline
			disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContainer->pipelineLayout, 0, 1, &sharedDescriptorSet.first, 0, nullptr);
		}

		// Bind material descriptor set
		VkDescriptorSet materialDescriptorSet = GetOrUpdateMaterialDescriptorSet(entity.get(), pipelineContainer, descriptorManager, allocator, debugUtils);
		disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContainer->pipelineLayout, 1, 1, &materialDescriptorSet, 0, nullptr);

		UpdatePushConstants(disp, cmd, *pipelineContainer, transform, debugUtils);

		debugUtils.BeginDebugMarker(cmd, "Draw Model", debugUtil_DrawModelColour);
		modelManager.DrawModel(disp, cmd, *model);
		debugUtils.EndDebugMarker(cmd);

		debugUtils.EndDebugMarker(cmd);
	}

	Camera& camera = scene->m_entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	PipelineContainer& infiniteGridPipeline = modelManager.GetPipelines()["InfiniteGrid"];
	DrawInfiniteGrid(disp, cmd, camera, infiniteGridPipeline.pipeline, infiniteGridPipeline.pipelineLayout);

	debugUtils.EndDebugMarker(cmd);
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
	PointLightObject& light = entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>();
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

PipelineContainer* Renderer::BindPipeline(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, const std::string& pipelineName, VulkanDebugUtils& debugUtils)
{
	auto pipelineIt = modelManager.GetPipelines().find(pipelineName);
	if (pipelineIt == modelManager.GetPipelines().end())
	{
		spdlog::error("Pipeline not found: {}", pipelineName);
		return nullptr;
	}

	PipelineContainer* pipelineContainer = &pipelineIt->second;
	disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContainer->pipeline);
	debugUtils.InsertDebugMarker(cmd, "Bind Pipeline", debugUtil_White);

	return pipelineContainer;
}

void Renderer::UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineContainer& pipelineContainer, Transform& transform, VulkanDebugUtils& debugUtils)
{
	m_mvp.model = transform.GetModelMatrix();
	m_mvp.normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_mvp.model)));
	disp.cmdPushConstants(cmd, pipelineContainer.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_mvp), &m_mvp);
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

void Renderer::UpdateSharedDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet sharedSet, VkDescriptorSetLayout setLayout, EntityManager& entityManager, VmaAllocator allocator)
{
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	descriptorManager.BindBuffer(sharedSet, 0, camera.GetCameraUBOBuffer(), 0, sizeof(CameraUBO));

	PointLightObject& light = entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>();
	descriptorManager.BindBuffer(sharedSet, 1, light.buffer, 0, sizeof(light.light));
}

//
/// MATERIALS ///////////////////////////////////
//
VkDescriptorSet Renderer::GetOrUpdateMaterialDescriptorSet(Entity* entity, PipelineContainer* pipelineContainer, DescriptorManager& descriptorManager, VmaAllocator allocator, VulkanDebugUtils& debugUtils)
{
	size_t materialHash = GenerateMaterialHash(entity);

	auto it = m_materialDescriptorCache.find(materialHash);
	if (it != m_materialDescriptorCache.end())
	{
		return it->second;
	}

	// If not found in cache, create a new descriptor set
	VkDescriptorSet newDescriptorSet = descriptorManager.AllocateDescriptorSet(pipelineContainer->descriptorSetLayouts[1]); //TODO Fix this as it doesnt clean old cached descriptor sets and will quickly run out
	debugUtils.SetObjectName(newDescriptorSet, pipelineContainer->name + " Material Descriptor Set");

	// Update the new descriptor set
	if (entity->HasComponent<BasicMaterial>())
	{
		UpdateBasicMaterialDescriptors(descriptorManager, newDescriptorSet, entity, allocator);
	}
	else if (entity->HasComponent<PBRMaterial>())
	{
		UpdatePBRMaterialDescriptors(descriptorManager, newDescriptorSet, entity, allocator);
	}

	// Cache the new descriptor set
	m_materialDescriptorCache[materialHash] = newDescriptorSet;

	return newDescriptorSet;
}

void Renderer::UpdateBasicMaterialDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet materialSet, Entity* entity, VmaAllocator allocator)
{
	BasicMaterialResource& materialResource = *entity->GetComponent<BasicMaterial>().materialResource;
	SlimeUtil::CopyStructToBuffer(materialResource.config, allocator, materialResource.configAllocation);
	descriptorManager.BindBuffer(materialSet, 0, materialResource.configBuffer, 0, sizeof(BasicMaterialResource::Config));
}

void Renderer::UpdatePBRMaterialDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet descSet, Entity* entity, VmaAllocator allocator)
{
	PBRMaterial& pbrMaterial = entity->GetComponent<PBRMaterial>();
	PBRMaterialResource& materialResource = *pbrMaterial.materialResource;

	SlimeUtil::CopyStructToBuffer(materialResource.config, allocator, materialResource.configAllocation);
	descriptorManager.BindBuffer(descSet, 0, materialResource.configBuffer, 0, sizeof(PBRMaterialResource::Config));

	if (materialResource.albedoTex)
		descriptorManager.BindImage(descSet, 1, materialResource.albedoTex->imageView, materialResource.albedoTex->sampler);
	if (materialResource.normalTex)
		descriptorManager.BindImage(descSet, 2, materialResource.normalTex->imageView, materialResource.normalTex->sampler);
	if (materialResource.metallicTex)
		descriptorManager.BindImage(descSet, 3, materialResource.metallicTex->imageView, materialResource.metallicTex->sampler);
	if (materialResource.roughnessTex)
		descriptorManager.BindImage(descSet, 4, materialResource.roughnessTex->imageView, materialResource.roughnessTex->sampler);
	if (materialResource.aoTex)
		descriptorManager.BindImage(descSet, 5, materialResource.aoTex->imageView, materialResource.aoTex->sampler);
}

// Helper function to generate a hash for a material
inline size_t Renderer::GenerateMaterialHash(const Entity* entity)
{
	size_t hash = 0;
	if (entity->HasComponent<BasicMaterial>())
	{
		const auto& material = entity->GetComponent<BasicMaterial>().materialResource;
		hash = std::hash<BasicMaterialResource::Config>{}(material->config);
	}
	else if (entity->HasComponent<PBRMaterial>())
	{
		const auto& material = entity->GetComponent<PBRMaterial>().materialResource;
		hash = std::hash<PBRMaterialResource::Config>{}(material->config);
		// Include texture pointers in the hash
		hash ^= std::hash<const void*>{}(material->albedoTex);
		hash ^= std::hash<const void*>{}(material->normalTex);
		hash ^= std::hash<const void*>{}(material->metallicTex);
		hash ^= std::hash<const void*>{}(material->roughnessTex);
		hash ^= std::hash<const void*>{}(material->aoTex);
	}
	return hash;
}
