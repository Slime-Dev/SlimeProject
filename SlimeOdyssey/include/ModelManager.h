#pragma once

#include <string>
#include <unordered_map>
#include <ResourcePathManager.h>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "vk_mem_alloc.h"

class Engine;
class ResourcePathManager;
struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T *;

class ModelManager {
public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
    	glm::vec3 tangent;

        bool operator==(const Vertex& other) const {
            return pos == other.pos && texCoord == other.texCoord && normal == other.normal;
        }
    };

    struct ModelResource {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VkBuffer vertexBuffer;
        VmaAllocation vertexAllocation;
        VkBuffer indexBuffer;
        VmaAllocation indexAllocation;
    	glm::mat4 model = glm::mat4(1.0f);
    	std::string pipeLineName;
    	bool isActive = true;
    };

    struct TextureResource {
        VkImage image;
        VmaAllocation allocation;
        VkImageView imageView;
        uint32_t width;
        uint32_t height;
    };

    ModelManager() = default;

    ModelManager(Engine* engine, VkDevice device, VmaAllocator allocator, ResourcePathManager &pathManager);

    ~ModelManager();

    ModelResource* LoadModel(const std::string &name, const std::string& pipelineName);

    bool LoadTexture(const std::string &name);

    const ModelResource* GetModel(const std::string &name) const;

    const TextureResource* GetTexture(const std::string &name) const;

    void UnloadResource(const std::string &name);

    void UnloadAllResources();

    void BindModel(const std::string &name, VkCommandBuffer commandBuffer);

    void BindTexture(const std::string &name, VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t binding, VkDescriptorSet set);

    void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

    int DrawModel(VkCommandBuffer &cmd, const std::string &name);
	int DrawModel(VkCommandBuffer &cmd, const ModelResource &model);

	// Iterator for models
	std::unordered_map<std::string, ModelResource>::iterator begin() { return m_models.begin(); }
	std::unordered_map<std::string, ModelResource>::iterator end() { return m_models.end(); }

	std::unordered_map<std::string, ModelResource>::const_iterator begin() const { return m_models.begin(); }
	std::unordered_map<std::string, ModelResource>::const_iterator end() const { return m_models.end(); }

private:
    VkDevice m_device;
    VmaAllocator m_allocator;
    ResourcePathManager m_pathManager;
    Engine *m_engine;

    std::unordered_map<std::string, ModelResource> m_models;
    std::unordered_map<std::string, TextureResource> m_textures;

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                      VkBuffer &buffer, VmaAllocation &allocation);

    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage &image,
                     VmaAllocation &allocation);

    VkImageView CreateImageView(VkImage image, VkFormat format);

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};

template<> struct std::hash<ModelManager::Vertex> {
    size_t operator()(ModelManager::Vertex const& vertex) const {
        return ((std::hash<glm::vec3>()(vertex.pos) ^
               (std::hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1) ^
               (std::hash<glm::vec3>()(vertex.normal) << 1);
    }
};