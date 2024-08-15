#include "RenderPasses/MainRenderPass.h"

#include <glm/common.hpp>

#include "Camera.h"
#include "DescriptorManager.h"
#include "Entity.h"
#include "MaterialManager.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "RenderPasses/ShadowRenderPass.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"
#include "ShadowSystem.h"
#include "VulkanUtil.h"

MainRenderPass::MainRenderPass(std::shared_ptr<ShadowRenderPass> shadowPass, MaterialManager* materialManager, ModelManager* modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, DescriptorManager* descriptorManager)
      : m_shadowPass(shadowPass), m_materialManager(materialManager), m_modelManager(modelManager), m_allocator(allocator), m_commandPool(commandPool), m_graphicsQueue(graphicsQueue), m_descriptorManager(descriptorManager)
{
	name = "Main Pass";
}

void MainRenderPass::Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils)
{
	m_debugUtils = &debugUtils;

	// Set up a basic pipeline
	std::vector<std::pair<std::string, VkShaderStageFlagBits>> shaderPaths = {
		{ResourcePathManager::GetShaderPath("basic.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT},
        {ResourcePathManager::GetShaderPath("basic.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT}
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
		.color = {m_clearColor.r, m_clearColor.g, m_clearColor.b, 0.0f}
	};

	m_depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	m_depthAttachmentInfo.pNext = VK_NULL_HANDLE;
	m_depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	m_depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
	m_depthAttachmentInfo.clearValue.depthStencil.depth = 1.f;

	m_descriptorManager->CreateSharedDescriptorSet(descriptorSetLayouts[0]);
	m_descriptorManager->CreateLightDescriptorSet(descriptorSetLayouts[1]);
}

void MainRenderPass::Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera, RenderPassManager* renderPassManager)
{
	SetupViewportAndScissor(disp, cmd, swapchain);
	SlimeUtil::SetupDepthTestingAndLineWidth(disp, cmd);

	UpdateCommonBuffers(cmd, scene);

	// I have removed the basic material models for now!
	EntityManager& entityManager = scene->m_entityManager;
	std::vector<std::shared_ptr<Entity>> modelEntities = entityManager.GetEntitiesWithComponents<Model, PBRMaterial, Transform>();

	// Get the shared descriptor set
	std::pair<VkDescriptorSet, VkDescriptorSetLayout> sharedDescriptorSet = m_descriptorManager->GetSharedDescriptorSet();
	std::pair<VkDescriptorSet, VkDescriptorSetLayout> lightDescriptorSet = m_descriptorManager->GetLightDescriptorSet();

	// Update shared descriptors before the loop
	UpdateSharedDescriptors(disp, sharedDescriptorSet.first, lightDescriptorSet.first, entityManager);

	disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &sharedDescriptorSet.first, 0, nullptr);
	disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &lightDescriptorSet.first, 0, nullptr);


	auto lightEntity = entityManager.GetEntityByName("Light");
	auto light = lightEntity->GetComponentShrPtr<DirectionalLight>();

	ShadowData* shadowData = m_shadowPass->GetShadowSystem().GetShadowData(light);
	if (!shadowData)
	{
		spdlog::critical("No Shadows??");
	}
	TextureResource* shadowMap = &shadowData->shadowMap;

	for (const auto& entity: modelEntities)
	{
		ModelResource* model = entity->GetComponent<Model>().modelResource;
		Transform& transform = entity->GetComponent<Transform>();
		PBRMaterial& pbrMaterial = entity->GetComponent<PBRMaterial>();

		m_debugUtils->BeginDebugMarker(cmd, ("Process Model: " + entity->GetName()).c_str(), debugUtil_StartDrawColour);

		// Update material buffer if dirty
		m_materialManager->UpdateMaterialBuffer(pbrMaterial.materialResource.get());

		// Get or create descriptor set
		VkDescriptorSet materialDescriptorSet = m_materialManager->GetOrCreateDescriptorSet(pbrMaterial.materialResource.get(), m_descriptorSetLayouts[2], shadowMap);
		disp.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1, &materialDescriptorSet, 0, nullptr);

		UpdatePushConstants(disp, cmd, transform);

		DrawModel(disp, cmd, *model);
		m_debugUtils->EndDebugMarker(cmd);
	}
}

void MainRenderPass::ImGuiDraw(vkb::DispatchTable disp)
{
	ImGui::Begin("Main Render Pass");

	ImGui::ColorEdit3("Clear Color", &m_clearColor.r);

	ImGui::End();
}

int MainRenderPass::DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model)
{
	VkDeviceSize offsets[] = { 0 };
	disp.cmdBindVertexBuffers(cmd, 0, 1, &model.vertexBuffer, offsets);
	disp.cmdBindIndexBuffer(cmd, model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	disp.cmdDrawIndexed(cmd, static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	return 0;
}

void MainRenderPass::SetupViewportAndScissor(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain)
{
	VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(swapchain.extent.width), static_cast<float>(swapchain.extent.height), 0.0f, 1.0f };
	VkRect2D scissor = {
		{0, 0},
        swapchain.extent
	};

	disp.cmdSetViewport(cmd, 0, 1, &viewport);
	disp.cmdSetScissor(cmd, 0, 1, &scissor);
}

void MainRenderPass::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	disp.destroyPipeline(m_pipeline, nullptr);
	disp.destroyPipelineLayout(m_pipelineLayout, nullptr);
}

VkRenderingInfo* MainRenderPass::GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView)
{
	m_colorAttachmentInfo.clearValue = {
		.color = {m_clearColor.r, m_clearColor.g, m_clearColor.b, 0.0f}
	};

	m_colorAttachmentInfo.imageView = swapchainImageView;
	m_depthAttachmentInfo.imageView = depthImageView;

	m_renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	m_renderingInfo.pNext = VK_NULL_HANDLE;
	m_renderingInfo.flags = 0;
	m_renderingInfo.renderArea = {
		.offset = {0, 0},
          .extent = swapchain.extent
	};
	m_renderingInfo.layerCount = 1;
	m_renderingInfo.colorAttachmentCount = 1;
	m_renderingInfo.pColorAttachments = &m_colorAttachmentInfo;
	m_renderingInfo.pDepthAttachment = &m_depthAttachmentInfo;

	return &m_renderingInfo;
}

void MainRenderPass::UpdateCommonBuffers(VkCommandBuffer& cmd, Scene* scene)
{
	m_debugUtils->BeginDebugMarker(cmd, "Update Common Buffers", debugUtil_UpdateLightBufferColour);

	EntityManager& entityManager = scene->m_entityManager;

	// LIGHT
	auto lightEntity = entityManager.GetEntityByName("Light");
	if (!lightEntity)
	{
		spdlog::critical("Light entity not found!");
		return;
	}

	DirectionalLight& light = lightEntity->GetComponent<DirectionalLight>();
	if (light.buffer == VK_NULL_HANDLE)
	{
		SlimeUtil::CreateBuffer("Light Buffer", m_allocator, light.GetBindingDataSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, light.buffer, light.allocation);
	}

	auto bindingData = light.GetBindingData();
	SlimeUtil::CopyStructToBuffer(bindingData, m_allocator, light.allocation);

	// CAMERA
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	camera.UpdateCameraUBO(m_allocator);
	SlimeUtil::CopyStructToBuffer(camera.GetCameraUBO(), m_allocator, camera.GetCameraUBOAllocation());

	m_debugUtils->EndDebugMarker(cmd);
}

void MainRenderPass::UpdateSharedDescriptors(vkb::DispatchTable& disp, VkDescriptorSet cameraSet, VkDescriptorSet lightSet, EntityManager& entityManager)
{
	Camera& camera = entityManager.GetEntityByName("MainCamera")->GetComponent<Camera>();
	SlimeUtil::BindBuffer(disp, cameraSet, 0, camera.GetCameraUBOBuffer(), 0, sizeof(CameraUBO));

	auto lightEntity = entityManager.GetEntityByName("Light");
	if (!lightEntity)
	{
		spdlog::critical("Light entity not found!");
		return;
	}

	auto light = lightEntity->GetComponentShrPtr<DirectionalLight>();
	SlimeUtil::BindBuffer(disp, lightSet, 0, light->buffer, 0, light->GetBindingDataSize());
}

void MainRenderPass::UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Transform& transform)
{
	m_mvp.model = transform.GetModelMatrix();
	m_mvp.normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_mvp.model)));
	disp.cmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_mvp), &m_mvp);
	m_debugUtils->InsertDebugMarker(cmd, "Update Push Constants", debugUtil_White);
}
