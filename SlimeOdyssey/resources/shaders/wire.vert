#version 450
#extension GL_EXT_scalar_block_layout : enable

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// Outputs to fragment shader
layout(location = 0) out vec4 FragColour;


// push constant Uniform buffer for transformation matrices
layout(push_constant) uniform TransformUBO {
	mat4 model;
	mat3 normalMatrix;
} transform;

layout(set = 0, binding = 0) uniform CameraUBO {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec4 viewPos;
} camera;

layout(set = 1, binding = 0, scalar) uniform ConfigBuffer {
    vec4 color;
} config;

void main() {
	FragColour = config.color;
	
	vec3 FragPos = vec3(transform.model * vec4(inPosition, 1.0));
	gl_Position = camera.projection * camera.view * vec4(FragPos, 1.0);
}