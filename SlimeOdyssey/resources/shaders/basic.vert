#version 450
#extension GL_EXT_scalar_block_layout : enable

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// Outputs to fragment shader
layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoords;
layout(location = 3) out vec3 Tangent;
layout(location = 4) out vec3 Bitangent;

// push constant Uniform buffer for transformation matrices
layout(push_constant) uniform TransformUBO {
	mat4 model;
	mat4 view;
	mat4 projection;
	mat3 normalMatrix;
} transform;

void main() {
	// Calculate vertex position in world space
	FragPos = vec3(transform.model * vec4(inPosition, 1.0));

	// Transform normal, tangent, and bitangent to world space
	Normal = transform.normalMatrix * inNormal;
	Tangent = transform.normalMatrix * inTangent;
	Bitangent = transform.normalMatrix * inBitangent;

	// Pass texture coordinates to fragment shader
	TexCoords = inTexCoords;

	// Calculate final vertex position
	gl_Position = transform.projection * transform.view * vec4(FragPos, 1.0);
}