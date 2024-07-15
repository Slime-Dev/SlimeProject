#pragma once

#include <spdlog/spdlog.h>
#include <VkBootstrap.h>

namespace SlimeUtil
{
	inline VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = commandPool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		if (vkAllocateCommandBuffers(device, &alloc_info, &command_buffer) != VK_SUCCESS)
		{
			spdlog::error("Failed to allocate command buffer!");
			return VK_NULL_HANDLE;
		}

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
		{
			spdlog::error("Failed to begin recording command buffer!");
			return VK_NULL_HANDLE;
		}

		return command_buffer;
	}

	inline void EndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer command_buffer)
	{
		if (command_buffer == VK_NULL_HANDLE)
		{
			return;
		}

		if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
		{
			spdlog::error("Failed to record command buffer!");
			return;
		}

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		if (vkQueueSubmit(graphicsQueue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			spdlog::error("Failed to submit command buffer!");
			return;
		}

		// Wait for the command buffer to finish execution
		if (vkQueueWaitIdle(graphicsQueue) != VK_SUCCESS)
		{
			spdlog::error("Failed to wait for queue to finish!");
			return;
		}

		// Free the command buffer
		vkFreeCommandBuffers(device, commandPool, 1, &command_buffer);
	}

	inline void CreateBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation)
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
		disp.cmdSetLineWidth(cmd, 10.0f);
	}
}; // namespace SlimeUtil
