#include "RenderPasses/ShadowRenderPass.h"

ShadowRenderPass::ShadowRenderPass(ModelManager& modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue)
      : m_modelManager(modelManager), m_allocator(allocator), m_commandPool(commandPool), m_graphicsQueue(graphicsQueue)
{
	name = "Shadow Pass";
}

void ShadowRenderPass::Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils& debugUtils)
{
	m_shadowSystem.Initialize(disp, allocator, debugUtils);
}

void ShadowRenderPass::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	m_shadowSystem.Cleanup(disp, allocator);
}

void ShadowRenderPass::Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera)
{
	std::vector<std::shared_ptr<Light>> lights;
	scene->m_entityManager.ForEachEntityWith<DirectionalLight>(
	        [&lights](Entity& entity)
	        {
		        if (auto light = entity.GetComponentShrPtr<DirectionalLight>())
		        {
			        lights.push_back(std::move(light));
		        }
	        });

	scene->m_entityManager.ForEachEntityWith<PointLight>(
	        [&lights](Entity& entity)
	        {
		        if (auto light = entity.GetComponentShrPtr<PointLight>())
		        {
			        lights.push_back(std::move(light));
		        }
	        });

	m_shadowSystem.UpdateShadowMaps(
	        disp, cmd, m_modelManager, m_allocator, m_commandPool, m_graphicsQueue, m_debugUtils, scene, [this](vkb::DispatchTable d, VulkanDebugUtils& du, VkCommandBuffer& c, ModelManager& mm, Scene* s) { this->DrawModelsForShadowMap(d, du, c, mm, s); }, lights, camera);
}

VkRenderingInfo ShadowRenderPass::GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) const
{
	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	return renderingInfo;
}

ShadowSystem& ShadowRenderPass::GetShadowSystem()
{
	return m_shadowSystem;
}

void ShadowRenderPass::DrawModelsForShadowMap(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VkCommandBuffer& cmd, ModelManager& modelManager, Scene* scene)
{
	// Implement shadow map rendering logic here
	// This is similar to your existing DrawModelsForShadowMap method
}
