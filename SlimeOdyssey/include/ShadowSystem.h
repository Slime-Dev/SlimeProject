#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include "Light.h"
#include "Camera.h"
#include "Model.h"

class VulkanDebugUtils;
class ModelManager;

namespace vkb {
    struct DispatchTable;
    struct Swapchain;
}

class ShadowSystem {
public:
    ShadowSystem() = default;
    ~ShadowSystem() = default;

    void Initialize(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils);
    void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator);

    void UpdateShadowMaps(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, VulkanDebugUtils& debugUtils, const std::vector<Light*>& lights, const Camera& camera);
    
    TextureResource GetShadowMap(const Light* light) const;
    glm::mat4 GetLightSpaceMatrix(const Light* light) const;

    void SetShadowMapResolution(uint32_t width, uint32_t height);
    void SetShadowNearPlane(float near);
    void SetShadowFarPlane(float far);

    float GetShadowMapPixelValue(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, const Light* light, int x, int y) const;

private:
    struct ShadowData {
        TextureResource shadowMap;
        glm::mat4 lightSpaceMatrix;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
        VkDeviceSize stagingBufferSize = 0;
    };

    std::unordered_map<const Light*, ShadowData> m_shadowData;

    uint32_t m_shadowMapWidth = 4096;
    uint32_t m_shadowMapHeight = 4096;
    float m_shadowNear = 0.1f;
    float m_shadowFar = 120.0f;

    void CreateShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils, const Light* light);
    void CleanupShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, const Light* light);
    void GenerateShadowMap(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, const Light* light, const Camera& camera);
    void CalculateLightSpaceMatrix(const Light* light, const Camera& camera);

    std::vector<glm::vec3> CalculateFrustumCorners(float fov, float aspect, float near, float far, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, const glm::vec3& right) const;
    void CalculateFrustumSphere(const std::vector<glm::vec3>& frustumCorners, glm::vec3& center, float& radius) const;
};