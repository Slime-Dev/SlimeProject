#include "scene.h"

#include "spdlog/spdlog.h"
#include "ShaderManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "ModelManager.h"

Scene::Scene(Engine& engine) : m_engine(engine), m_camera(engine.GetCamera()), m_window(m_engine.GetWindow())
{
	m_window.LockMouse(true);
}

int Scene::Setup() {
    ResourcePathManager resourcePaths;
    spdlog::info("Root directory: {}", resourcePaths.GetRootDirectory());

    // Load and parse shaders
    ShaderManager& shaderManager = m_engine.GetShaderManager();
    std::string vertShaderPath = resourcePaths.GetShaderPath("basic.vert.spv");
    std::string fragShaderPath = resourcePaths.GetShaderPath("basic.frag.spv");

    auto vertexShaderModule = shaderManager.LoadShader(vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
    auto fragmentShaderModule = shaderManager.LoadShader(fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
    auto vertexResources = shaderManager.ParseShader(vertexShaderModule);
    auto fragmentResources = shaderManager.ParseShader(fragmentShaderModule);
    auto combinedResources = shaderManager.CombineResources({ vertexShaderModule, fragmentShaderModule });

    // Set up descriptor set layout
    auto descriptorSetLayout = shaderManager.CreateDescriptorSetLayout(combinedResources.descriptorSetLayoutBindings);

    m_pipelineGenerator = std::make_unique<PipelineGenerator>(m_engine.GetDevice());
    m_pipelineGenerator->SetShaderModules(vertexShaderModule, fragmentShaderModule);
    m_pipelineGenerator->SetVertexInputState(combinedResources.attributeDescriptions, combinedResources.bindingDescriptions);
    m_pipelineGenerator->SetDescriptorSetLayouts({ descriptorSetLayout });
    m_pipelineGenerator->SetPushConstantRanges(combinedResources.pushConstantRanges);
    m_pipelineGenerator->Generate();

    // MODEL LOADING
	ModelManager& modelManager = m_engine.GetModelManager();
    m_bunny = modelManager.LoadModel("stanford-bunny.obj", "basic");
    m_bunny->model = glm::translate(m_bunny->model, glm::vec3(1.75f, -0.75f, -2.0f));
	m_bunny->model = glm::scale(m_bunny->model, glm::vec3(10.0f, 10.0f, 10.0f));

	m_bunny->isActive = true;

    m_cube = modelManager.LoadModel("cube.obj", "basic");
    m_cube->model = glm::translate(m_cube->model, glm::vec3(-4.0f, 0.0f, -5.0f));

    // Descriptor set layout
    DescriptorManager& descriptorManager = m_engine.GetDescriptorManager();
    m_descriptorSetLayoutIndex = descriptorManager.AddDescriptorSetLayout(descriptorSetLayout);
    m_descriptorSet = descriptorManager.AllocateDescriptorSet(m_descriptorSetLayoutIndex);
    m_pipelineGenerator->SetDescriptorSets({ m_descriptorSet });

    m_engine.GetPipelines()["basic"] = std::move(m_pipelineGenerator);

    return 0;
}

void Scene::Update(float dt) {
	m_time += dt;
	m_bunny->model = glm::rotate(m_bunny->model, dt, glm::vec3(0.0f, 1.0f, 0.0f));
	m_cube->model = glm::rotate(m_cube->model, dt, glm::vec3(0.0f, -1.0f, 0.0f));

	if (m_window.MouseMoved())
	{
		auto [mouseX, mouseY] = m_window.GetMouseDelta();
		float yaw = mouseX * 0.1f;
		float pitch = (mouseY * 0.1f) * -1.0f;
		m_camera.rotate(yaw, pitch);
	}
}

void Scene::Render() {
    // Implement render logic here
}