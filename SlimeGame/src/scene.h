#pragma once
#include "Engine.h"
#include <memory>

class PipelineGenerator;

class Scene {
public:
	Scene(Engine& engine);
	~Scene() = default;

	int Setup();
	void Update(float dt);
	void Render();

private:
	Engine& m_engine;
	Camera& m_camera;
	Window& m_window;

	std::unique_ptr<PipelineGenerator> m_pipelineGenerator;
	size_t m_descriptorSetLayoutIndex;
	VkDescriptorSet m_descriptorSet;

	ModelManager::ModelResource* m_bunny;
	ModelManager::ModelResource* m_cube;
	ModelManager::ModelResource* m_suzanne;

	float m_time = 0.0f;
};