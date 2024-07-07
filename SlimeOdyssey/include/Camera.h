#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera(float fov, float aspect, float nearZ, float farZ);

	glm::mat4 getViewMatrix() const;

	glm::mat4 getProjectionMatrix() const;

	void moveForward(float distance);

	void moveRight(float distance);

	void moveUp(float distance);

	void rotate(float yaw, float pitch);

	void setPosition(const glm::vec3& position);

	glm::vec3 getPosition() const;

	void setAspectRatio(float aspect);

private:
	void updateCameraVectors();

	glm::vec3 m_position;
	glm::vec3 m_front;
	glm::vec3 m_up;
	float m_fov;
	float m_aspect;
	float m_nearZ;
	float m_farZ;
	float m_yaw;
	float m_pitch;
};