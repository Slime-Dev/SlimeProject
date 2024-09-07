#include "Light.h"
#include "ModelManager.h"
#include <cmath>
#include "spdlog/spdlog.h"

const float EPSILON = 1e-6f;

bool VecAlmostEqual(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(a - b) < EPSILON;
}

struct TestResult {
    std::string testName;
    bool passed;
    std::string errorMessage;
};

void TestPointLightCreation(ModelManager& modelManager) {
    PointLight light;
    if (light.GetType() != LightType::Point) {
        throw std::runtime_error("PointLight type is incorrect");
    }
    if (!VecAlmostEqual(light.GetPosition(), glm::vec3(0.0f))) {
        throw std::runtime_error("PointLight default position is incorrect");
    }
    if (std::abs(light.GetRadius() - 1.0f) > EPSILON) {
        throw std::runtime_error("PointLight default radius is incorrect");
    }
}

void TestPointLightPosition(ModelManager& modelManager) {
    PointLight light;
    glm::vec3 newPos(1.0f, 2.0f, 3.0f);
    light.SetPosition(newPos);
    if (!VecAlmostEqual(light.GetPosition(), newPos)) {
        throw std::runtime_error("PointLight position setter/getter failed");
    }
}

void TestPointLightRadius(ModelManager& modelManager) {
    PointLight light;
    float newRadius = 5.0f;
    light.SetRadius(newRadius);
    if (std::abs(light.GetRadius() - newRadius) > EPSILON) {
        throw std::runtime_error("PointLight radius setter/getter failed");
    }
}

void TestPointLightData(ModelManager& modelManager) {
    PointLight light;
    LightData data;
    data.color = glm::vec3(0.5f, 0.6f, 0.7f);
    data.ambientStrength = 0.2f;
    data.specularStrength = 0.8f;
    light.SetData(data);

    const LightData& retrievedData = light.GetData();
    if (!VecAlmostEqual(retrievedData.color, data.color) ||
        std::abs(retrievedData.ambientStrength - data.ambientStrength) > EPSILON ||
        std::abs(retrievedData.specularStrength - data.specularStrength) > EPSILON) {
        throw std::runtime_error("PointLight data setter/getter failed");
    }
}

void TestDirectionalLightCreation(ModelManager& modelManager) {
    DirectionalLight light;
    if (light.GetType() != LightType::Directional) {
        throw std::runtime_error("DirectionalLight type is incorrect");
    }
    if (!VecAlmostEqual(light.GetDirection(), glm::vec3(0.0f, -1.0f, 0.0f))) {
        throw std::runtime_error("DirectionalLight default direction is incorrect");
    }
    if (std::abs(light.GetData().ambientStrength - 0.075f) > EPSILON) {
        throw std::runtime_error("DirectionalLight default ambient strength is incorrect");
    }
}

void TestDirectionalLightDirection(ModelManager& modelManager) {
    DirectionalLight light;
    glm::vec3 newDir(1.0f, 0.0f, 0.0f);
    light.SetDirection(newDir);
    if (!VecAlmostEqual(light.GetDirection(), glm::normalize(newDir))) {
        throw std::runtime_error("DirectionalLight direction setter/getter failed");
    }
}

void TestDirectionalLightData(ModelManager& modelManager) {
    DirectionalLight light;
    LightData data;
    data.color = glm::vec3(0.8f, 0.9f, 1.0f);
    data.ambientStrength = 0.1f;
    light.SetData(data);

    const LightData& retrievedData = light.GetData();
    if (!VecAlmostEqual(retrievedData.color, data.color) ||
        std::abs(retrievedData.ambientStrength - data.ambientStrength) > EPSILON) {
        throw std::runtime_error("DirectionalLight data setter/getter failed");
    }
}

TestResult RunTest(const std::string& testName, std::function<void(ModelManager&)> testFunction) {
    ModelManager modelManager = ModelManager();
    try {
        testFunction(modelManager);
        return { testName, true, "" };
    }
    catch (const std::exception& e) {
        return { testName, false, e.what() };
    }
    catch (...) {
        return { testName, false, "Unknown exception occurred" };
    }
}

int main() {
    std::vector<TestResult> testResults;
    testResults.push_back(RunTest("PointLightCreation", TestPointLightCreation));
    testResults.push_back(RunTest("PointLightPosition", TestPointLightPosition));
    testResults.push_back(RunTest("PointLightRadius", TestPointLightRadius));
    testResults.push_back(RunTest("PointLightData", TestPointLightData));
    testResults.push_back(RunTest("DirectionalLightCreation", TestDirectionalLightCreation));
    testResults.push_back(RunTest("DirectionalLightDirection", TestDirectionalLightDirection));
    testResults.push_back(RunTest("DirectionalLightData", TestDirectionalLightData));

    bool allPassed = true;
    for (const auto& result : testResults) {
        if (result.passed) {
            spdlog::info("Test '{}' passed.", result.testName);
        } else {
            spdlog::error("Test '{}' failed with error: {}", result.testName, result.errorMessage);
            allPassed = false;
        }
    }

    if (allPassed) {
        spdlog::info("All Tests Passed!");
        return 0;
    } else {
        spdlog::error("Some Tests Failed.");
        return 1;
    }
}