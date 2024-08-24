//#include "ShadowSystem.h"
//#include <cmath>
//#include <memory>
//
//// Mock classes for Vulkan dependencies
//struct MockDispatchTable : public vkb::DispatchTable {};
//struct MockVmaAllocator {};
//struct MockVulkanDebugUtils {};
//
//const float EPSILON = 1e-6f;
//
//bool VecAlmostEqual(const glm::vec3& a, const glm::vec3& b) {
//    return glm::length(a - b) < EPSILON;
//}
//
//bool MatAlmostEqual(const glm::mat4& a, const glm::mat4& b) {
//    for (int i = 0; i < 4; ++i) {
//        for (int j = 0; j < 4; ++j) {
//            if (std::abs(a[i][j] - b[i][j]) > EPSILON) {
//                return false;
//            }
//        }
//    }
//    return true;
//}
//
//void TestShadowSystemCreation(ModelManager& modelManager) {
//    MockDispatchTable disp;
//    MockVmaAllocator allocator;
//    MockVulkanDebugUtils debugUtils;
//
//    ShadowSystem shadowSystem;
//    shadowSystem.Initialize(disp, (VMAAllocator)allocator, debugUtils);
//
//    // Check default values
//    if (shadowSystem.GetShadowMapWidth() != 1024 || shadowSystem.GetShadowMapHeight() != 1024) {
//        throw std::runtime_error("Default shadow map resolution is incorrect");
//    }
//
//    if (std::abs(shadowSystem.GetShadowNearPlane() - 1.0f) > EPSILON ||
//        std::abs(shadowSystem.GetShadowFarPlane() - 100.0f) > EPSILON) {
//        throw std::runtime_error("Default shadow near/far planes are incorrect");
//    }
//
//    if (std::abs(shadowSystem.GetDirectionalLightDistance() - 100.0f) > EPSILON) {
//        throw std::runtime_error("Default directional light distance is incorrect");
//    }
//}
//
//void TestShadowMapResolution(ModelManager& modelManager) {
//    MockDispatchTable disp;
//    MockVmaAllocator allocator;
//    MockVulkanDebugUtils debugUtils;
//
//    ShadowSystem shadowSystem;
//    shadowSystem.Initialize(disp, allocator, debugUtils);
//
//    uint32_t newWidth = 2048, newHeight = 2048;
//    shadowSystem.SetShadowMapResolution(disp, allocator, debugUtils, newWidth, newHeight, true);
//
//    if (shadowSystem.GetShadowMapWidth() != newWidth || shadowSystem.GetShadowMapHeight() != newHeight) {
//        throw std::runtime_error("Shadow map resolution setter/getter failed");
//    }
//}
//
//void TestShadowNearFarPlanes(ModelManager& modelManager) {
//    MockDispatchTable disp;
//    MockVmaAllocator allocator;
//    MockVulkanDebugUtils debugUtils;
//
//    ShadowSystem shadowSystem;
//    shadowSystem.Initialize(disp, allocator, debugUtils);
//
//    float newNear = 0.5f, newFar = 200.0f;
//    shadowSystem.SetShadowNearPlane(newNear);
//    shadowSystem.SetShadowFarPlane(newFar);
//
//    if (std::abs(shadowSystem.GetShadowNearPlane() - newNear) > EPSILON ||
//        std::abs(shadowSystem.GetShadowFarPlane() - newFar) > EPSILON) {
//        throw std::runtime_error("Shadow near/far plane setter/getter failed");
//    }
//}
//
//void TestDirectionalLightDistance(ModelManager& modelManager) {
//    MockDispatchTable disp;
//    MockVmaAllocator allocator;
//    MockVulkanDebugUtils debugUtils;
//
//    ShadowSystem shadowSystem;
//    shadowSystem.Initialize(disp, allocator, debugUtils);
//
//    float newDistance = 150.0f;
//    shadowSystem.SetDirectionalLightDistance(newDistance);
//
//    if (std::abs(shadowSystem.GetDirectionalLightDistance() - newDistance) > EPSILON) {
//        throw std::runtime_error("Directional light distance setter/getter failed");
//    }
//}
//
//void TestCalculateFrustumCorners(ModelManager& modelManager) {
//    MockDispatchTable disp;
//    MockVmaAllocator allocator;
//    MockVulkanDebugUtils debugUtils;
//
//    ShadowSystem shadowSystem;
//    shadowSystem.Initialize(disp, allocator, debugUtils);
//
//    float fov = 45.0f;
//    float aspect = 16.0f / 9.0f;
//    float near = 0.1f;
//    float far = 100.0f;
//    glm::vec3 position(0.0f, 0.0f, 0.0f);
//    glm::vec3 forward(0.0f, 0.0f, -1.0f);
//    glm::vec3 up(0.0f, 1.0f, 0.0f);
//    glm::vec3 right(1.0f, 0.0f, 0.0f);
//
//    std::vector<glm::vec3> corners = shadowSystem.CalculateFrustumCorners(fov, aspect, near, far, position, forward, up, right);
//
//    if (corners.size() != 8) {
//        throw std::runtime_error("Incorrect number of frustum corners");
//    }
//
//    // Add more specific checks for corner positions if needed
//}
//
//void TestCalculateFrustumSphere(ModelManager& modelManager) {
//    MockDispatchTable disp;
//    MockVmaAllocator allocator;
//    MockVulkanDebugUtils debugUtils;
//
//    ShadowSystem shadowSystem;
//    shadowSystem.Initialize(disp, allocator, debugUtils);
//
//    std::vector<glm::vec3> frustumCorners = {
//        glm::vec3(-1, -1, -1), glm::vec3(1, -1, -1),
//        glm::vec3(-1, 1, -1), glm::vec3(1, 1, -1),
//        glm::vec3(-1, -1, 1), glm::vec3(1, -1, 1),
//        glm::vec3(-1, 1, 1), glm::vec3(1, 1, 1)
//    };
//
//    glm::vec3 center;
//    float radius;
//    shadowSystem.CalculateFrustumSphere(frustumCorners, center, radius);
//
//    if (!VecAlmostEqual(center, glm::vec3(0.0f)) || std::abs(radius - std::sqrt(3.0f)) > EPSILON) {
//        throw std::runtime_error("Frustum sphere calculation failed");
//    }
//}
//
//void TestLightSpaceMatrixDirectional(ModelManager& modelManager) {
//    MockDispatchTable disp;
//    MockVmaAllocator allocator;
//    MockVulkanDebugUtils debugUtils;
//
//    ShadowSystem shadowSystem;
//    shadowSystem.Initialize(disp, allocator, debugUtils);
//
//    auto light = std::make_shared<DirectionalLight>();
//    light->SetDirection(glm::vec3(0.0