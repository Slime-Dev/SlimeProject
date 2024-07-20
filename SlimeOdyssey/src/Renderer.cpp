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

	debugUtils.BeginDebugMarker(cmd, "Update Light Buffer", debugUtil_UpdateLightBufferColour);

	// Shader Debug
	if (m_shaderDebugBuffer == VK_NULL_HANDLE)
	{
		SlimeUtil::CreateBuffer("ShaderDebug", allocator, sizeof(ShaderDebug), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_shaderDebugBuffer, m_shaderDebugAllocation);
	}

	EntityManager& entityManager = scene->m_entityManager;

	// Light
	PointLightObject& light = entityManager.GetEntityByName("Light")->GetComponent<PointLightObject>();
	if (light.buffer == VK_NULL_HANDLE)
	{
		SlimeUtil::CreateBuffer("Light Buffer", allocator, sizeof(light.light), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, light.buffer, light.allocation);
	}

	SlimeUtil::CopyStructToBuffer(light.light, allocator, light.allocation);

	// Camera
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	camera.UpdateCameraUBO(allocator);
	SlimeUtil::CopyStructToBuffer(camera.GetCameraUBO(), allocator, camera.GetCameraUBOAllocation());

	debugUtils.EndDebugMarker(cmd);

	VkDescriptorSet boundDescriptorSet = VK_NULL_HANDLE;

	std::string lastUsedPipeline;
	PipelineContainer* pipelineContainer = nullptr;

	auto modelEntities = entityManager.GetEntitiesWithComponents<Model, Material, Transform>();
	for (const auto& entity: modelEntities)
	{
		ModelResource* model = entity->GetComponent<Model>().modelResource;
		MaterialResource* material = entity->GetComponent<Material>().materialResource;
		Transform& transform = entity->GetComponent<Transform>();

		debugUtils.BeginDebugMarker(cmd, ("Process Model: " + entity->GetName()).c_str(), debugUtil_StartDrawColour);

		std::string pipelineName = model->pipeLineName;
		if (pipelineName != lastUsedPipeline)
		{
			auto pipelineIt = modelManager.GetPipelines().find(pipelineName);
			if (pipelineIt == modelManager.GetPipelines().end())
			{
				spdlog::error("Pipeline not found: {}", pipelineName);
				debugUtils.EndDebugMarker(cmd);
				continue;
			}

			pipelineContainer = &pipelineIt->second;

			disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContainer->pipeline);
			debugUtils.InsertDebugMarker(cmd, "Bind Pipeline", debugUtil_White);
		}

		{
			if (!boundDescriptorSet)
			{
				auto descSets = pipelineContainer->descriptorSets;
				boundDescriptorSet = descSets[0];

				// Bind descriptor set
				disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContainer->pipelineLayout, 0, 2, descSets.data(), 0, nullptr);

				// Set Camera UBO buffer
				descriptorManager.BindBuffer(descSets[0], 0, camera.GetCameraUBOBuffer(), 0, sizeof(CameraUBO));

				// Set Light buffer
				descriptorManager.BindBuffer(descSets[0], 1, light.buffer, 0, sizeof(light.light));

				SlimeUtil::CopyStructToBuffer(m_shaderDebug, allocator, m_shaderDebugAllocation);
				descriptorManager.BindBuffer(descSets[0], 2, m_shaderDebugBuffer, 0, sizeof(ShaderDebug));

				// Set material buffer
				SlimeUtil::CopyStructToBuffer(material->config, allocator, material->configAllocation);
				descriptorManager.BindBuffer(descSets[1], 0, material->configBuffer, 0, sizeof(MaterialResource::Config));

				// Bind the sampler2D textures
				descriptorManager.BindImage(descSets[1], 1, material->albedoTex->imageView, material->albedoTex->sampler);
				descriptorManager.BindImage(descSets[1], 2, material->normalTex->imageView, material->normalTex->sampler);
				descriptorManager.BindImage(descSets[1], 3, material->metallicTex->imageView, material->metallicTex->sampler);
				descriptorManager.BindImage(descSets[1], 4, material->roughnessTex->imageView, material->roughnessTex->sampler);
				descriptorManager.BindImage(descSets[1], 5, material->aoTex->imageView, material->aoTex->sampler);
			}
		}

		m_mvp.model = transform.GetModelMatrix();
		m_mvp.normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_mvp.model)));
		disp.cmdPushConstants(cmd, pipelineContainer->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_mvp), &m_mvp);
		debugUtils.InsertDebugMarker(cmd, "Update Push Constants", debugUtil_White);

		debugUtils.BeginDebugMarker(cmd, "Draw Model", debugUtil_DrawModelColour);
		modelManager.DrawModel(disp, cmd, *model);
		debugUtils.EndDebugMarker(cmd); // End "Draw Model"

		debugUtils.EndDebugMarker(cmd); // End "Process Model: [name]"
	}

	debugUtils.EndDebugMarker(cmd); // End "Draw Models"
}

void Renderer::CleanUp(VmaAllocator allocator)
{
	vmaDestroyBuffer(allocator, m_shaderDebugBuffer, m_shaderDebugAllocation);
}
