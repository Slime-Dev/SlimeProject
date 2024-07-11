#include <iostream>
#include "Engine.h"
#include "spdlog/spdlog.h"
#include "ShaderManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "ModelManager.h"
#include <vector>

void SecondTest()
{
    int test = 2 * 2;
    int result = 4;
    assert(test == result);
}

int main()
{
        try {
        SecondTest();
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred during testing." << std::endl;
        return 2;
    }
}