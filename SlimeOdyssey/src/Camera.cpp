#include "Camera.h"

#include "spdlog/spdlog.h"
#include "VulkanUtil.h"

Camera::Camera(float fov, float aspect, float nearZ, float farZ)
      : m_position(0.0f, 0.0f, 1.0f), m_front(0.0f, 0.0f, -1.0f), m_up(0.0f, 1.0f, 0.0f), m_fov(fov), m_aspect(aspect), m_nearZ(nearZ), m_farZ(farZ), m_yaw(-90.0f), m_pitch(0.0f)
{
	UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::GetProjectionMatrix() const
{
	glm::mat4 proj = glm::perspective(glm::radians(m_fov), m_aspect, m_nearZ, m_farZ);
	proj[1][1] *= -1; // Flip Y-axis for Vulkan
	return proj;
}

void Camera::MoveForward(float distance)
{
	m_position += m_front * distance;
}

void Camera::MoveRight(float distance)
{
	m_position += glm::normalize(glm::cross(m_front, m_up)) * distance;
}

void Camera::MoveUp(float distance)
{
	m_position += m_up * distance;
}

void Camera::Rotate(float yaw, float pitch)
{
	m_yaw += yaw;
	m_pitch += pitch;

	// Constrain pitch
	if (m_pitch > 89.0f)
		m_pitch = 89.0f;
	if (m_pitch < -89.0f)
		m_pitch = -89.0f;

	UpdateCameraVectors();
}

void Camera::SetPosition(const glm::vec3& position)
{
	m_position = position;
}

glm::vec3 Camera::GetPosition() const
{
	return m_position;
}

void Camera::SetTarget(const glm::vec3& target)
{
	m_front = glm::normalize(target - m_position);

	m_yaw = glm::degrees(std::atan2(m_front.z, m_front.x));
	m_pitch = glm::degrees(std::asin(m_front.y));

	UpdateCameraVectors();
}

void Camera::SetAspectRatio(float aspect)
{
	m_aspect = aspect;
}

void Camera::CreateCameraUBO(VmaAllocator allocator)
{
	// Create buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(CameraUBO);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &m_cameraUBOBBuffer, &m_cameraUBOAllocation, nullptr) != VK_SUCCESS)
	{
		spdlog::error("Failed to create camera UBO buffer!");
	}
}

void Camera::DestroyCameraUBOBuffer(VmaAllocator allocator)
{
	vmaDestroyBuffer(allocator, m_cameraUBOBBuffer, m_cameraUBOAllocation);
}

void Camera::UpdateCameraUBO(VmaAllocator allocator)
{
	if (m_cameraUBOBBuffer == VK_NULL_HANDLE)
	{
		CreateCameraUBO(allocator);
		return;
	}

	m_cameraUBO.view = GetViewMatrix();
	m_cameraUBO.projection = GetProjectionMatrix();
	m_cameraUBO.viewProjection = m_cameraUBO.projection * m_cameraUBO.view;
	m_cameraUBO.viewPos = glm::vec4(m_position, 1.0f);

	// Copy to buffer
	SlimeUtil::CopyStructToBuffer(m_cameraUBO, allocator, m_cameraUBOAllocation);
}

CameraUBO& Camera::GetCameraUBO()
{
	return m_cameraUBO;
}

void Camera::UpdateCameraVectors()
{
	glm::vec3 front;
	front.x = std::cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = std::sin(glm::radians(m_pitch));
	front.z = std::sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_front = glm::normalize(front);
}
