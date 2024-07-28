#version 450
#extension GL_EXT_scalar_block_layout : enable

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// Outputs to fragment shader
layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoords;
layout(location = 3) out vec3 Tangent;
layout(location = 4) out vec3 Bitangent;
layout(location = 5) out vec4 FragPosLightSpace;

// Push constant for per-object transform
layout(push_constant) uniform TransformUBO {
    mat4 model;
    mat3 normalMatrix;
} transform;

// Camera uniforms (set = 0)
layout(set = 0, binding = 0, scalar) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 viewPos;
    float padding;  // Add padding to ensure 16-byte alignment
} camera;

// Light uniforms (set = 1)
layout(set = 1, binding = 0, scalar) uniform LightUBO {
    vec3 direction;
    float ambientStrength;
    vec3 color;
    float padding1;  // Add padding to ensure 16-byte alignment
    mat4 lightSpaceMatrix;
} light;

const mat4 bias = mat4( // Bias matrix to transform NDC space [-1,1] to texture space [0,1]
  0.5, 0.0, 0.0, 0.0,
  0.0, 0.5, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.5, 0.5, 0.0, 1.0 );

void main() {
    // Calculate vertex position in world space
    FragPos = vec3(transform.model * vec4(inPosition, 1.0));
    
    // Transform normal, tangent, and bitangent to world space
    Normal = normalize(transform.normalMatrix * inNormal);
    Tangent = normalize(transform.normalMatrix * inTangent);
    Bitangent = normalize(transform.normalMatrix * inBitangent);
    
    // Pass texture coordinates to fragment shader
    TexCoords = inTexCoords;
    
    // Calculate fragment position in light space for shadow mapping
    FragPosLightSpace = bias * light.lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // Calculate final vertex position
    gl_Position = camera.viewProjection * vec4(FragPos, 1.0);
}