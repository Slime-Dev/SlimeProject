#include "Camera.h"

Camera::Camera(float fov, float aspect, float nearZ, float farZ): m_position(0.0f, 0.0f, 1.0f)
                                                                  , m_front(0.0f, 0.0f, -1.0f)
                                                                  , m_up(0.0f, 1.0f, 0.0f)
                                                                  , m_fov(fov)
                                                                  , m_aspect(aspect)
                                                                  , m_nearZ(nearZ)
                                                                  , m_farZ(farZ)
                                                                  , m_yaw(-90.0f)
                                                                  , m_pitch(0.0f)
{
	updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
	return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix() const
{
	glm::mat4 proj = glm::perspective(glm::radians(m_fov), m_aspect, m_nearZ, m_farZ);
	proj[1][1] *= -1; // Flip Y-axis for Vulkan
	return proj;
}

void Camera::moveForward(float distance)
{
	m_position += m_front * distance;
}

void Camera::moveRight(float distance)
{
	m_position += glm::normalize(glm::cross(m_front, m_up)) * distance;
}

void Camera::moveUp(float distance)
{
	m_position += m_up * distance;
}

void Camera::rotate(float yaw, float pitch)
{
	m_yaw += yaw;
	m_pitch += pitch;

	// Constrain pitch
	if (m_pitch > 89.0f)
		m_pitch = 89.0f;
	if (m_pitch < -89.0f)
		m_pitch = -89.0f;

	updateCameraVectors();
}

void Camera::setPosition(const glm::vec3& position)
{
	m_position = position;
}

glm::vec3 Camera::getPosition() const
{
	return m_position;
}

void Camera::setAspectRatio(float aspect)
{
	m_aspect = aspect;
}

void Camera::updateCameraVectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_front = glm::normalize(front);
}