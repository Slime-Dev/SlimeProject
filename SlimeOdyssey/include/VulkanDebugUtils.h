#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>

class VulkanDebugUtils
{
private:
	vkb::Instance m_instance;
	vkb::Device m_device;

	PFN_vkCmdBeginDebugUtilsLabelEXT fp_vkCmdBeginDebugUtilsLabelEXT   = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT fp_vkCmdEndDebugUtilsLabelEXT       = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT fp_vkCmdInsertDebugUtilsLabelEXT = nullptr;

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
#endif

	// Wrapper functions that can be called regardless of whether VK_EXT_debug_utils is defined
	void BeginDebugMarker(VkCommandBuffer commandBuffer, const char* markerName, VulkanDebugUtils::Colour colour)
	{
#if defined(VK_EXT_debug_utils)
		if (fp_vkCmdBeginDebugUtilsLabelEXT)
		{
			VkDebugUtilsLabelEXT labelInfo = {};
			labelInfo.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName           = markerName;
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
			labelInfo.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName           = markerName;
			memcpy(labelInfo.color, &colour, sizeof(float) * 4);
			cmdInsertDebugUtilsLabelEXT(commandBuffer, &labelInfo);
		}
#endif
	}
};

constexpr VulkanDebugUtils::Colour debugUtil_BeginColour             = { 1.0f, 0.94f, 0.7f, 1.0f }; // Pastel Yellow
constexpr VulkanDebugUtils::Colour debugUtil_StartDrawColour         = { 0.7f, 0.9f, 0.7f, 1.0f };  // Pastel Green
constexpr VulkanDebugUtils::Colour debugUtil_BindDescriptorSetColour = { 0.7f, 0.8f, 1.0f, 1.0f };  // Pastel Blue
constexpr VulkanDebugUtils::Colour debugUtil_UpdateLightBufferColour = { 0.7f, 0.8f, 1.0f, 1.0f };  // Pastel Blue
constexpr VulkanDebugUtils::Colour debugUtil_PushConstantsColour     = { 1.0f, 0.8f, 0.7f, 1.0f };  // Pastel Orange
constexpr VulkanDebugUtils::Colour debugUtil_DrawModelColour         = { 0.9f, 0.9f, 0.9f, 1.0f };  // Very Light Gray