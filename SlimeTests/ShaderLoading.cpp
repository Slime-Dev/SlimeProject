#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include "ShaderManager.h"
#include "ResourcePathManager.h"
#include <spdlog/spdlog.h>

 struct TestResult {
     std::string testName;
     bool passed;
     std::string errorMessage;
 };

 TestResult RunTest(const std::string& testName, std::function<void(ShaderManager& shaderManager)> testFunction) {
     ShaderManager shaderManager = ShaderManager();
     try {
         testFunction(shaderManager);
         return { testName, true, "" };
     } catch (const std::exception& e) {
         return { testName, false, e.what() };
     } catch (...) {
         return { testName, false, "Unknown exception occurred" };
     }
 }

 void BasicVert(ShaderManager& shaderManager)
 {
     ResourcePathManager resourcePathManager = ResourcePathManager();
     auto basicFrag = shaderManager.LoadShader(vkb::DispatchTable(), resourcePathManager.GetShaderPath("basic.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT);
     if (basicFrag.handle != nullptr) {
         throw std::runtime_error("Failed to load shader");
     }
 }

 int main() {
     std::vector<TestResult> testResults;
     testResults.push_back(RunTest("Load Basic Vert", BasicVert));

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
