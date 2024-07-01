#include "vulkanhelper.h"
#include "vulkanGraphicsPipeline.h"
#include "vulkanModel.h"
#include "vulkanwindow.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

#include <filesystem>
#include <string>

std::string getBuildDirectory()
{
	// Get the absolute path of the current source file
	auto srcPath = std::filesystem::absolute(__FILE__);
	return std::filesystem::path(srcPath).parent_path().parent_path().string();
}

int CreateDescriptorSetLayout(SlimeEngine::Init& init, SlimeEngine::RenderData& data)
{
	spdlog::info("Creating descriptor set layout...");

	// Define descriptor set layout bindings
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(init.device, &layoutInfo, nullptr, &data.descriptorSetLayout.emplace_back()) != VK_SUCCESS)
	{
		spdlog::error("Failed to create descriptor set layout!");
		return -1;
	}

	return 0;
}

int main()
{
	spdlog::set_level(spdlog::level::trace);
	spdlog::stdout_color_mt("console");
	std::string buildDir = getBuildDirectory();

	spdlog::info("Build directory: {}", buildDir);

	std::string shadersDir = buildDir + "/shaders";

	SlimeEngine::ShaderConfig shaderConfig =
	{
		.vertShaderPath = shadersDir + "/basic.vert.spv",
		.fragShaderPath = shadersDir + "/basic.frag.spv"
	 };

	SlimeEngine::ModelConfig modelConfig =
	{
		.modelPath = buildDir + "/models/Cube/Cube.gltf"
	};

	SlimeEngine::Init init;
	SlimeEngine::RenderData data;

	if (SlimeEngine::DeviceInit(init) != 0)
		return -1;
	if (SlimeEngine::CreateCommandPool(init, data) != 0)
		return -1;
	if (SlimeEngine::GetQueues(init, data) != 0)
		return -1;
	if (SlimeEngine::CreateSwapchain(init, data) != 0)
		return -1;

	if (CreateDescriptorSetLayout(init, data) != 0)
		return -1;

	if (SlimeEngine::CreateGraphicsPipeline(init, data, shaderConfig, {}, {}) != 0)
		return -1;

	// Load model
	data.models.emplace_back(SlimeEngine::loadModel(modelConfig.modelPath.c_str(), init));

	if (SlimeEngine::CreateRenderCommandBuffers(init, data) != 0)
		return -1;
	if (SlimeEngine::InitSyncObjects(init, data) != 0)
		return -1;

	// Main loop
	while (!SlimeEngine::ShouldClose(init.window))
	{
		SlimeEngine::PollEvents();
		SlimeEngine::RenderFrame(init, data);
	}

	// Cleanup
	SlimeEngine::Cleanup(init, data);

	return 0;
}