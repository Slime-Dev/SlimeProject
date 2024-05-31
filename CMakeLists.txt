cmake_minimum_required(VERSION 3.28)

project(SlimeOdyssey)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCES "src/*.cpp")

# enable testing
enable_testing()

add_executable(SlimeOdyssey ${SOURCES})


# if on linux, pass exe directory to the shell script
# if(UNIX AND NOT APPLE)
#   add_test(NAME OpenClose COMMAND ../../tests/open_close.sh $<TARGET_FILE:SlimeOdyssey>)
# endif()

# if on windows, pass exe directory to the bat script
if(WIN32)
  add_test(NAME OpenClose COMMAND ../../tests/open_close.bat $<TARGET_FILE:SlimeOdyssey>)
endif()


# using CPM to download and build GLFW
include(cmake/CPM.cmake)
CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.3.4
  OPTIONS "GLFW_BUILD_DOCS OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_EXAMPLES OFF"
)

target_link_libraries(SlimeOdyssey glfw)
