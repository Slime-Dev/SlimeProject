struct Light
{
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    glm::vec3 viewPos;
    float ambientStrength;  // This now aligns correctly with the GPU struct
    float specularStrength;
    float shininess;
};

struct LightObject
{
	Light light;
	VkBuffer buffer;
	VmaAllocation allocation;
};