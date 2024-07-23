#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec4 FragColour;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, scalar) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewPos;
} camera;

void main() {
    outColor = FragColour;
}