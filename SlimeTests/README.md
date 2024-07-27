# Unit Testing

This document outlines the unit testing strategy for the SlimeOdyssey project.

## Testing Framework

We use CMake's built-in testing capabilities (CTest) for our unit tests. Our tests are written as standalone C++ programs that use assertions and exceptions to verify correct behavior.

## Test Structure

Our tests are organized as follows:

- `SlimeTests/` - Root directory for all tests
  - Each test is a separate .cpp file.

## Running Tests

To run the tests:

1. Ensure you have built the project with the `BUILD_TESTING` option enabled:
   ```
   cmake -B build -S . -DBUILD_TESTING=ON
   cmake --build build
   ```
2. Navigate to the build directory and run the tests using CTest:
   ```
   cd build
   ctest
   ```

## Writing New Tests

When adding new functionality, please follow these guidelines for creating tests:

1. Create a new .cpp file in the `tests/` directory.
2. Name your test file descriptively (e.g., `ModelLoading.cpp`).
3. Include the necessary headers and the component you're testing.
4. Write test functions that use assertions or throw exceptions on failure.
5. In the `main()` function, call your test functions and handle exceptions.
6. Use spdlog for logging test results.

## Adding Tests to CMake

To add your new test to the build system:

1. Open the `CMakeLists.txt` file in the `tests/` directory.
2. Add your test using the `add_executable()` and `add_test()` commands:

```cmake
add_executable(ModelLoading ModelLoading.cpp)
target_link_libraries(ModelLoading PRIVATE project_options project_warnings CONAN_PKG::spdlog)
add_test(NAME ModelLoading COMMAND ModelLoading)
```

## Continuous Integration

We use GitHub Actions to run our test suite on every push and pull request. The workflow is defined in `.github/workflows/multi-platform-test.yml`.

## Updating Tests

As the project evolves, please ensure that:

1. Tests are updated to reflect changes in the codebase.
2. New features are accompanied by appropriate tests.
3. Deprecated features have their associated tests removed or updated.

Remember, good tests are essential for maintaining the stability and reliability of SlimeOdyssey as we continue to develop and improve the engine.