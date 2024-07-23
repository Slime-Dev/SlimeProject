#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, scalar) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewPos;
} camera;

layout(set = 0, binding = 1, scalar) uniform LightUBO {
	vec3 position;
	vec3 color;
	vec3 view;
	float ambientStrength;
	float specularStrength;
	float shininess;
} light;

layout(set = 0, binding = 2, scalar) uniform DebugUBO {
    int debugMode; // 0: normal render, 1: show normals, 2: show light direction, 3: show view direction
    bool useNormalMap; // Toggle normal mapping
} debug;


void main() {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}