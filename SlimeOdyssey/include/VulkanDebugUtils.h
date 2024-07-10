#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <string>

class VulkanDebugUtils
{
private:
    vkb::Instance m_instance;
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

public:
    struct Colour
    {
        float r, g, b, a;
    };

    VulkanDebugUtils(const vkb::Instance& instance, const vkb::Device& device)
        : m_instance(instance), m_device(device)
    {
        LoadDebugUtilsFunctions();
    }

    VulkanDebugUtils() = default;

    void LoadDebugUtilsFunctions()
    {
#if defined(VK_EXT_debug_utils)
        fp_vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkCmdBeginDebugUtilsLabelEXT"));
        fp_vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkCmdEndDebugUtilsLabelEXT"));
        fp_vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkCmdInsertDebugUtilsLabelEXT"));
        fp_vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkSetDebugUtilsObjectNameEXT"));
        fp_vkSetDebugUtilsObjectTagEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkSetDebugUtilsObjectTagEXT"));
        fp_vkQueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkQueueBeginDebugUtilsLabelEXT"));
        fp_vkQueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkQueueEndDebugUtilsLabelEXT"));
        fp_vkQueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(
            vkGetInstanceProcAddr(m_instance.instance, "vkQueueInsertDebugUtilsLabelEXT"));
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

    template <typename T>
    void SetObjectName(T vkObject, VkObjectType objectType, const std::string& name)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType                         = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType                    = objectType;
			nameInfo.objectHandle                  = (uint64_t)vkObject;
			nameInfo.pObjectName                   = name.c_str();

			VkResult result = fp_vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
			if (result != VK_SUCCESS)
			{
				// Handle error, perhaps log it
				// For example: spdlog::error("Failed to set debug name for instance. Error: {}", result);
			}
		}
#endif
	}

	// Helper function to get the correct object type for common Vulkan handles
	template <typename T>
    VkObjectType GetObjectType()
	{
		if constexpr (std::is_same_v<T, VkInstance>)
			return VK_OBJECT_TYPE_INSTANCE;
		else if constexpr (std::is_same_v<T, VkPhysicalDevice>)
			return VK_OBJECT_TYPE_PHYSICAL_DEVICE;
		else if constexpr (std::is_same_v<T, VkDevice>)
			return VK_OBJECT_TYPE_DEVICE;
		else if constexpr (std::is_same_v<T, VkQueue>)
			return VK_OBJECT_TYPE_QUEUE;
		else if constexpr (std::is_same_v<T, VkCommandBuffer>)
			return VK_OBJECT_TYPE_COMMAND_BUFFER;
		else if constexpr (std::is_same_v<T, VkBuffer>)
			return VK_OBJECT_TYPE_BUFFER;
		else if constexpr (std::is_same_v<T, VkImage>)
			return VK_OBJECT_TYPE_IMAGE;
        else if constexpr (std::is_same_v<T, VkImageView>)
            return VK_OBJECT_TYPE_IMAGE_VIEW;
		else
			return VK_OBJECT_TYPE_UNKNOWN;
	}

	// Convenience function to set object name with automatic type deduction
	template <typename T>
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
};

constexpr VulkanDebugUtils::Colour debugUtil_White = { 1.0f, 1.0f, 1.0f, 1.0f }; // White
constexpr VulkanDebugUtils::Colour debugUtil_BeginColour = { 1.0f, 0.94f, 0.7f, 1.0f }; // Pastel Yellow
constexpr VulkanDebugUtils::Colour debugUtil_StartDrawColour = { 0.7f, 0.9f, 0.7f, 1.0f }; // Pastel Green
constexpr VulkanDebugUtils::Colour debugUtil_BindDescriptorSetColour = { 0.7f, 0.8f, 1.0f, 1.0f }; // Pastel Blue
constexpr VulkanDebugUtils::Colour debugUtil_UpdateLightBufferColour = { 0.7f, 0.8f, 1.0f, 1.0f }; // Pastel Blue
constexpr VulkanDebugUtils::Colour debugUtil_FrameSubmission = { 1.0f, 0.8f, 0.7f, 1.0f }; // Pastel Orange
constexpr VulkanDebugUtils::Colour debugUtil_DrawModelColour = { 0.9f, 0.9f, 0.9f, 1.0f }; // Very Light Gray