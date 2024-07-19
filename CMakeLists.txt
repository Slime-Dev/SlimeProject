cmake_minimum_required(VERSION 3.20)
project(SlimeProject VERSION 1.0.0 LANGUAGES CXX)

option(ENABLE_TESTING "Enable testing" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SHADER_DIR "${CMAKE_SOURCE_DIR}/SlimeOdyssey/resources/shaders")

if(MSVC)
  add_compile_options(/MP)
endif()

if(NOT MSVC)
  include(ProcessorCount)
  ProcessorCount(N)
  if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
  endif()
endif()

add_subdirectory(SlimeOdyssey)

if(ENABLE_TESTING)
    message("Testing Enabled, only including Slime Tests")
    enable_testing()
    add_subdirectory(SlimeTests)
else()
    message("Testing Disabled, only including Slime Game")
    add_subdirectory(SlimeGame)
endif()

get_property(TARGETS DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
foreach(TARGET ${TARGETS})
  if(MSVC)
    target_compile_options(${TARGET} PRIVATE /MP)
  endif()
endforeach()