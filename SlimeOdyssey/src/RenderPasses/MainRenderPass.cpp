#include "RenderPasses/MainRenderPass.h"

MainRenderPass::MainRenderPass(ShadowRenderPass* shadowPass)
      : m_shadowPass(shadowPass)
{
	name = "Main Pass";
}

void MainRenderPass::Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera)
{
}

void MainRenderPass::BindDescriptorSets(vkb::DispatchTable& disp, VkCommandBuffer& cmd, VkPipelineLayout layout, Scene* scene)
{
}

VkDescriptorSet MainRenderPass::GetShadowMapDescriptorSet(Scene* scene, ShadowSystem& shadowSystem)
{
	return VK_NULL_HANDLE;
}

void MainRenderPass::Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils& debugUtils)
{
}

void MainRenderPass::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{
}

VkRenderingInfo MainRenderPass::GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) const
{
	return VkRenderingInfo();
}
