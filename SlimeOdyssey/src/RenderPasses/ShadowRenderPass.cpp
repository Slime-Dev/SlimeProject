#include "RenderPasses/ShadowRenderPass.h"
#include "Camera.h"
#include "DescriptorManager.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "RenderPasses/ShadowRenderPass.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"
#include "ShadowSystem.h"
#include "VulkanUtil.h"


ShadowRenderPass::ShadowRenderPass(ModelManager& modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue)
      : m_modelManager(modelManager), m_allocator(allocator), m_commandPool(commandPool), m_graphicsQueue(graphicsQueue)
{
	name = "Shadow Pass";
}

void ShadowRenderPass::Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils)
{
	m_shadowSystem.Initialize(disp, allocator, debugUtils);

	// Setup pipeline
	const std::string pipelineName = "ShadowMap";

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> shaderPaths = {
		{ResourcePathManager::GetShaderPath("shadowmap.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT},
        {ResourcePathManager::GetShaderPath("shadowmap.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT}
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

	PipelineGenerator pipelineGenerator;

	pipelineGenerator.SetName(pipelineName);

	// Set up rendering info for dynamic rendering
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.depthAttachmentFormat = depthFormat;
	renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	renderingInfo.colorAttachmentCount = 0;
	renderingInfo.pColorAttachmentFormats = nullptr;
	pipelineGenerator.SetRenderingInfo(renderingInfo);

	pipelineGenerator.SetShaderStages(shaderStages);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(combinedResources.bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = combinedResources.bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(combinedResources.attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = combinedResources.attributeDescriptions.data();
	pipelineGenerator.SetVertexInputState(vertexInputInfo);

	pipelineGenerator.SetDefaultInputAssembly();
	pipelineGenerator.SetDefaultViewportState();

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	pipelineGenerator.SetRasterizationState(rasterizer);

	// Set up multisample state
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	pipelineGenerator.SetMultisampleState(multisampling);

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	pipelineGenerator.SetDepthStencilState(depthStencil);

	// For shadow mapping, we typically don't use color attachments
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 0;
	colorBlending.pAttachments = nullptr;
	pipelineGenerator.SetColorBlendState(colorBlending);

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT, VK_DYNAMIC_STATE_LINE_WIDTH };
	pipelineGenerator.SetDynamicState({ .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() });

	pipelineGenerator.SetPushConstantRanges(combinedResources.pushConstantRanges);

	PipelineConfig config = pipelineGenerator.Build(disp, debugUtils);
	m_pipeline = config.pipeline;
	m_pipelineLayout = config.pipelineLayout;

	spdlog::debug("Created the Shadow Map Pipeline");
}

void ShadowRenderPass::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	m_shadowSystem.Cleanup(disp, allocator);
	disp.destroyPipeline(m_pipeline, nullptr);
	disp.destroyPipelineLayout(m_pipelineLayout, nullptr);
}

void ShadowRenderPass::Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera, RenderPassManager* renderPassManager)
{
	std::vector<Light*> lights;

	// ENTT registry is used to iterate over all entities with a specific component
	entt::registry& registry = scene->m_entityRegistry;
	
	// Get and add all point lights to lights
	auto pointLightView = registry.view<PointLight>();
	for (auto entity : pointLightView)
	{
		auto& pointLight = pointLightView.get<PointLight>(entity);
		lights.push_back(&pointLight);
	}

	// Get and add all directional lights to lights
	auto directionalLightView = registry.view<DirectionalLight>();
	for (auto entity : directionalLightView)
	{
		auto& directionalLight = directionalLightView.get<DirectionalLight>(entity);
		lights.push_back(&directionalLight);
	}

	auto drawModelsFn = [this](vkb::DispatchTable d, VulkanDebugUtils& du, VkCommandBuffer& c, ModelManager& mm, Scene* s) { this->DrawModelsForShadowMap(d, du, c, mm, s); };
	m_shadowSystem.UpdateShadowMaps(disp, cmd, m_modelManager, m_allocator, m_commandPool, m_graphicsQueue, m_debugUtils, scene, drawModelsFn, lights, camera);
}

VkRenderingInfo* ShadowRenderPass::GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView)
{
	return VK_NULL_HANDLE;
}

ShadowSystem& ShadowRenderPass::GetShadowSystem()
{
	return m_shadowSystem;
}

void ShadowRenderPass::ImGuiDraw(vkb::DispatchTable disp)
{
	m_shadowSystem.RenderShadowMapInspector(disp, m_allocator, m_commandPool, m_graphicsQueue, m_modelManager, m_debugUtils);
}

void ShadowRenderPass::DrawModelsForShadowMap(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VkCommandBuffer& cmd, ModelManager& modelManager, Scene* scene)
{
	entt::registry& registry = scene->m_entityRegistry;

	std::vector<entt::entity> modelEntities;
	registry.view<Model, PBRMaterial, Transform>().each([&modelEntities](auto entity, Model& model, PBRMaterial& material, Transform& transform) // Maybe remove the material
	{
		modelEntities.push_back(entity);
	});

	// Get the light entity and its transform
	entt::entity lightEntity = registry.view<DirectionalLight>().front();
	Light& light = registry.get<DirectionalLight>(lightEntity);
	
	entt::entity cameraEntity = registry.view<Camera>().front();
	Camera& camera = registry.get<Camera>(cameraEntity);

	// Bind the shadow map pipeline
	disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	VkBool32 depthTestEnable = VK_TRUE;
	disp.cmdSetDepthTestEnable(cmd, depthTestEnable);

	VkBool32 depthWriteEnable = VK_TRUE;
	disp.cmdSetDepthWriteEnable(cmd, depthWriteEnable);

	VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
	disp.cmdSetDepthCompareOp(cmd, depthCompareOp);

	for (const auto& entity: modelEntities)
	{
		ModelResource* model = registry.get<Model>(entity).modelResource;
		Transform& transform = registry.get<Transform>(entity);

		debugUtils.BeginDebugMarker(cmd, ("Process Model for Shadow: "), debugUtil_StartDrawColour); // TODO: Update once we have names again

		// Update push constants for shadow mapping
		struct ShadowMapPushConstants
		{
			glm::mat4 lightSpaceMatrix;
			glm::mat4 modelMatrix;
		} pushConstants;

		pushConstants.lightSpaceMatrix = light.GetLightSpaceMatrix();
		pushConstants.modelMatrix = transform.GetModelMatrix();

		disp.cmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowMapPushConstants), &pushConstants);

		debugUtils.BeginDebugMarker(cmd, "Draw Model for Shadow", debugUtil_DrawModelColour);
		DrawModel(disp, cmd, *model);
		debugUtils.EndDebugMarker(cmd);

		debugUtils.EndDebugMarker(cmd);
	}
}

int ShadowRenderPass::DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model)
{
	VkDeviceSize offsets[] = { 0 };
	disp.cmdBindVertexBuffers(cmd, 0, 1, &model.vertexBuffer, offsets);
	disp.cmdBindIndexBuffer(cmd, model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	disp.cmdDrawIndexed(cmd, static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	return 0;
}
