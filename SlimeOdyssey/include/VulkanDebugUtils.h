#pragma once

#include <string>
#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>

#include "spdlog/spdlog.h"

class VulkanDebugUtils
{
public:
	struct Colour
	{
		float r, g, b, a;
	};

	VulkanDebugUtils(const vkb::InstanceDispatchTable& instDisp, const vkb::Device& device)
	      : m_device(device)
	{
		LoadDebugUtilsFunctions(instDisp);
	}

	VulkanDebugUtils() = default;

	void LoadDebugUtilsFunctions(const vkb::InstanceDispatchTable& instDisp)
	{
#if defined(VK_EXT_debug_utils)
		fp_vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(instDisp.getInstanceProcAddr("vkCmdBeginDebugUtilsLabelEXT"));
		fp_vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(instDisp.getInstanceProcAddr("vkCmdEndDebugUtilsLabelEXT"));
		fp_vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(instDisp.getInstanceProcAddr("vkCmdInsertDebugUtilsLabelEXT"));
		fp_vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(instDisp.getInstanceProcAddr("vkSetDebugUtilsObjectNameEXT"));
		fp_vkSetDebugUtilsObjectTagEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(instDisp.getInstanceProcAddr("vkSetDebugUtilsObjectTagEXT"));
		fp_vkQueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(instDisp.getInstanceProcAddr("vkQueueBeginDebugUtilsLabelEXT"));
		fp_vkQueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(instDisp.getInstanceProcAddr("vkQueueEndDebugUtilsLabelEXT"));
		fp_vkQueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(instDisp.getInstanceProcAddr("vkQueueInsertDebugUtilsLabelEXT"));
#endif
	}

#if defined(VK_EXT_debug_utils)
	void cmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) const noexcept
	{
		if (fp_vkCmdBeginDebugUtilsLabelEXT)
		{
			fp_vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
		}
	}

	void cmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) const noexcept
	{
		if (fp_vkCmdEndDebugUtilsLabelEXT)
		{
			fp_vkCmdEndDebugUtilsLabelEXT(commandBuffer);
		}
	}

	void cmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) const noexcept
	{
		if (fp_vkCmdInsertDebugUtilsLabelEXT)
		{
			fp_vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
		}
	}

	void setDebugUtilsObjectNameEXT(const VkDebugUtilsObjectNameInfoEXT* pNameInfo) const noexcept
	{
		if (fp_vkSetDebugUtilsObjectNameEXT)
		{
			fp_vkSetDebugUtilsObjectNameEXT(m_device.device, pNameInfo);
		}
	}

	void setDebugUtilsObjectTagEXT(const VkDebugUtilsObjectTagInfoEXT* pTagInfo) const noexcept
	{
		if (fp_vkSetDebugUtilsObjectTagEXT)
		{
			fp_vkSetDebugUtilsObjectTagEXT(m_device.device, pTagInfo);
		}
	}

	void queueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo) const noexcept
	{
		if (fp_vkQueueBeginDebugUtilsLabelEXT)
		{
			fp_vkQueueBeginDebugUtilsLabelEXT(queue, pLabelInfo);
		}
	}

	void queueEndDebugUtilsLabelEXT(VkQueue queue) const noexcept
	{
		if (fp_vkQueueEndDebugUtilsLabelEXT)
		{
			fp_vkQueueEndDebugUtilsLabelEXT(queue);
		}
	}

	void queueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo) const noexcept
	{
		if (fp_vkQueueInsertDebugUtilsLabelEXT)
		{
			fp_vkQueueInsertDebugUtilsLabelEXT(queue, pLabelInfo);
		}
	}
#endif

	// Wrapper functions that can be called regardless of whether VK_EXT_debug_utils is defined
	void BeginDebugMarker(VkCommandBuffer commandBuffer, const char* markerName, VulkanDebugUtils::Colour colour)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkCmdBeginDebugUtilsLabelEXT)
		{
			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = markerName;
			memcpy(labelInfo.color, &colour, sizeof(float) * 4);
			cmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
		}
#endif
	}

	void EndDebugMarker(VkCommandBuffer commandBuffer)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkCmdEndDebugUtilsLabelEXT)
		{
			cmdEndDebugUtilsLabelEXT(commandBuffer);
		}
#endif
	}

	void InsertDebugMarker(VkCommandBuffer commandBuffer, const char* markerName, VulkanDebugUtils::Colour colour)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkCmdInsertDebugUtilsLabelEXT)
		{
			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = markerName;
			memcpy(labelInfo.color, &colour, sizeof(float) * 4);
			cmdInsertDebugUtilsLabelEXT(commandBuffer, &labelInfo);
		}
#endif
	}

	template<typename T>
	void SetObjectName(T vkObject, VkObjectType objectType, const std::string& name)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = objectType;
			nameInfo.objectHandle = (uint64_t) vkObject;
			nameInfo.pObjectName = name.c_str();

			VkResult result = fp_vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
			if (result != VK_SUCCESS)
			{
				spdlog::error("Failed to set debug name");
			}
		}
#endif
	}

	// Helper function to get the correct object type for common Vulkan handles
#define VKOBJECT_TO_ENUM(type, enum)       \
	if constexpr (std::is_same_v<T, type>) \
		return enum;

	template<typename T>
	static VkObjectType GetObjectType()
	{
		VKOBJECT_TO_ENUM(VkInstance, VK_OBJECT_TYPE_INSTANCE)
		VKOBJECT_TO_ENUM(VkPhysicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE)
		VKOBJECT_TO_ENUM(VkDevice, VK_OBJECT_TYPE_DEVICE)
		VKOBJECT_TO_ENUM(VkQueue, VK_OBJECT_TYPE_QUEUE)
		VKOBJECT_TO_ENUM(VkSemaphore, VK_OBJECT_TYPE_SEMAPHORE)
		VKOBJECT_TO_ENUM(VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER)
		VKOBJECT_TO_ENUM(VkFence, VK_OBJECT_TYPE_FENCE)
		VKOBJECT_TO_ENUM(VkDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY)
		VKOBJECT_TO_ENUM(VkBuffer, VK_OBJECT_TYPE_BUFFER)
		VKOBJECT_TO_ENUM(VkImage, VK_OBJECT_TYPE_IMAGE)
		VKOBJECT_TO_ENUM(VkEvent, VK_OBJECT_TYPE_EVENT)
		VKOBJECT_TO_ENUM(VkQueryPool, VK_OBJECT_TYPE_QUERY_POOL)
		VKOBJECT_TO_ENUM(VkBufferView, VK_OBJECT_TYPE_BUFFER_VIEW)
		VKOBJECT_TO_ENUM(VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW)
		VKOBJECT_TO_ENUM(VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE)
		VKOBJECT_TO_ENUM(VkPipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE)
		VKOBJECT_TO_ENUM(VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT)
		VKOBJECT_TO_ENUM(VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS)
		VKOBJECT_TO_ENUM(VkPipeline, VK_OBJECT_TYPE_PIPELINE)
		VKOBJECT_TO_ENUM(VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)
		VKOBJECT_TO_ENUM(VkSampler, VK_OBJECT_TYPE_SAMPLER)
		VKOBJECT_TO_ENUM(VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL)
		VKOBJECT_TO_ENUM(VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET)
		VKOBJECT_TO_ENUM(VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER)
		VKOBJECT_TO_ENUM(VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL)
		VKOBJECT_TO_ENUM(VkSamplerYcbcrConversion, VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION)
		VKOBJECT_TO_ENUM(VkDescriptorUpdateTemplate, VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE)
		VKOBJECT_TO_ENUM(VkSurfaceKHR, VK_OBJECT_TYPE_SURFACE_KHR)
		VKOBJECT_TO_ENUM(VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR)
		VKOBJECT_TO_ENUM(VkDisplayKHR, VK_OBJECT_TYPE_DISPLAY_KHR)
		VKOBJECT_TO_ENUM(VkDisplayModeKHR, VK_OBJECT_TYPE_DISPLAY_MODE_KHR)
		VKOBJECT_TO_ENUM(VkDebugReportCallbackEXT, VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT)
		VKOBJECT_TO_ENUM(VkDebugUtilsMessengerEXT, VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT)
		VKOBJECT_TO_ENUM(VkValidationCacheEXT, VK_OBJECT_TYPE_VALIDATION_CACHE_EXT)
		VKOBJECT_TO_ENUM(VkAccelerationStructureKHR, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR)
		VKOBJECT_TO_ENUM(VkAccelerationStructureNV, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV)
		VKOBJECT_TO_ENUM(VkPerformanceConfigurationINTEL, VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL)
		VKOBJECT_TO_ENUM(VkDeferredOperationKHR, VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR)
		VKOBJECT_TO_ENUM(VkIndirectCommandsLayoutNV, VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV)
		VKOBJECT_TO_ENUM(VkPrivateDataSlotEXT, VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT)
		VKOBJECT_TO_ENUM(VkCuModuleNVX, VK_OBJECT_TYPE_CU_MODULE_NVX)
		VKOBJECT_TO_ENUM(VkCuFunctionNVX, VK_OBJECT_TYPE_CU_FUNCTION_NVX)
		VKOBJECT_TO_ENUM(VkOpticalFlowSessionNV, VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV)
		VKOBJECT_TO_ENUM(VkMicromapEXT, VK_OBJECT_TYPE_MICROMAP_EXT)
		VKOBJECT_TO_ENUM(VkShaderEXT, VK_OBJECT_TYPE_SHADER_EXT)

		return VK_OBJECT_TYPE_UNKNOWN;
	}

#undef VKOBJECT_TO_ENUM

	// Convenience function to set object name with automatic type deduction
	template<typename T>
	void SetObjectName(T object, const std::string& name)
	{
		SetObjectName(object, GetObjectType<T>(), name);
	}

	void SetObjectTag(uint64_t object, VkObjectType objectType, uint64_t tagName, size_t tagSize, const void* tagData)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkSetDebugUtilsObjectTagEXT)
		{
			VkDebugUtilsObjectTagInfoEXT tagInfo = {};
			tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
			tagInfo.objectType = objectType;
			tagInfo.objectHandle = object;
			tagInfo.tagName = tagName;
			tagInfo.tagSize = tagSize;
			tagInfo.pTag = tagData;
			setDebugUtilsObjectTagEXT(&tagInfo);
		}
#endif
	}

	void BeginQueueDebugMarker(VkQueue queue, const char* markerName, VulkanDebugUtils::Colour colour)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkQueueBeginDebugUtilsLabelEXT)
		{
			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = markerName;
			memcpy(labelInfo.color, &colour, sizeof(float) * 4);
			queueBeginDebugUtilsLabelEXT(queue, &labelInfo);
		}
#endif
	}

	void EndQueueDebugMarker(VkQueue queue)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkQueueEndDebugUtilsLabelEXT)
		{
			queueEndDebugUtilsLabelEXT(queue);
		}
#endif
	}

	void InsertQueueDebugMarker(VkQueue queue, const char* markerName, VulkanDebugUtils::Colour colour)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkQueueInsertDebugUtilsLabelEXT)
		{
			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = markerName;
			memcpy(labelInfo.color, &colour, sizeof(float) * 4);
			queueInsertDebugUtilsLabelEXT(queue, &labelInfo);
		}
#endif
	}

private:
	vkb::Device m_device;
	PFN_vkCmdBeginDebugUtilsLabelEXT fp_vkCmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT fp_vkCmdEndDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT fp_vkCmdInsertDebugUtilsLabelEXT = nullptr;
	PFN_vkSetDebugUtilsObjectNameEXT fp_vkSetDebugUtilsObjectNameEXT = nullptr;
	PFN_vkSetDebugUtilsObjectTagEXT fp_vkSetDebugUtilsObjectTagEXT = nullptr;
	PFN_vkQueueBeginDebugUtilsLabelEXT fp_vkQueueBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkQueueEndDebugUtilsLabelEXT fp_vkQueueEndDebugUtilsLabelEXT = nullptr;
	PFN_vkQueueInsertDebugUtilsLabelEXT fp_vkQueueInsertDebugUtilsLabelEXT = nullptr;
	PFN_vkSetDebugUtilsObjectNameEXT fp_vkSetDebugUtilsObjectNameEXT_instance = nullptr;
};

constexpr VulkanDebugUtils::Colour debugUtil_White = { 1.0f, 1.0f, 1.0f, 1.0f };                   // White
constexpr VulkanDebugUtils::Colour debugUtil_BeginColour = { 1.0f, 0.94f, 0.7f, 1.0f };            // Pastel Yellow
constexpr VulkanDebugUtils::Colour debugUtil_StartDrawColour = { 0.7f, 0.9f, 0.7f, 1.0f };         // Pastel Green
constexpr VulkanDebugUtils::Colour debugUtil_BindDescriptorSetColour = { 0.7f, 0.8f, 1.0f, 1.0f }; // Pastel Blue
constexpr VulkanDebugUtils::Colour debugUtil_UpdateLightBufferColour = { 0.7f, 0.8f, 1.0f, 1.0f }; // Pastel Blue
constexpr VulkanDebugUtils::Colour debugUtil_FrameSubmission = { 1.0f, 0.8f, 0.7f, 1.0f };         // Pastel Orange
constexpr VulkanDebugUtils::Colour debugUtil_DrawModelColour = { 0.9f, 0.9f, 0.9f, 1.0f };         // Very Light Gray
