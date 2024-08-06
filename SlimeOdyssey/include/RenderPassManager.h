#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>
#include <vector>
#include <VkBootstrap.h>
#include <VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

#include "Camera.h"
#include "ModelManager.h"
#include "RenderPasses/RenderPassBase.h"
#include "Scene.h"
#include "VulkanDebugUtils.h"

class RenderPassManager
{
public:
	void AddPass(std::shared_ptr<RenderPassBase> pass)
	{
		m_passes.push_back(std::move(pass));
	}

	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils)
	{
		for (auto& pass: m_passes)
		{
			pass->Setup(disp, allocator, swapchain, shaderManager, debugUtils);
		}
	}

	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
	{
		for (auto& pass: m_passes)
		{
			pass->Cleanup(disp, allocator);
		}
	}

	void ExecutePasses(vkb::DispatchTable& disp, VkCommandBuffer& cmd, VulkanDebugUtils& debugUtils, vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView, Scene* scene, Camera* camera)
	{
		for (auto& pass: m_passes)
		{
			debugUtils.BeginDebugMarker(cmd, ("Render Pass: " + pass->name).c_str(), debugUtil_BeginColour);

			auto renderingInfo = pass->GetRenderingInfo(swapchain, swapchainImageView, depthImageView);
			disp.cmdBeginRendering(cmd, &renderingInfo);

			pass->Execute(disp, cmd, scene, camera);

			disp.cmdEndRendering(cmd);

			debugUtils.EndDebugMarker(cmd);
		}
	}

	std::shared_ptr<RenderPassBase> GetPass(std::string name)
	{
		for (auto pass: m_passes)
		{
			if (pass->name == name)
			{
				return pass;
			}
		}

		spdlog::error("Failed to get renderpass {}", name);
		return nullptr;
	}

private:
	std::vector<std::shared_ptr<RenderPassBase>> m_passes;
};
