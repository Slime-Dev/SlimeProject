// Vertex Shader
#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vPosition;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out vec3 vTangent;
layout(location = 4) out vec3 vBitangent;
layout(location = 5) out vec3 vViewPosition;

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

void main() {
	vNormal = normalize(mat3(ubo.model) * aNormal);
	vPosition = vec3(ubo.model * vec4(aPosition, 1.0));
	vTexCoord = aTexCoord;
	vTangent = normalize(mat3(ubo.model) * aTangent);
	vBitangent = normalize(cross(vNormal, vTangent));

	vec4 viewPosition = ubo.view * ubo.model * vec4(aPosition, 1.0);
	vViewPosition = -viewPosition.xyz;

	gl_Position = ubo.proj * viewPosition;
}