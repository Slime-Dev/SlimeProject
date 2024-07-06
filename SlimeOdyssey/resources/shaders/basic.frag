#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec2 vTexCoord;

layout(location = 0) out vec4 FragColor;

vec3 lightPos = vec3(0.0, 0.0, 2.0);
vec3 viewPos = vec3(0.0, 0.0, 3.0);
vec3 lightColor = vec3(1.0, 1.0, 1.0);
float ambientStrength = 0.1;
float specularStrength = 0.5;
float shininess = 32.0;

void main() {
    // Normalize the input normal
    vec3 normal = normalize(vNormal);

    // Calculate the light direction
    vec3 lightDir = normalize(lightPos - vPosition);

    // Calculate the view direction
    vec3 viewDir = normalize(viewPos - vPosition);

    // Calculate the reflect direction
    vec3 reflectDir = reflect(-lightDir, normal);

    // Ambient light
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse light
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular light (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Attenuation (distance-based falloff)
    float distance = length(lightPos - vPosition);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

    // Combine all lighting components
    vec3 lighting = (ambient + diffuse + specular) * attenuation;

    // Apply a warm color tint
    vec3 warmTint = vec3(1.0, 0.9, 0.8);
    vec3 color = lighting * warmTint;

    // Tone mapping (optional, helps with bright highlights)
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    // Ensure the color doesn't exceed 1.0 for any channel
    color = clamp(color, 0.0, 1.0);

    FragColor = vec4(color, 1.0);
}