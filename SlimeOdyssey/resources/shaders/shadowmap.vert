#version 450

layout(push_constant) uniform PushConstants {
	mat4 lightSpaceMatrix;
    mat4 modelMatrix;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

void main() {
    gl_Position = pushConstants.lightSpaceMatrix * pushConstants.modelMatrix * vec4(inPosition, 1.0);
}