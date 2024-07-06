#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 FragColor;

void main() {
    // Simple lighting
    vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));
    float intensity = dot(vNormal, lightDir);
    vec3 color = vColor.rgb * intensity;
    FragColor = vec4(color, 1.0);
}