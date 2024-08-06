#include "RenderPasses/MainRenderPass.h"

#include "DescriptorManager.h"
#include "PipelineGenerator.h"
#include "RenderPasses/ShadowRenderPass.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"
#include "ShadowSystem.h"

MainRenderPass::MainRenderPass(ShadowRenderPass* shadowPass, ModelManager& modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, DescriptorManager& descriptorManager)
      : m_shadowPass(shadowPass), m_modelManager(modelManager), m_allocator(allocator), m_commandPool(commandPool), m_graphicsQueue(graphicsQueue), m_descriptorManager(descriptorManager)
{
	name = "Main Pass";
}

void MainRenderPass::Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils)
{
	// Set up a basic pipeline
	std::vector<std::pair<std::string, VkShaderStageFlagBits>> shaderPaths = {
		{ ResourcePathManager::GetShaderPath("basic.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT },
        { ResourcePathManager::GetShaderPath("basic.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT }
	};

	// Load and parse shaders
	std::vector<ShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (const auto& [shaderPath, shaderStage]: shaderPaths)
	{
		auto shaderModule = shaderManager->LoadShader(disp, shaderPath, shaderStage);
		shaderModules.push_back(shaderModule);
		shaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = shaderStage, .module = shaderModule.handle, .pName = "main" });
	}
	auto combinedResources = shaderManager->CombineResources(shaderModules);

	// Set up descriptor set layout
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = shaderManager->CreateDescriptorSetLayouts(disp, combinedResources);

	VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	PipelineGenerator pipelineGenerator;

	pipelineGenerator.SetName("pbr");

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &colorFormat;
	renderingInfo.depthAttachmentFormat = depthFormat;
	renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	pipelineGenerator.SetRenderingInfo(renderingInfo);

	pipelineGenerator.SetShaderStages(shaderStages);

	// Set vertex input state only if vertex shader is present
	if (std::find_if(shaderPaths.begin(), shaderPaths.end(), [](const auto& pair) { return pair.second == VK_SHADER_STAGE_VERTEX_BIT; }) != shaderPaths.end())
	{
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(combinedResources.bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = combinedResources.bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(combinedResources.attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = combinedResources.attributeDescriptions.data();
		pipelineGenerator.SetVertexInputState(vertexInputInfo);
	}

	pipelineGenerator.SetDefaultInputAssembly();
	pipelineGenerator.SetDefaultViewportState();

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	pipelineGenerator.SetRasterizationState(rasterizer);

	pipelineGenerator.SetDefaultMultisampleState();

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = true;
	depthStencil.depthWriteEnable = true;
	depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // Assuming reverse-Z
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	pipelineGenerator.SetDepthStencilState(depthStencil);

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	pipelineGenerator.SetColorBlendState(colorBlending);

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT, VK_DYNAMIC_STATE_LINE_WIDTH };
	pipelineGenerator.SetDynamicState({ .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() });

	pipelineGenerator.SetDescriptorSetLayouts(descriptorSetLayouts);
	pipelineGenerator.SetPushConstantRanges(combinedResources.pushConstantRanges);

	PipelineConfig config = pipelineGenerator.Build(disp, debugUtils);
	m_pipeline = config.pipeline;
	m_pipelineLayout = config.pipelineLayout;
	m_descriptorSetLayouts = config.descriptorSetLayouts;
	spdlog::debug("Created pipeline: {}", "pbr");

	m_colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	m_colorAttachmentInfo.pNext = VK_NULL_HANDLE;
	m_colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_colorAttachmentInfo.clearValue = {
		.color = { m_clearColor.r, m_clearColor.g, m_clearColor.b, 0.0f }
	};

	m_depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	m_depthAttachmentInfo.pNext = VK_NULL_HANDLE;
	m_depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	m_depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
	m_depthAttachmentInfo.clearValue.depthStencil.depth = 1.f;
}

void MainRenderPass::Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera)
{
	UpdateCommonBuffers(m_debugUtils, m_allocator, cmd, scene);

	// I have removed the basic material models for now!
	EntityManager& entityManager = scene->m_entityManager;
	std::vector<std::shared_ptr<Entity>> modelEntities = entityManager.GetEntitiesWithComponents<Model, PBRMaterial, Transform>();

	std::sort(modelEntities.begin(), modelEntities.end(), [](const auto& a, const auto& b) { return a->template GetComponent<Model>().modelResource->pipelineName < b->template GetComponent<Model>().modelResource->pipelineName; });

	// Get the shared descriptor set
	std::pair<VkDescriptorSet, VkDescriptorSetLayout> sharedDescriptorSet = m_descriptorManager.GetSharedDescriptorSet();

	// Update shared descriptors before the loop
	UpdateSharedDescriptors(sharedDescriptorSet.first, sharedDescriptorSet.second, entityManager, m_allocator);

	disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &sharedDescriptorSet.first, 0, nullptr);

	for (const auto& entity: modelEntities)
	{
		ModelResource* model = entity->GetComponent<Model>().modelResource;
		Transform& transform = entity->GetComponent<Transform>();

		m_debugUtils.BeginDebugMarker(cmd, ("Process Model: " + entity->GetName()).c_str(), debugUtil_StartDrawColour);

		// Bind descriptor sets
		for (size_t i = 1; i < m_descriptorSetLayouts.size(); i++)
		{
			VkDescriptorSet currentDescriptorSet = GetOrUpdateDescriptorSet(entityManager, entity.get(), i);
			disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, i, 1, &currentDescriptorSet, 0, nullptr);
		}

		UpdatePushConstants(disp, cmd, transform);

		m_debugUtils.BeginDebugMarker(cmd, "Draw Model", debugUtil_DrawModelColour);
		DrawModel(disp, cmd, *model);
		m_debugUtils.EndDebugMarker(cmd);

		m_debugUtils.EndDebugMarker(cmd);
	}

	m_debugUtils.EndDebugMarker(cmd);
}

int MainRenderPass::DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model)
{
	VkDeviceSize offsets[] = { 0 };
	disp.cmdBindVertexBuffers(cmd, 0, 1, &model.vertexBuffer, offsets);
	disp.cmdBindIndexBuffer(cmd, model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	disp.cmdDrawIndexed(cmd, static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	return 0;
}

void MainRenderPass::BindDescriptorSets(vkb::DispatchTable& disp, VkCommandBuffer& cmd, VkPipelineLayout layout, Scene* scene)
{
}

VkDescriptorSet MainRenderPass::GetShadowMapDescriptorSet(Scene* scene, ShadowSystem& shadowSystem)
{
	return VK_NULL_HANDLE;
}

void MainRenderPass::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	disp.destroyPipeline(m_pipeline, nullptr);
	disp.destroyPipelineLayout(m_pipelineLayout, nullptr);
}

VkRenderingInfo MainRenderPass::GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView)
{
	m_colorAttachmentInfo.imageView = swapchainImageView;
	m_depthAttachmentInfo.imageView = depthImageView;

	m_renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	m_renderingInfo.pNext = VK_NULL_HANDLE;
	m_renderingInfo.flags = 0;
	m_renderingInfo.renderArea = {
		.offset = { 0, 0 },
          .extent = swapchain.extent
	};
	m_renderingInfo.layerCount = 1;
	m_renderingInfo.colorAttachmentCount = 1;
	m_renderingInfo.pColorAttachments = &m_colorAttachmentInfo;
	m_renderingInfo.pDepthAttachment = &m_depthAttachmentInfo;

	return m_renderingInfo;
}
