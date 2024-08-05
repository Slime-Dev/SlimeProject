#include "RenderPasses/GridRenderPass.h"

#include "Camera.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "Scene.h"
#include "ShaderManager.h"
#include <VkBootstrapDispatch.h>

GridRenderPass::GridRenderPass(vkb::DispatchTable& disp, ShaderManager* shaderManager, VulkanDebugUtils* debugUtils)
{
	name = "Grid";

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> gridShaderPaths = {
		{ResourcePathManager::GetShaderPath("grid.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT},
        {ResourcePathManager::GetShaderPath("grid.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	// Load and parse shaders
	std::vector<ShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (const auto& [shaderPath, shaderStage]: gridShaderPaths)
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

	pipelineGenerator.SetName("Grid");

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &colorFormat;
	renderingInfo.depthAttachmentFormat = depthFormat;
	renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	pipelineGenerator.SetRenderingInfo(renderingInfo);

	pipelineGenerator.SetShaderStages(shaderStages);

	// Set vertex input state only if vertex shader is present
	if (std::find_if(gridShaderPaths.begin(), gridShaderPaths.end(), [](const auto& pair) { return pair.second == VK_SHADER_STAGE_VERTEX_BIT; }) != gridShaderPaths.end())
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
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	pipelineGenerator.SetRasterizationState(rasterizer);

	pipelineGenerator.SetDefaultMultisampleState();

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = false;
	depthStencil.depthWriteEnable = false;
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

	PipelineConfig config = pipelineGenerator.Build(disp, *debugUtils);

	spdlog::debug("Created pipeline: {}", "Grid");
}

void GridRenderPass::Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils& debugUtils)
{
}

void GridRenderPass::Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera)
{
	disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	struct GridPushConstants
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec3 pos;
	} pushConstants;

	pushConstants.view = camera->GetViewMatrix();
	pushConstants.projection = camera->GetProjectionMatrix();
	pushConstants.pos = camera->GetPosition();

	disp.cmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GridPushConstants), &pushConstants);

	disp.cmdDraw(cmd, 6, 1, 0, 0);
}

void GridRenderPass::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{
}

VkRenderingInfo GridRenderPass::GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) const
{
	VkRenderingAttachmentInfo colorAttachmentInfo = {};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachmentInfo.pNext = VK_NULL_HANDLE;
	colorAttachmentInfo.imageView = swapchainImageView;
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.clearValue = {
		.color = {m_clearColor.r, m_clearColor.g, m_clearColor.b, 0.0f}
	};

	VkRenderingAttachmentInfo depthAttachmentInfo = {};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.pNext = VK_NULL_HANDLE;
	depthAttachmentInfo.imageView = depthImageView;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue.depthStencil.depth = 1.f;

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = VK_NULL_HANDLE;
	renderingInfo.renderArea = {
		.offset = {0, 0},
          .extent = swapchain.extent
	};
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	return renderingInfo;
}
