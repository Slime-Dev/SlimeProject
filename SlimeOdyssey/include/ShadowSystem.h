#pragma once

#include <cstdint>
#include <functional>
#include <glm/fwd.hpp>
#include <imgui.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "Camera.h"
#include "Light.h"
#include "Model.h"
#include "ModelManager.h"

class VulkanDebugUtils;
class ModelManager;
class Scene;

namespace vkb
{
	struct DispatchTable;
	struct Swapchain;
} // namespace vkb

struct ShadowData
{
	ImTextureID textureId = 0;
	TextureResource shadowMap;
	glm::mat4 lightSpaceMatrix;
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
	VkDeviceSize stagingBufferSize = 0;

	// For optimizing the recalculations
	glm::vec3 lastCameraPosition;
	float frustumRadius;
};

class ShadowSystem
{
public:
	ShadowSystem() = default;
	~ShadowSystem() = default;

	void Initialize(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils);
	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator);

	bool UpdateShadowMaps(vkb::DispatchTable& disp,
	        VkCommandBuffer& cmd,
	        ModelManager& modelManager,
	        VmaAllocator allocator,
	        VkCommandPool commandPool,
	        VkQueue graphicsQueue,
	        VulkanDebugUtils& debugUtils,
	        Scene* scene,
	        std::function<void(vkb::DispatchTable, VulkanDebugUtils&, VkCommandBuffer&, ModelManager&, Scene*)> drawModels,
	        const std::vector<std::shared_ptr<Light>>& lights,
	        Camera* camera);

	ShadowData* GetShadowData(const std::shared_ptr<Light> light);
	glm::mat4 GetLightSpaceMatrix(const std::shared_ptr<Light> light) const;

	void SetShadowMapResolution(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils, uint32_t width, uint32_t height, bool reconstructImmediately = false);
	void ReconstructShadowMaps(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils);

	void SetShadowNearPlane(float near);
	void SetShadowFarPlane(float far);

	void SetDirectionalLightDistance(float distance);
	float GetDirectionalLightDistance() const;

	float GetShadowMapPixelValue(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, const std::shared_ptr<Light> light, int x, int y) const;

	void RenderShadowMapInspector(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, VulkanDebugUtils& debugUtils);

private:
	std::unordered_map<std::shared_ptr<Light>, ShadowData> m_shadowData;

	float m_directionalLightDistance = 100.0f;

	uint32_t m_pendingShadowMapWidth = 0;
	uint32_t m_pendingShadowMapHeight = 0;
	bool m_shadowMapNeedsReconstruction = false;

	uint32_t m_shadowMapWidth = 4096;
	uint32_t m_shadowMapHeight = 4096;
	float m_shadowNear = 0.1f;
	float m_shadowFar = 120.0f;

	void CreateShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils, const std::shared_ptr<Light> light);
	void CleanupShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, const std::shared_ptr<Light> light);
	void GenerateShadowMap(vkb::DispatchTable& disp,
	        VkCommandBuffer& cmd,
	        ModelManager& modelManager,
	        VmaAllocator allocator,
	        VkCommandPool commandPool,
	        VkQueue graphicsQueue,
	        VulkanDebugUtils& debugUtils,
	        Scene* scene,
	        std::function<void(vkb::DispatchTable, VulkanDebugUtils&, VkCommandBuffer&, ModelManager&, Scene*)> drawModels,
	        const std::shared_ptr<Light> light,
	        const Camera* camera);
	void CalculateLightSpaceMatrix(const std::shared_ptr<Light> light, const Camera* camera);

	std::vector<glm::vec3> CalculateFrustumCorners(float fov, float aspect, float near, float far, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, const glm::vec3& right) const;
	void CalculateFrustumSphere(const std::vector<glm::vec3>& frustumCorners, glm::vec3& center, float& radius) const;
};
