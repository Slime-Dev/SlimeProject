#pragma once
#include "Scene.h"
#include <glm/glm.hpp>
#include <memory>

class VulkanContext;
class SlimeWindow;
class InputManager;
class ModelManager;
class ShaderManager;
class DescriptorManager;

class DebugScene : public Scene
{
public:
    DebugScene(SlimeWindow* window);
	int Enter(VulkanContext& vulkanContext, ModelManager& modelManager) override;
    void Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager) override;
    void Render() override;
    void Exit(VulkanContext& vulkanContext, ModelManager& modelManager) override;

private:
	void SetupShaders(VulkanContext& vulkanContext, ModelManager& modelManager, ShaderManager& shaderManager, DescriptorManager& descriptorManager);
    void InitializeDebugObjects(VulkanContext& vulkanContext, ModelManager& modelManager);
    void UpdateFlyCam(float dt, const InputManager* inputManager);

	void CreateCube(ModelResource* mesh, const glm::vec3& position, const glm::vec3& scale, std::shared_ptr<PBRMaterialResource> material);
	void CreateLargeCube(ModelResource* mesh, const glm::vec3& position, const glm::vec3& scale, std::shared_ptr<PBRMaterialResource> material);

    std::vector<std::shared_ptr<PBRMaterialResource>> m_pbrMaterials;
	std::vector<std::shared_ptr<BasicMaterialResource>> m_basicMaterials;
	std::vector<std::shared_ptr<Model>> m_models;

    // Debug camera controls
    bool m_cameraMouseControl = true;
    float m_manualYaw = 0.0f;
    float m_manualPitch = 10.0f;
    float m_manualDistance = 10.0f;
    glm::vec3 m_flyCamPosition = glm::vec3(0.0f, 5.0f, -10.0f);
    float m_flyCamYaw = 0.0f;
    float m_flyCamPitch = 0.0f;
    bool m_rightMousePressed = false;
	float m_cameraSpeed = 10.0f;

    SlimeWindow* m_window;
};