#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera(float fov, float aspect, float nearZ, float farZ);

	[[nodiscard]] glm::mat4 GetViewMatrix() const;
	[[nodiscard]] glm::mat4 GetProjectionMatrix() const;

	void MoveForward(float distance);
	void MoveRight(float distance);
	void MoveUp(float distance);

	void Rotate(float yaw, float pitch);

	void SetPosition(const glm::vec3& position);
	[[nodiscard]] glm::vec3 GetPosition() const;

	void SetTarget(const glm::vec3& target);

	void SetAspectRatio(float aspect);

private:
	void UpdateCameraVectors();

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
