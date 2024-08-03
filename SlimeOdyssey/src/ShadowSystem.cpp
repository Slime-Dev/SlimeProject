#include "ShadowSystem.h"

#include <backends/imgui_impl_vulkan.h>
#include <Scene.h>

#include "ModelManager.h"
#include "ShadowSystem.h"
#include "VulkanDebugUtils.h"
#include "VulkanUtil.h"

void ShadowSystem::Initialize(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils)
{
	// Initialization code (if needed)
}

void ShadowSystem::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	for (auto& [light, shadowData]: m_shadowData)
	{
		CleanupShadowMap(disp, allocator, light);
	}
	m_shadowData.clear();
}

void ShadowSystem::UpdateShadowMaps(vkb::DispatchTable& disp,
        VkCommandBuffer& cmd,
        ModelManager& modelManager,
        VmaAllocator allocator,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VulkanDebugUtils& debugUtils,
        Scene* scene,
        std::function<void(vkb::DispatchTable, VulkanDebugUtils&, VkCommandBuffer&, ModelManager&, Scene*)> drawModels,
        const std::vector<std::shared_ptr<Light>>& lights,
        std::shared_ptr<Camera> camera)
{
	for (const auto light: lights)
	{
		if (m_shadowData.find(light) == m_shadowData.end())
		{
			CreateShadowMap(disp, allocator, debugUtils, light);
		}
		CalculateLightSpaceMatrix(light, camera);
		GenerateShadowMap(disp, cmd, modelManager, allocator, commandPool, graphicsQueue, debugUtils, scene, drawModels, light, camera);
	}
}

TextureResource ShadowSystem::GetShadowMap(const std::shared_ptr<Light> light) const
{
	auto it = m_shadowData.find(light);
	if (it != m_shadowData.end())
	{
		return it->second.shadowMap;
	}
	return TextureResource(); // Return an empty TextureResource if not found
}

glm::mat4 ShadowSystem::GetLightSpaceMatrix(const std::shared_ptr<Light> light) const
{
	auto it = m_shadowData.find(light);
	if (it != m_shadowData.end())
	{
		return it->second.lightSpaceMatrix;
	}
	return glm::mat4(1.0f); // Return identity matrix if not found
}

void ShadowSystem::SetShadowMapResolution(uint32_t width, uint32_t height)
{
	m_shadowMapWidth = width;
	m_shadowMapHeight = height;
}

void ShadowSystem::SetShadowNearPlane(float near)
{
	m_shadowNear = near;
}

void ShadowSystem::SetShadowFarPlane(float far)
{
	m_shadowFar = far;
}

float ShadowSystem::GetShadowMapPixelValue(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, const std::shared_ptr<Light> light, int x, int y) const
{
	auto it = m_shadowData.find(light);
	if (it == m_shadowData.end())
	{
		return 1.0f; // Return max depth if shadow map not found
	}

	const auto& shadowData = it->second;

	// Ensure x and y are within bounds
	x = std::clamp(x, 0, static_cast<int>(m_shadowMapWidth) - 1);
	y = std::clamp(y, 0, static_cast<int>(m_shadowMapHeight) - 1);

	// Create command buffer for copy operation
	VkCommandBuffer commandBuffer = SlimeUtil::BeginSingleTimeCommands(disp, commandPool);

	// Transition shadow map image layout for transfer
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = shadowData.shadowMap.image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	disp.cmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	// Copy image to buffer
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { x, y, 0 };
	region.imageExtent = { 1, 1, 1 };

	disp.cmdCopyImageToBuffer(commandBuffer, shadowData.shadowMap.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, shadowData.stagingBuffer, 1, &region);

	// Transition shadow map image layout back
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	disp.cmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	SlimeUtil::EndSingleTimeCommands(disp, graphicsQueue, commandPool, commandBuffer);

	// Read data from staging buffer
	float pixelValue;
	void* data;
	vmaMapMemory(allocator, shadowData.stagingBufferAllocation, &data);
	memcpy(&pixelValue, data, sizeof(float));
	vmaUnmapMemory(allocator, shadowData.stagingBufferAllocation);

	return pixelValue;
}

void ShadowSystem::RenderShadowMapInspector(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, VulkanDebugUtils& debugUtils)
{
	ImGui::Begin("Shadow Map Inspector");

	// Window size and aspect ratio
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	float shadowmapAspectRatio = static_cast<float>(m_shadowMapWidth) / m_shadowMapHeight;

	// Zoom control
	static float zoom = 1.0f;
	ImGui::SliderFloat("Zoom", &zoom, 0.1f, 10.0f);

	// Shadow map size controls
	//ImGui::Text("Shadow Map Size:");
	//ImGui::SameLine();
	//ImGui::PushItemWidth(100);
	//ImGui::DragScalar("Width", ImGuiDataType_U32, &m_newShadowMapWidth, 1.0f, nullptr, nullptr, "%u");
	//ImGui::SameLine();
	//ImGui::DragScalar("Height", ImGuiDataType_U32, &m_newShadowMapHeight, 1.0f, nullptr, nullptr, "%u");
	//ImGui::PopItemWidth();

	// Shadow near and far plane controls
	ImGui::Text("Shadow Planes:");
	ImGui::SameLine();
	ImGui::PushItemWidth(100);
	ImGui::DragFloat("Near", &m_shadowNear, 0.1f, 0.1f, m_shadowFar - 0.1f, "%.2f");
	ImGui::SameLine();
	ImGui::DragFloat("Far", &m_shadowFar, 0.1f, m_shadowNear + 0.1f, 1000.0f, "%.2f");
	ImGui::PopItemWidth();

	// Contrast control
	static float contrast = 1.0f;
	ImGui::SliderFloat("Contrast", &contrast, 0.1f, 5.0f);

	// Display a depth scale
	ImGui::Text("Depth Scale:");
	ImVec2 scaleSize(200, 20);
	ImVec2 scalePos = ImGui::GetCursorScreenPos();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	for (int i = 0; i < 100; i++)
	{
		float t = i / 99.0f;
		float enhancedT = std::pow((t - 0.5f) * contrast + 0.5f, 2.2f);
		enhancedT = std::clamp(enhancedT, 0.0f, 1.0f);
		ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(enhancedT, enhancedT, enhancedT, 1.0f));
		drawList->AddRectFilled(ImVec2(scalePos.x + t * scaleSize.x, scalePos.y), ImVec2(scalePos.x + (t + 0.01f) * scaleSize.x, scalePos.y + scaleSize.y), color);
	}
	drawList->AddRect(scalePos, ImVec2(scalePos.x + scaleSize.x, scalePos.y + scaleSize.y), IM_COL32_WHITE);

	ImGui::Dummy(scaleSize);
	ImGui::Text("0.0");
	ImGui::SameLine(185);
	ImGui::Text("1.0");

	// Calculate image size
	ImVec2 imageSize;
	if (windowSize.x / windowSize.y > shadowmapAspectRatio)
	{
		imageSize = ImVec2(windowSize.y * shadowmapAspectRatio, windowSize.y);
	}
	else
	{
		imageSize = ImVec2(windowSize.x, windowSize.x / shadowmapAspectRatio);
	}
	imageSize.x *= zoom;
	imageSize.y *= zoom;

	// TODO: add like a drop down list or something not just the first shadow map
	auto light = m_shadowData.begin()->first;
	auto& lightData = m_shadowData.begin()->second;

	// Display shadow map
	ImGui::BeginChild("ShadowMapRegion", windowSize, true, ImGuiWindowFlags_HorizontalScrollbar);
	if (lightData.textureId == 0)
	{
		lightData.textureId = ImGui_ImplVulkan_AddTexture(lightData.shadowMap.sampler, lightData.shadowMap.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	ImGui::Image(lightData.textureId, imageSize);

	// Pixel info on hover
	if (ImGui::IsItemHovered())
	{
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 imagePos = ImGui::GetItemRectMin();
		ImVec2 relativePos = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

		int pixelX = static_cast<int>((relativePos.x / imageSize.x) * m_shadowMapWidth);
		int pixelY = static_cast<int>((relativePos.y / imageSize.y) * m_shadowMapHeight);

		float pixelValue = GetShadowMapPixelValue(disp, allocator, commandPool, graphicsQueue, light, pixelX, pixelY);

		// Apply contrast enhancement
		float enhancedValue = std::pow((pixelValue - 0.5f) * contrast + 0.5f, 2.2f);
		enhancedValue = std::clamp(enhancedValue, 0.0f, 1.0f);

		ImGui::BeginTooltip();
		ImGui::Text("Pixel: (%d, %d)", pixelX, pixelY);
		ImGui::Text("Depth: %.3f", pixelValue);
		ImGui::Text("Enhanced: %.3f", enhancedValue);

		// Display a color swatch representing the depth
		ImVec4 depthColor(enhancedValue, enhancedValue, enhancedValue, 1.0f);
		ImGui::ColorButton("Depth Color", depthColor, 0, ImVec2(40, 20));

		ImGui::EndTooltip();
	}

	ImGui::EndChild();
	ImGui::End();
}

void ShadowSystem::CreateShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils, const std::shared_ptr<Light> light)
{
	ShadowData& shadowData = m_shadowData[light];

	// Create Shadow map image
	VkImageCreateInfo shadowMapImageInfo = {};
	shadowMapImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	shadowMapImageInfo.imageType = VK_IMAGE_TYPE_2D;
	shadowMapImageInfo.extent.width = m_shadowMapWidth;
	shadowMapImageInfo.extent.height = m_shadowMapHeight;
	shadowMapImageInfo.extent.depth = 1;
	shadowMapImageInfo.mipLevels = 1;
	shadowMapImageInfo.arrayLayers = 1;
	shadowMapImageInfo.format = VK_FORMAT_D32_SFLOAT;
	shadowMapImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	shadowMapImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	shadowMapImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	shadowMapImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	shadowMapImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo shadowMapAllocInfo = {};
	shadowMapAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(allocator, &shadowMapImageInfo, &shadowMapAllocInfo, &shadowData.shadowMap.image, &shadowData.shadowMap.allocation, nullptr));
	debugUtils.SetObjectName(shadowData.shadowMap.image, "ShadowMapImage");

	// Create the shadow map image view
	VkImageViewCreateInfo shadowMapImageViewInfo = {};
	shadowMapImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	shadowMapImageViewInfo.image = shadowData.shadowMap.image;
	shadowMapImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	shadowMapImageViewInfo.format = VK_FORMAT_D32_SFLOAT;
	shadowMapImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	shadowMapImageViewInfo.subresourceRange.baseMipLevel = 0;
	shadowMapImageViewInfo.subresourceRange.levelCount = 1;
	shadowMapImageViewInfo.subresourceRange.baseArrayLayer = 0;
	shadowMapImageViewInfo.subresourceRange.layerCount = 1;

	VK_CHECK(disp.createImageView(&shadowMapImageViewInfo, nullptr, &shadowData.shadowMap.imageView));
	debugUtils.SetObjectName(shadowData.shadowMap.imageView, "ShadowMapImageView");

	// Create the shadow map sampler
	shadowData.shadowMap.sampler = SlimeUtil::CreateSampler(disp);
	debugUtils.SetObjectName(shadowData.shadowMap.sampler, "ShadowMapSampler");

	shadowData.stagingBufferSize = sizeof(float);
	SlimeUtil::CreateBuffer("ShadowMapPixelStagingBuffer", allocator, shadowData.stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, shadowData.stagingBuffer, shadowData.stagingBufferAllocation);
}

void ShadowSystem::CleanupShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, const std::shared_ptr<Light> light)
{
	auto it = m_shadowData.find(light);
	if (it != m_shadowData.end())
	{
		ShadowData& shadowData = it->second;
		vmaDestroyImage(allocator, shadowData.shadowMap.image, shadowData.shadowMap.allocation);
		disp.destroyImageView(shadowData.shadowMap.imageView, nullptr);
		disp.destroySampler(shadowData.shadowMap.sampler, nullptr);
		vmaDestroyBuffer(allocator, shadowData.stagingBuffer, shadowData.stagingBufferAllocation);
	}
}

void ShadowSystem::GenerateShadowMap(vkb::DispatchTable& disp,
        VkCommandBuffer& cmd,
        ModelManager& modelManager,
        VmaAllocator allocator,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VulkanDebugUtils& debugUtils,
        Scene* scene,
        std::function<void(vkb::DispatchTable, VulkanDebugUtils&, VkCommandBuffer&, ModelManager&, Scene*)> drawModels,
        const std::shared_ptr<Light> light,
        const std::shared_ptr<Camera> camera)
{
	debugUtils.BeginDebugMarker(cmd, "Draw Models for Shadow Map", debugUtil_BeginColour);

	auto shadowMap = GetShadowMap(light);

	// Transition shadow map image to depth attachment optimal
	modelManager.TransitionImageLayout(disp, graphicsQueue, commandPool, shadowMap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfo depthAttachmentInfo = {};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = shadowMap.imageView;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue.depthStencil.depth = 1.0f;

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = {
		.offset = {               0,                 0},
          .extent = {m_shadowMapWidth, m_shadowMapHeight}
	};
	renderingInfo.layerCount = 1;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	disp.cmdBeginRendering(cmd, &renderingInfo);

	// Set viewport and scissor for shadow map
	VkViewport viewport = { 0, 0, (float) m_shadowMapWidth, (float) m_shadowMapHeight, 0.0f, 1.0f };
	VkRect2D scissor = {
		{		       0,		         0},
        {m_shadowMapWidth, m_shadowMapHeight}
	};
	disp.cmdSetViewport(cmd, 0, 1, &viewport);
	disp.cmdSetScissor(cmd, 0, 1, &scissor);

	// Draw models for shadow map
	drawModels(disp, debugUtils, cmd, modelManager, scene);

	disp.cmdEndRendering(cmd);

	// Transition shadow map image to shader read-only optimal
	modelManager.TransitionImageLayout(disp, graphicsQueue, commandPool, shadowMap.image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	debugUtils.EndDebugMarker(cmd);
}

void ShadowSystem::CalculateLightSpaceMatrix(const std::shared_ptr<Light> light, const std::shared_ptr<Camera> camera)
{
	auto it = m_shadowData.find(light);
	if (it == m_shadowData.end())
		return;

	ShadowData& shadowData = it->second;

	float fov = camera->GetFOV();
	float aspect = camera->GetAspectRatio();
	glm::vec3 lightDir;
	glm::vec3 lightPos;

	if (light->GetType() == LightType::Directional)
	{
		const DirectionalLight* dirLight = static_cast<const DirectionalLight*>(light.get());
		lightDir = glm::normalize(-dirLight->GetDirection());
		// For directional light, we'll use the camera's position as a reference
		lightPos = camera->GetPosition() - lightDir * (m_shadowFar - m_shadowNear) * 0.5f;
	}
	else if (light->GetType() == LightType::Point)
	{
		const PointLight* pointLight = static_cast<const PointLight*>(light.get());
		lightPos = pointLight->GetPosition();
		// For point light, we'll use a direction towards the camera
		lightDir = glm::normalize(camera->GetPosition() - lightPos);
	}
	else
	{
		spdlog::error("Unsupported light type");
		return;
	}

	// Calculate the corners of the view frustum in world space
	std::vector<glm::vec3> frustumCorners = CalculateFrustumCorners(fov, aspect, m_shadowNear, m_shadowFar, camera->GetPosition(), camera->GetForward(), camera->GetUp(), camera->GetRight());

	// Calculate the bounding sphere of the frustum
	glm::vec3 frustumCenter;
	float frustumRadius;
	CalculateFrustumSphere(frustumCorners, frustumCenter, frustumRadius);

	// Create the light's view matrix
	glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

	// Calculate the bounding box of the frustum in light space
	glm::vec3 minBounds(std::numeric_limits<float>::max());
	glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

	for (const auto& corner: frustumCorners)
	{
		glm::vec3 lightSpaceCorner = glm::vec3(lightView * glm::vec4(corner, 1.0f));
		minBounds = glm::min(minBounds, lightSpaceCorner);
		maxBounds = glm::max(maxBounds, lightSpaceCorner);
	}

	// Add some padding to the bounding box
	float padding = frustumRadius * 0.1f; // 10% of the frustum radius as padding
	minBounds -= glm::vec3(padding);
	maxBounds += glm::vec3(padding);

	// Adjust the Z range for Vulkan's depth range [0, 1]
	float zNear = -maxBounds.z;
	float zFar = -minBounds.z;

	// Create the light's orthographic projection matrix
	glm::mat4 lightProjection = glm::ortho(minBounds.x, maxBounds.x, minBounds.y, maxBounds.y, zNear, zFar);

	glm::mat4 vulkanNdcAdjustment = glm::mat4(1.0f);
	vulkanNdcAdjustment[1][1] = -1.0f;
	vulkanNdcAdjustment[2][2] = 0.5f;
	vulkanNdcAdjustment[3][2] = 0.5f;

	// Combine view and projection matrices
	shadowData.lightSpaceMatrix = vulkanNdcAdjustment * lightProjection * lightView;
	light->SetLightSpaceMatrix(shadowData.lightSpaceMatrix);
}

std::vector<glm::vec3> ShadowSystem::CalculateFrustumCorners(float fov, float aspect, float near, float far, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, const glm::vec3& right) const
{
	float tanHalfFov = tan(glm::radians(fov * 0.5f));
	glm::vec3 nearCenter = position + forward * near;
	glm::vec3 farCenter = position + forward * far;

	float nearHeight = 2.0f * tanHalfFov * near;
	float nearWidth = nearHeight * aspect;
	float farHeight = 2.0f * tanHalfFov * far;
	float farWidth = farHeight * aspect;

	std::vector<glm::vec3> corners(8);
	corners[0] = nearCenter + up * (nearHeight * 0.5f) - right * (nearWidth * 0.5f);
	corners[1] = nearCenter + up * (nearHeight * 0.5f) + right * (nearWidth * 0.5f);
	corners[2] = nearCenter - up * (nearHeight * 0.5f) - right * (nearWidth * 0.5f);
	corners[3] = nearCenter - up * (nearHeight * 0.5f) + right * (nearWidth * 0.5f);
	corners[4] = farCenter + up * (farHeight * 0.5f) - right * (farWidth * 0.5f);
	corners[5] = farCenter + up * (farHeight * 0.5f) + right * (farWidth * 0.5f);
	corners[6] = farCenter - up * (farHeight * 0.5f) - right * (farWidth * 0.5f);
	corners[7] = farCenter - up * (farHeight * 0.5f) + right * (farWidth * 0.5f);

	return corners;
}

void ShadowSystem::CalculateFrustumSphere(const std::vector<glm::vec3>& frustumCorners, glm::vec3& center, float& radius) const
{
	center = glm::vec3(0.0f);
	for (const auto& corner: frustumCorners)
	{
		center += corner;
	}
	center /= frustumCorners.size();

	radius = 0.0f;
	for (const auto& corner: frustumCorners)
	{
		float distance = glm::length(corner - center);
		radius = glm::max(radius, distance);
	}
}
