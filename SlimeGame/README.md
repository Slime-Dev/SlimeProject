# SlimeOdyssey

Slime Odyssey is a graphics engine I am developing in C++ using vulkan. The goal of this project is to create a simple and easy to use graphics engine that can be used on all platforms. This is mainly for me to practice modern c++ and vulkan, but I hope that it can be useful to others as well.

This project uses cmake to build as well as CPM.cmake to manage dependencies.

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/65fcce3de3e74dc3bc526bd1fc56ee3d)](https://app.codacy.com/gh/AlexMollard/SlimeOdyssey/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
![example workflow](https://github.com/AlexMollard/SlimeOdyssey/actions/workflows/cmake-multi-platform.yml/badge.svg)
## Features

- [x] Cross Platform
- [ ] Vulkan
- [ ] GLFW
- [ ] GLM
- [ ] SPIRV-Cross
- [ ] Shader Hot Reloading
- [ ] ImGUI

### Features I want to add

- [ ] Fully Dynamic Vulkan
- [ ] Scene Graph
- [ ] ECS
- [ ] PBR

## Building

### Windows and Linux

1. Clone the repository
2. Run `cmake -B build -S .`
3. Run `cmake --build build`

### MacOS

1. Clone the repository
2. Run `cmake -B build -S . -G Xcode`
3. Run `cmake --build build`

## Dependencies

Dependencies are managed using CPM.cmake so you don't need to install them manually. but here are the dependencies used in this project.

- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)
- [Vulkan](https://vulkan.lunarg.com/)
- [Vulkan-Loader](https://github.com/KhronosGroup/Vulkan-Loader)
- [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)
- [Vk-Bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [GLFW](https://www.glfw.org/)
