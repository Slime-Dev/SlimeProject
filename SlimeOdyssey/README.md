# SlimeOdyssey

Slime Odyssey is a graphics engine that uses Vulkan to render graphics. It is designed to be cross-platform and easy to use.

This project uses cmake to build as well as CPM.cmake to manage dependencies.

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/65fcce3de3e74dc3bc526bd1fc56ee3d)](https://app.codacy.com/gh/AlexMollard/SlimeOdyssey/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
## Features

- [x] Cross Platform
- [x] Vulkan
- [x] GLFW
- [x] GLM
- [x] SPIRV-Cross
- [x] ImGUI
- [x] ECS
- [x] PBR
- [ ] Shader Hot Reloading

### Features I want to add

- [ ] Fully Dynamic Vulkan
- [ ] Scene Graph

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

- Vulkan SDK 1.3.2.283.0

- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)
- [Vulkan](https://vulkan.lunarg.com/)
- [Vulkan-Loader](https://github.com/KhronosGroup/Vulkan-Loader)
- [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)
- [Vk-Bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [GLFW](https://www.glfw.org/)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
- [spdlog](https://github.com/gabime/spdlog)
- [tinyobj](https://github.com/tinyobjloader/tinyobjloader)
- [GLM](https://github.com/g-truc/glm)
- [GSL](https://github.com/microsoft/GSL)
- [IMGUI](https://github.com/ocornut/imgui)