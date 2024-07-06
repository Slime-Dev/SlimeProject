#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vPosition;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out vec4 vColor;

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

void main() {
	vNormal = mat3(ubo.model) * aNormal;
	vPosition = vec3(ubo.model * vec4(aPosition, 1.0));
	vTexCoord = aTexCoord;

	float R = 0.5 + 0.5 * sin(aPosition.x);
	float G = 0.5 + 0.5 * sin(aPosition.y);
	float B = 0.5 + 0.5 * sin(aPosition.z);

	vColor = vec4(R, G, B, 1.0);
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(aPosition, 1.0);
}