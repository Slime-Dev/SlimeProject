#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vPosition;
layout(location = 2) out vec3 vColor;

//layout(std140, binding = 0) uniform UniformBufferObject {
//	mat4 modelMatrix;
//	mat4 viewMatrix;
//	mat4 projectionMatrix;
//};

void main() {
	mat4 modelMatrix = mat4(1.0);
	mat4 viewMatrix = mat4(1.0);
	mat4 projectionMatrix = mat4(1.0);
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPosition, 1.0);
	vNormal = aNormal;
	vPosition = aPosition;
	vColor = vec3(1.0, 0.7, 0.5);
}
