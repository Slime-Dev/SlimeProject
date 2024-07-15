#include "scene.h"

#include "Engine.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"
#include "spdlog/spdlog.h"

Scene::Scene(Engine& engine)
      : m_engine(engine), m_camera(engine.GetCamera()), m_window(m_engine.GetWindow())
{
}

int Scene::Setup(ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	ResourcePathManager resourcePaths;
	spdlog::info("Root directory: {}", resourcePaths.GetRootDirectory());

	// MODEL LOADING
	m_bunny = modelManager.LoadModel(m_engine.GetAllocator(), "stanford-bunny.obj", "basic");
	m_bunny->model = glm::translate(m_bunny->model, glm::vec3(1.75f, -0.75f, -2.0f));
	m_bunny->model = glm::scale(m_bunny->model, glm::vec3(10.0f, 10.0f, 10.0f));

	m_bunny->isActive = true;

	m_cube = modelManager.LoadModel(m_engine.GetAllocator(), "cube.obj", "basic");
	m_cube->model = glm::translate(m_cube->model, glm::vec3(-4.0f, 0.0f, -5.0f));

	m_suzanne = modelManager.LoadModel(m_engine.GetAllocator(), "suzanne.obj", "basic");
	m_suzanne->model = glm::translate(m_cube->model, glm::vec3(0.0f, 0.0f, -5.0f));

	// Load and parse shaders
	std::string vertShaderPath = resourcePaths.GetShaderPath("basic.vert.spv");
	std::string fragShaderPath = resourcePaths.GetShaderPath("basic.frag.spv");

	auto vertexShaderModule = shaderManager.LoadShader(m_engine.GetDevice(), vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
	auto fragmentShaderModule = shaderManager.LoadShader(m_engine.GetDevice(), fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto vertexResources = shaderManager.ParseShader(vertexShaderModule);
	auto fragmentResources = shaderManager.ParseShader(fragmentShaderModule);
	auto combinedResources = shaderManager.CombineResources({ vertexShaderModule, fragmentShaderModule });

	// Set up descriptor set layout
	auto descriptorSetLayouts = shaderManager.CreateDescriptorSetLayouts(m_engine.GetDevice(), combinedResources);

	PipelineGenerator pipelineGenerator(m_engine);
	pipelineGenerator.SetName("Basic");
	pipelineGenerator.SetShaderModules(vertexShaderModule, fragmentShaderModule);
	pipelineGenerator.SetVertexInputState(combinedResources.attributeDescriptions, combinedResources.bindingDescriptions);
	pipelineGenerator.SetDescriptorSetLayouts(descriptorSetLayouts);
	pipelineGenerator.SetPushConstantRanges(combinedResources.pushConstantRanges);
	pipelineGenerator.Generate();

	// Descriptor set layout
	descriptorManager.AddDescriptorSetLayouts(descriptorSetLayouts);

	std::vector<VkDescriptorSet> descriptorSets;
	for (int i = descriptorSetLayouts.size() - 1; i >= 0; i--)
	{
		descriptorSets.push_back(descriptorManager.AllocateDescriptorSet(i));
	}

	// Sort the descriptor sets in the order of the layout
	std::reverse(descriptorSets.begin(), descriptorSets.end());

	pipelineGenerator.SetDescriptorSets(descriptorSets);

	m_engine.GetPipelines()["basic"] = pipelineGenerator.GetPipelineContainer();
	return 0;
}

void Scene::Update(float dt, const InputManager* inputManager)
{
	m_time += dt;
	//m_bunny->model = glm::rotate(m_bunny->model, dt, glm::vec3(0.0f, 1.0f, 0.0f));
	m_cube->model = glm::rotate(m_cube->model, dt, glm::vec3(0.0f, -1.0f, 0.0f));

	// If right mouse button is pressed, disable the cursor and rotate the camera
	if (inputManager->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
	{
		m_window->SetCursorMode(GLFW_CURSOR_DISABLED);
		auto [mouseX, mouseY] = inputManager->GetMouseDelta();
		float yaw = 0.1f * mouseX;
		float pitch = (mouseY * 0.1f) * -1.0f;
		m_camera.Rotate(yaw, pitch);
	}
	else
	{
		m_window->SetCursorMode(GLFW_CURSOR_NORMAL);
	}

	float speed = 2.0f;
	if (inputManager->IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
	{
		speed = 5.0f;
	}
	if (inputManager->IsKeyPressed(GLFW_KEY_W))
	{
		m_camera.MoveForward(dt * speed);
	}
	if (inputManager->IsKeyPressed(GLFW_KEY_S))
	{
		m_camera.MoveForward(-(dt * speed));
	}
	if (inputManager->IsKeyPressed(GLFW_KEY_A))
	{
		m_camera.MoveRight(-(dt * speed));
	}
	if (inputManager->IsKeyPressed(GLFW_KEY_D))
	{
		m_camera.MoveRight(dt * speed);
	}
	if (inputManager->IsKeyPressed(GLFW_KEY_SPACE))
	{
		m_camera.MoveUp(dt * speed);
	}
	if (inputManager->IsKeyPressed(GLFW_KEY_LEFT_CONTROL))
	{
		m_camera.MoveUp(-(dt * speed));
	}

	// Escape key
	if (inputManager->IsKeyJustPressed(GLFW_KEY_ESCAPE))
	{
		m_window->Close();
	}
}

void Scene::Render()
{
	// Implement render logic here
}
