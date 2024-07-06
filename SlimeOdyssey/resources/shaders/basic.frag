#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec3 vColor;

layout (location = 0) out vec4 FragColor;

void main() {
    // Simple ambient lighting calculation based on the normal vector
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0); // White light

    // Calculate diffuse lighting based on the normal vector
    vec3 lightDirection = normalize(vec3(-1.0, -1.0, -1.0)); // Light direction (for simplicity, pointing towards negative axis)
    float diff = max(dot(vNormal, lightDirection), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0); // Diffuse reflection of white light

    // Combine ambient and diffuse components
    vec3 result = (ambient + diffuse) * vColor;

    // Output the final color
    FragColor = vec4(result, 1.0);
}
