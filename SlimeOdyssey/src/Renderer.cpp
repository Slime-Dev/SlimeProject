#include "Renderer.h"

#include "VulkanContext.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "Scene.h"
#include "VulkanUtil.h"
#include "vk_mem_alloc.h"
#include <Camera.h>
#include <Light.h>

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
	m_shaderDebug.debugMode = 0;

	debugUtils.BeginDebugMarker(cmd, "Draw Models", debugUtil_BeginColour);

	UpdateCommonBuffers(debugUtils, allocator, cmd, scene);

	EntityManager& entityManager = scene->m_entityManager;
	auto modelEntities = entityManager.GetEntitiesWithComponents<Model, Material, Transform>();

	std::string lastUsedPipeline;
	PipelineContainer* pipelineContainer = nullptr;
	VkDescriptorSet lastBoundDescriptorSet = VK_NULL_HANDLE;

	// Sort entities by pipeline name
	std::sort(modelEntities.begin(), modelEntities.end(), [](const auto& a, const auto& b) {
		return a->GetComponent<Model>().modelResource->pipeLineName < b->GetComponent<Model>().modelResource->pipeLineName;
	});

	for (const auto& entity: modelEntities)
	{
		ModelResource* model = entity->GetComponent<Model>().modelResource;
		MaterialResource* material = entity->GetComponent<Material>().materialResource;
		Transform& transform = entity->GetComponent<Transform>();

		debugUtils.BeginDebugMarker(cmd, ("Process Model: " + entity->GetName()).c_str(), debugUtil_StartDrawColour);

		std::string pipelineName = model->pipeLineName;
		if (pipelineName != lastUsedPipeline)
		{
			pipelineContainer = BindPipeline(disp, cmd, modelManager, pipelineName, debugUtils);
			if (!pipelineContainer)
			{
				debugUtils.EndDebugMarker(cmd);
				continue;
			}
			lastUsedPipeline = pipelineName;
			lastBoundDescriptorSet = VK_NULL_HANDLE; // Reset bound descriptor set when pipeline changes
		}

		if (pipelineContainer)
		{
			BindDescriptorSets(disp, cmd, *pipelineContainer, descriptorManager, material, entityManager, entity.get(), lastBoundDescriptorSet, allocator);
		}

		UpdatePushConstants(disp, cmd, *pipelineContainer, transform, debugUtils);

		debugUtils.BeginDebugMarker(cmd, "Draw Model", debugUtil_DrawModelColour);
		modelManager.DrawModel(disp, cmd, *model);
		debugUtils.EndDebugMarker(cmd);

		debugUtils.EndDebugMarker(cmd);
	}

	debugUtils.EndDebugMarker(cmd);
}

void Renderer::UpdateCommonBuffers(VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, Scene* scene)
{
	debugUtils.BeginDebugMarker(cmd, "Update Common Buffers", debugUtil_UpdateLightBufferColour);

	// Shader Debug Buffer
	if (m_shaderDebugBuffer == VK_NULL_HANDLE)
	{
		SlimeUtil::CreateBuffer("ShaderDebug", allocator, sizeof(ShaderDebug), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_shaderDebugBuffer, m_shaderDebugAllocation);
	}

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

void Renderer::BindDescriptorSets(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineContainer& pipelineContainer, DescriptorManager& descriptorManager, MaterialResource* material, EntityManager& entityManager, Entity* entity, VkDescriptorSet& lastBoundDescriptorSet, VmaAllocator allocator)
{
	auto& descSets = pipelineContainer.descriptorSets;

	// Ensure we have the correct number of descriptor sets for this pipeline
	if (descSets.size() != pipelineContainer.descriptorSetLayouts.size())
	{
		spdlog::error("Mismatch in descriptor set count for pipeline: {}", pipelineContainer.name);
		return;
	}
	
	if (m_boundDescriptorSet == descSets[0])
	{
		return;
	}
	
	m_boundDescriptorSet = descSets[0];

	// Bind all descriptor sets for the current pipeline
	disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContainer.pipelineLayout, 0, descSets.size(), descSets.data(), 0, nullptr);
	lastBoundDescriptorSet = descSets[0];

	// Update descriptor sets based on the pipeline's layout
	for (size_t i = 0; i < descSets.size(); ++i)
	{
		UpdateDescriptorSet(descriptorManager, descSets[i], pipelineContainer, i, material, entityManager, entity, allocator);
	}
}

void Renderer::UpdateDescriptorSet(DescriptorManager& descriptorManager, VkDescriptorSet descSet, PipelineContainer& pipelineContainer, size_t setIndex, MaterialResource* material, EntityManager& entityManager, Entity* entity, VmaAllocator allocator)
{
	VkDescriptorSetLayout layout = pipelineContainer.descriptorSetLayouts[setIndex];

	if (setIndex == 0) // Common descriptor set
	{
		BindCommonDescriptors(entityManager, descriptorManager, descSet, layout, allocator);
	}
	else if (setIndex == 1 && entity->GetComponent<Material>().type != MaterialType::LINE) // Material descriptor set
	{
		BindMaterialDescriptors(descriptorManager, descSet, layout, material, allocator);
	}
	// Add more conditions for other descriptor sets if needed
}

void Renderer::BindCommonDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet descSet, VkDescriptorSetLayout layout, VmaAllocator allocator)
{
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();

	// Always bind the camera buffer
	descriptorManager.BindBuffer(descSet, 0, camera.GetCameraUBOBuffer(), 0, sizeof(CameraUBO));

	PointLightObject& light = entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>();
	descriptorManager.BindBuffer(descSet, 1, light.buffer, 0, sizeof(light.light));

	SlimeUtil::CopyStructToBuffer(m_shaderDebug, allocator, m_shaderDebugAllocation);
	descriptorManager.BindBuffer(descSet, 2, m_shaderDebugBuffer, 0, sizeof(ShaderDebug));
}


void Renderer::BindMaterialDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet descSet, VkDescriptorSetLayout layout, MaterialResource* material, VmaAllocator allocator)
{
	SlimeUtil::CopyStructToBuffer(material->config, allocator, material->configAllocation);
	descriptorManager.BindBuffer(descSet, 0, material->configBuffer, 0, sizeof(MaterialResource::Config));

	if (material->albedoTex)
	{
		descriptorManager.BindImage(descSet, 1, material->albedoTex->imageView, material->albedoTex->sampler);
	}
	if (material->normalTex)
	{
		descriptorManager.BindImage(descSet, 2, material->normalTex->imageView, material->normalTex->sampler);
	}
	if (material->metallicTex)
	{
		descriptorManager.BindImage(descSet, 3, material->metallicTex->imageView, material->metallicTex->sampler);
	}
	if (material->roughnessTex)
	{
		descriptorManager.BindImage(descSet, 4, material->roughnessTex->imageView, material->roughnessTex->sampler);
	}
	if (material->aoTex)
	{
		descriptorManager.BindImage(descSet, 5, material->aoTex->imageView, material->aoTex->sampler);
	}
}

void Renderer::UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineContainer& pipelineContainer, Transform& transform, VulkanDebugUtils& debugUtils)
{
	m_mvp.model = transform.GetModelMatrix();
	m_mvp.normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_mvp.model)));
	disp.cmdPushConstants(cmd, pipelineContainer.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_mvp), &m_mvp);
	debugUtils.InsertDebugMarker(cmd, "Update Push Constants", debugUtil_White);
}

void Renderer::CleanUp(VmaAllocator allocator)
{
	vmaDestroyBuffer(allocator, m_shaderDebugBuffer, m_shaderDebugAllocation);
}

bool Renderer::LayoutIncludesLightBuffer(VkDescriptorSetLayout layout)
{
	if (layout == VK_NULL_HANDLE)
	{
		return false;
	}
	return true;
}

bool Renderer::LayoutIncludesShaderDebugBuffer(VkDescriptorSetLayout layout)
{
	if (layout == VK_NULL_HANDLE)
	{
		return false;
	}
	
	return true;
}
