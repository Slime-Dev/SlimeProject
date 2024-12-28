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
    vec3 color;
    float padding1;
    float ambientStrength;
    float specularStrength;
    vec2 padding2;
    mat4 lightSpaceMatrix;
    vec3 direction;
    float padding3;
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

    // Create TBN matrix for normal mapping
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    vec3 N = normalize(Normal);
    mat3 TBN = mat3(T, B, N);

    // Sample and transform normal from normal map
    vec3 normalMap = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    vec3 normal = normalize(TBN * normalMap);

    // Use the transformed normal for lighting calculations
    vec3 V = normalize(camera.viewPos - FragPos);
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(V + L);

    // Calculate basic dot products with transformed normal
    float NdotV = max(dot(normal, V), 0.001);
    float NdotL = max(dot(normal, L), 0.001);
    float NdotH = max(dot(normal, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Modify roughness to prevent specular artifacts at very low values
    float roughness_clamped = max(roughness, 0.01);

    // Cook-Torrance BRDF components
    float D = DistributionGGX(normal, H, roughness_clamped);
    float G = GeometrySmith(normal, V, L, roughness_clamped);
    vec3 F = fresnelSchlick(HdotV, F0);

    // Calculate specular
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / max(denominator, 0.001);

    // Debug visualization
    vec3 ambientDebug = albedo * 0.1;
    vec3 specularDebug = (specular * 0.75) * albedo;

    // You can uncomment different debug outputs to see different components:

    // Output 1: Just specular contribution (current debug setup)
    // FragColor = vec4(specularDebug + ambientDebug, 1.0);

    // Output 2: Normal map visualization
    //FragColor = vec4(normal * 0.5 + 0.5, 1.0);

    // Output 3: Individual components
    //FragColor = vec4(vec3(D, G, F.r) + ambientDebug, 1.0);

    //return;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    // Combine components with better energy conservation
    vec3 diffuse = kD * albedo / PI;

    // Final combination with better bounded specular contribution
    vec3 Lo = (diffuse + specular * clamp(light.specularStrength, 0.0, 1.0)) * light.color * NdotL;

    // Calculate shadow (your existing shadow calculation)
    float shadow = ShadowCalculation(FragPosLightSpace);

    // Combine lighting with shadow and ambient
    vec3 ambient = light.ambientStrength * albedo * ao;
    vec3 color = ambient + Lo * (1.0 - shadow);

    // HDR tonemapping (modified for better specular handling)
    color = color / (color + vec3(1.0));

    // Gamma correction
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