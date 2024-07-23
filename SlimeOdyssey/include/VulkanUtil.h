#pragma once

#include <spdlog/spdlog.h>
#include <VkBootstrap.h>

namespace SlimeUtil
{
	inline VkCommandBuffer BeginSingleTimeCommands(const vkb::DispatchTable& disp, VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = commandPool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		if (disp.allocateCommandBuffers(&alloc_info, &command_buffer) != VK_SUCCESS)
		{
			spdlog::error("Failed to allocate command buffer!");
			return VK_NULL_HANDLE;
		}

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (disp.beginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
		{
			spdlog::error("Failed to begin recording command buffer!");
			return VK_NULL_HANDLE;
		}

		return command_buffer;
	}

	inline void EndSingleTimeCommands(const vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer command_buffer)
	{
		if (command_buffer == VK_NULL_HANDLE)
		{
			return;
		}

		if (disp.endCommandBuffer(command_buffer) != VK_SUCCESS)
		{
			spdlog::error("Failed to record command buffer!");
			return;
		}

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		if (disp.queueSubmit(graphicsQueue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			spdlog::error("Failed to submit command buffer!");
			return;
		}

		// Wait for the command buffer to finish execution
		if (disp.queueWaitIdle(graphicsQueue) != VK_SUCCESS)
		{
			spdlog::error("Failed to wait for queue to finish!");
			return;
		}

		// Free the command buffer
		disp.freeCommandBuffers(commandPool, 1, &command_buffer);
	}

	inline void CreateBuffer(const char* name, VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = memoryUsage;

		if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}
		
		vmaSetAllocationName(allocator, allocation, name);
		spdlog::debug("Created Buffer: {}", name);
	}

	inline void CreateImage(const char* name, VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& allocation)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = memoryUsage;

		if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image!");
		}

		vmaSetAllocationName(allocator, allocation, name);
		spdlog::debug("Created Image Buffer: {}", name);
	}

	inline int BeginCommandBuffer(const vkb::DispatchTable& disp, VkCommandBuffer& cmd)
	{
		VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

		if (disp.beginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
		{
			spdlog::error("Failed to begin recording command buffer!");
			return -1;
		}
		return 0;
	}

	inline int EndCommandBuffer(const vkb::DispatchTable& disp, VkCommandBuffer& cmd)
	{
		if (disp.endCommandBuffer(cmd) != VK_SUCCESS)
		{
			spdlog::error("Failed to record command buffer!");
			return -1;
		}
		return 0;
	}

	template<typename T>
	void CopyStructToBuffer(T& data, VmaAllocator allocator, VmaAllocation allocation)
	{
		void* mappedData;
		vmaMapMemory(allocator, allocation, &mappedData);
		memcpy(mappedData, &data, sizeof(T));
		vmaUnmapMemory(allocator, allocation);
	}

	inline void SetupDepthTestingAndLineWidth(const vkb::DispatchTable& disp, VkCommandBuffer& cmd)
	{
		disp.cmdSetDepthTestEnable(cmd, VK_TRUE);
		disp.cmdSetDepthWriteEnable(cmd, VK_TRUE);
		disp.cmdSetDepthCompareOp(cmd, VK_COMPARE_OP_LESS_OR_EQUAL);
		disp.cmdSetLineWidth(cmd, 3.0f);
	}

	inline VkSampler CreateSampler(const vkb::DispatchTable& disp)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VkSampler sampler;
		if (disp.createSampler(&samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture sampler");
		}

		return sampler;
	}

	inline void DestroySampler(const vkb::DispatchTable& disp, VkSampler sampler)
	{
		// Destroy the sampler
		disp.destroySampler(sampler, nullptr);
	}

	inline std::string vkResultToString(VkResult result)
	{
		switch (result)
		{
			case VK_SUCCESS:
				return "VK_SUCCESS";
			case VK_NOT_READY:
				return "VK_NOT_READY";
			case VK_TIMEOUT:
				return "VK_TIMEOUT";
			case VK_EVENT_SET:
				return "VK_EVENT_SET";
			case VK_EVENT_RESET:
				return "VK_EVENT_RESET";
			case VK_INCOMPLETE:
				return "VK_INCOMPLETE";
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				return "VK_ERROR_OUT_OF_HOST_MEMORY";
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
			case VK_ERROR_INITIALIZATION_FAILED:
				return "VK_ERROR_INITIALIZATION_FAILED";
			case VK_ERROR_DEVICE_LOST:
				return "VK_ERROR_DEVICE_LOST";
			case VK_ERROR_MEMORY_MAP_FAILED:
				return "VK_ERROR_MEMORY_MAP_FAILED";
			case VK_ERROR_LAYER_NOT_PRESENT:
				return "VK_ERROR_LAYER_NOT_PRESENT";
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				return "VK_ERROR_EXTENSION_NOT_PRESENT";
			case VK_ERROR_FEATURE_NOT_PRESENT:
				return "VK_ERROR_FEATURE_NOT_PRESENT";
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				return "VK_ERROR_INCOMPATIBLE_DRIVER";
			case VK_ERROR_TOO_MANY_OBJECTS:
				return "VK_ERROR_TOO_MANY_OBJECTS";
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				return "VK_ERROR_FORMAT_NOT_SUPPORTED";
			case VK_ERROR_FRAGMENTED_POOL:
				return "VK_ERROR_FRAGMENTED_POOL";
			case VK_ERROR_OUT_OF_POOL_MEMORY:
				return "VK_ERROR_OUT_OF_POOL_MEMORY";
			case VK_ERROR_UNKNOWN:
				return "VK_ERROR_UNKNOWN";
			// Add more cases as needed for other VkResult values
			default:
				return "UNKNOWN_VK_RESULT";
		}
	}

}; // namespace SlimeUtil

// Macro to log VkResult
#define VK_CHECK(x)                                                                                        \
	do                                                                                                     \
	{                                                                                                      \
		VkResult err = x;                                                                                  \
		if (err)                                                                                           \
		{                                                                                                  \
			spdlog::error("Vulkan error: {} in {} at line {}", SlimeUtil::vkResultToString(err), __FILE__, __LINE__); \
			abort();                                                                                       \
		}                                                                                                  \
	}                                                                                                      \
	while (0)
