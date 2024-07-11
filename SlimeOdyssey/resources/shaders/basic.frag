#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vPosition;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in vec3 vTangent;
layout(location = 4) in vec3 vBitangent;
layout(location = 5) in vec3 vViewPosition;

layout(location = 0) out vec4 FragColor;

// Lighting uniform block
layout(scalar, binding = 0) uniform LightBlock {
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
    float ambientStrength;
    float specularStrength;
    float shininess;
} Light;

// Material properties
const vec3 diffuseColor = vec3(1, 1, 1);
float specularIntensity = 0.5;

// Calculate the lighting
vec3 calculateLighting(vec3 normal, vec3 fragPos) {
    // Ambient
    vec3 ambient = Light.ambientStrength * Light.lightColor;

    // Diffuse
    vec3 lightDir = normalize(Light.lightPos - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * Light.lightColor * diffuseColor;

    // Specular
    vec3 viewDir = normalize(Light.viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), Light.shininess);
    vec3 specular = Light.specularStrength * spec * Light.lightColor * specularIntensity;

    return (ambient + diffuse + specular);
}

// Calculate the normal in tangent space
vec3 calculateNormal() {
    vec3 tangent = normalize(vTangent);
    vec3 bitangent = normalize(vBitangent);
    vec3 normal = normalize(vNormal);

    // Gram-Schmidt process to re-orthogonalize the TBN matrix
    tangent = normalize(tangent - dot(tangent, normal) * normal);
    bitangent = cross(normal, tangent);

    mat3 TBN = mat3(tangent, bitangent, normal);
    return normalize(TBN * vec3(0.0, 0.0, 1.0)); // Assuming a flat normal for now
}

void main() {
    // Calculate the normal
    vec3 normal = calculateNormal();

    // Calculate the lighting
    vec3 lighting = calculateLighting(normal, vPosition);

    // Output the color
    FragColor = vec4(lighting, 1.0);
}