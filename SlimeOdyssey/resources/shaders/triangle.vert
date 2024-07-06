#version 450
#extension GL_ARB_separate_shader_objects : enable

// MVP push constant
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 fragNormal;

// Define 36 vertices for a cube (6 per face)
vec3 positions[36] = vec3[](
    // Front face
    vec3(-0.5, -0.5,  0.5), vec3(0.5, -0.5,  0.5), vec3(0.5,  0.5,  0.5),
    vec3(-0.5, -0.5,  0.5), vec3(0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5),
    // Back face
    vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5,  0.5, -0.5),
    vec3(-0.5, -0.5, -0.5), vec3(0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5),
    // Top face
    vec3(-0.5,  0.5, -0.5), vec3(0.5,  0.5, -0.5), vec3(0.5,  0.5,  0.5),
    vec3(-0.5,  0.5, -0.5), vec3(0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5),
    // Bottom face
    vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, -0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5,  0.5), vec3(-0.5, -0.5,  0.5),
    // Right face
    vec3(0.5, -0.5, -0.5), vec3(0.5,  0.5, -0.5), vec3(0.5,  0.5,  0.5),
    vec3(0.5, -0.5, -0.5), vec3(0.5,  0.5,  0.5), vec3(0.5, -0.5,  0.5),
    // Left face
    vec3(-0.5, -0.5, -0.5), vec3(-0.5,  0.5, -0.5), vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3(-0.5,  0.5,  0.5), vec3(-0.5, -0.5,  0.5)
);

// Define colors for each vertex
vec3 colors[36] = vec3[](
    // Front face (Red)
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    // Back face (Green)
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    // Top face (Blue)
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
    // Bottom face (Yellow)
    vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    // Right face (Magenta)
    vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0),
    vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0),
    // Left face (Cyan)
    vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0),
    vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0)
);

// Define normals for each face
vec3 normals[36] = vec3[](
    // Front face
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
    // Back face
    vec3(0.0, 0.0, -1.0), vec3(0.0, 0.0, -1.0), vec3(0.0, 0.0, -1.0),
    vec3(0.0, 0.0, -1.0), vec3(0.0, 0.0, -1.0), vec3(0.0, 0.0, -1.0),
    // Top face
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    // Bottom face
    vec3(0.0, -1.0, 0.0), vec3(0.0, -1.0, 0.0), vec3(0.0, -1.0, 0.0),
    vec3(0.0, -1.0, 0.0), vec3(0.0, -1.0, 0.0), vec3(0.0, -1.0, 0.0),
    // Right face
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    // Left face
    vec3(-1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0),
    vec3(-1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0)
);

void main() {
    vec4 worldPos = ubo.model * vec4(positions[gl_VertexIndex], 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragColor = colors[gl_VertexIndex];
    fragPos = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(ubo.model))) * normals[gl_VertexIndex];
}
