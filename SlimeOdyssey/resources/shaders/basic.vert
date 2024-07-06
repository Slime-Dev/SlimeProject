#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vPosition;
layout(location = 2) out vec2 vTexCoord;

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

void main() {
	vNormal = mat3(ubo.model) * aNormal;
	vPosition = vec3(ubo.model * vec4(aPosition, 1.0));
	vTexCoord = aTexCoord;

	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(aPosition, 1.0);
}