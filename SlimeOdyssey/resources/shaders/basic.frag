#version 450
#extension GL_EXT_scalar_block_layout : enable

// Input from vertex shader
layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in vec3 Bitangent;
layout(location = 5) in vec4 FragPosLightSpace;

// Output
layout(location = 0) out vec4 FragColor;

// Camera uniforms (set = 0)
layout(set = 0, binding = 0, scalar) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 viewPos;
} camera;

// Light uniforms (set = 1)
layout(set = 1, binding = 0, scalar) uniform LightUBO {
    vec3 direction;
    float ambientStrength;
    vec3 color;
    float padding1;  // Add padding to ensure 16-byte alignment
    mat4 lightSpaceMatrix;
} light;

// Material uniforms (set = 2)
layout(set = 2, binding = 0, scalar) uniform MaterialUBO {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec2 padding;  // Add padding to ensure 16-byte alignment
} material;

// Material textures (set = 2)
layout(set = 2, binding = 1) uniform sampler2D shadowMap;
layout(set = 2, binding = 2) uniform sampler2D albedoMap;
layout(set = 2, binding = 3) uniform sampler2D normalMap;
layout(set = 2, binding = 4) uniform sampler2D metallicMap;
layout(set = 2, binding = 5) uniform sampler2D roughnessMap;
layout(set = 2, binding = 6) uniform sampler2D aoMap;

const float PI = 3.14159265359;

// Function declarations
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
float ShadowCalculation(vec4 fragPosLightSpace);

void main()
{
    // Sample textures
    vec3 albedo = texture(albedoMap, TexCoords).rgb * material.albedo;
    float metallic = texture(metallicMap, TexCoords).r * material.metallic;
    float roughness = texture(roughnessMap, TexCoords).r * material.roughness;
    float ao = texture(aoMap, TexCoords).r * material.ao;

    // Normal mapping
    vec3 normal = normalize(Normal);
    vec3 tangent = normalize(Tangent);
    vec3 bitangent = normalize(Bitangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    vec3 normalMap = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalMap);

    // Debug normals
    // FragColor = vec4(N * 0.5 + 0.5, 1.0);
    // return;

    // Correct view and light vectors
    vec3 V = normalize(camera.viewPos - FragPos);
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(V + L);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    // Calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);

    // Combine lighting
    vec3 Lo = (kD * albedo / PI + specular) * light.color * NdotL * (1.0 - shadow) * ao;
    vec3 ambient = light.ambientStrength * albedo * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    float shadow = 1.0;
    vec4 shadowCoords = fragPosLightSpace / fragPosLightSpace.w;
    float currentDepth = shadowCoords.z;
    float bias = 0.0005;
    float shadowSample = texture(shadowMap, shadowCoords.xy).r;

    shadow = currentDepth - bias > shadowSample ? 1.0 : 0.0;

    // Apply PCF

    float shadowSum = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, shadowCoords.xy + vec2(x, y) * texelSize).r;
            shadowSum += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow = shadowSum / 9.0;

    return shadow;
}