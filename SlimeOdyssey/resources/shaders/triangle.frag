#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 fragNormal;

layout (location = 0) out vec4 outColor;

void main()
{
    // Define a light direction (you can make this a uniform variable to control it from CPU side)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));

    // Calculate diffuse lighting
    float diff = max(dot(fragNormal, lightDir), 0.0);

    // Mix the vertex color with the diffuse lighting
    vec3 result = fragColor * (0.3 + 0.7 * diff); // 30% ambient, 70% diffuse

    outColor = vec4(result, 1.0);
}