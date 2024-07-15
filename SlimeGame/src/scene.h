#pragma once

class ModelManager;
class ShaderManager;
class DescriptorManager;
class InputManager;
struct PipelineContainer;
class SlimeWindow;
class Camera;
class Engine;
struct ModelResource;
class PipelineGenerator;

class Scene
{
public:
	Scene(Engine& engine);
	~Scene() = default;

	virtual int Setup(ModelManager& modelManager , ShaderManager& shaderManager, DescriptorManager& descriptorManager);
	virtual void Update(float dt, const InputManager* inputManager);
	virtual void Render();

protected:
	Engine& m_engine;
	Camera& m_camera;
	SlimeWindow* m_window = nullptr;

	PipelineContainer* m_pipeline = nullptr;

	ModelResource* m_bunny;
	ModelResource* m_cube;
	ModelResource* m_suzanne;

	float m_time = 0.0f;
};
