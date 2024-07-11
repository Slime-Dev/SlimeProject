#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	// Constructor
	Camera(float fov, float aspect, float nearZ, float farZ);

	// Matrix getters
	[[nodiscard]] glm::mat4 GetViewMatrix() const;
	[[nodiscard]] glm::mat4 GetProjectionMatrix() const;

	// Movement methods
	void MoveForward(float distance);
	void MoveRight(float distance);
	void MoveUp(float distance);

	// Rotation method
	void Rotate(float yaw, float pitch);

	// Position methods
	void SetPosition(const glm::vec3& position);
	[[nodiscard]] glm::vec3 GetPosition() const;

	// Target method
	void SetTarget(const glm::vec3& target);

	// Camera properties
	void SetAspectRatio(float aspect);

private:
	// Helper method
	void UpdateCameraVectors();

	// Camera vectors
	glm::vec3 m_position;
	glm::vec3 m_front;
	glm::vec3 m_up;

	// Camera properties
	float m_fov;
	float m_aspect;
	float m_nearZ;
	float m_farZ;
	float m_yaw;
	float m_pitch;
};
