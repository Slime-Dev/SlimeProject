 
cmake_minimum_required(VERSION 3.20)
project(SlimeProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SHADER_DIR "${CMAKE_SOURCE_DIR}/SlimeOdyssey/resources/shaders")

add_subdirectory(SlimeOdyssey)
add_subdirectory(SlimeGame)