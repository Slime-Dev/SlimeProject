#version 450
#extension GL_EXT_scalar_block_layout : enable

// Input from vertex shader
layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in vec3 Bitangent;

// Output
layout(location = 0) out vec4 FragColor;

// Camera and lighting properties
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

// Material
layout(set = 1, binding = 0, scalar) uniform MaterialUBO {
    vec4 albedo;
    float metallic;
    float roughness;
    float ao;
} material;

layout(set = 1, binding = 1) uniform sampler2D albedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D metallicMap;
layout(set = 1, binding = 4) uniform sampler2D roughnessMap;
layout(set = 1, binding = 5) uniform sampler2D aoMap;

layout(set = 0, binding = 2, scalar) uniform DebugUBO {
    int debugMode; // 0: normal render, 1: show normals, 2: show light direction, 3: show view direction
    bool useNormalMap; // Toggle normal mapping
} debug;

vec3 calculateNormal() {
    if (!debug.useNormalMap) {
        return normalize(Normal);
    }
    
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    vec3 Q1 = dFdx(FragPos);
    vec3 Q2 = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);
    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo = texture(albedoMap, TexCoords).rgb * material.albedo.rgb;
    float metallic = texture(metallicMap, TexCoords).r * material.metallic;
    float roughness = texture(roughnessMap, TexCoords).r * material.roughness;
    float ao = texture(aoMap, TexCoords).r * material.ao;
    
    vec3 N = calculateNormal();
    vec3 V = normalize(camera.viewPos.xyz - FragPos);
    vec3 L = normalize(light.position - FragPos);
    vec3 H = normalize(V + L);
    
    // Debug output
    if (debug.debugMode == 1) {
        FragColor = vec4(N * 0.5 + 0.5, 1.0);
        return;
    } else if (debug.debugMode == 2) {
        FragColor = vec4(L * 0.5 + 0.5, 1.0);
        return;
    } else if (debug.debugMode == 3) {
        FragColor = vec4(V * 0.5 + 0.5, 1.0);
        return;
    }
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / 3.14159265359 + specular) * light.color * NdotL;
    
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    
    // Exposure tone mapping
    float exposure = 1.0;
    color = vec3(1.0) - exp(-color * exposure);
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}