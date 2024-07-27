cmake_minimum_required(VERSION 3.20)
project(SlimeGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the SlimeOdyssey library
if(NOT DEFINED SHADER_DIR)
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/../SlimeOdyssey/resources/shaders")
endif()

# check if SlimeOdyssey is already defined
if(NOT TARGET SlimeOdyssey)
    message(STATUS "Adding SlimeOdyssey library")
    add_subdirectory(../SlimeOdyssey ${CMAKE_BINARY_DIR}/SlimeOdyssey)
endif()

# Source files for SlimeGame
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Enable Unity Build
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

# Link SlimeOdyssey library
target_link_libraries(${PROJECT_NAME} PRIVATE SlimeOdyssey)

# Add include directories for SlimeOdyssey and ImGui
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SLIME_INCLUDE_DIRS}
)

# Add ImGui-related compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_NO_PROTOTYPES
    IMGUI_IMPL_VULKAN_DYNAMIC_LOADER=glfwGetInstanceProcAddress
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
    IMGUI_DEFINE_MATH_OPERATORS
)

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_precompile_headers(${PROJECT_NAME} REUSE_FROM SlimeOdyssey)

# Function to create symlink or copy based on the build type
function(link_or_copy_resources TARGET_NAME)
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        COMMAND ${CMAKE_COMMAND}
        -D SOURCE_DIR=${SLIME_RESOURCES_DIR}
        -D DESTINATION_DIR="$<TARGET_FILE_DIR:${TARGET_NAME}>"
        -D BUILD_TYPE=$<CONFIG>
        -P "${SLIME_CMAKE_DIR}/CopyOrLink.cmake"
        COMMENT "Linking or copying SlimeOdyssey resources to executable directory"
    )
endfunction()

# Call the function
link_or_copy_resources(${PROJECT_NAME})

# Set the working directory for Visual Studio
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)