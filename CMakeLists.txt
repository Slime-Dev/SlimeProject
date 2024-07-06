 
cmake_minimum_required(VERSION 3.20)
project(SlimeProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(SlimeOdyssey)
add_subdirectory(SlimeGame)
