# Function to compile Vulkan shaders to SPIR-V
function(compile_shaders)
    message(STATUS "Compiling shaders to SPIR-V")

    set(shader_dir "${CMAKE_SOURCE_DIR}/shaders")
    message(STATUS "Shader directory: ${shader_dir}")

    # get search dir for glslc
    if (WIN32)
        set(VULKAN_SDK "$ENV{VULKAN_SDK}")
        message(STATUS "VULKAN_SDK: ${VULKAN_SDK}")
    else ()
        set(VULKAN_SDK "/usr")
    endif ()

    # Find glslc executable
    if (WIN32)
        find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc.exe HINTS "${VULKAN_SDK}/Bin" REQUIRED)
    else ()
        find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS "${VULKAN_SDK}/bin" REQUIRED)
    endif ()

    if(NOT Vulkan_GLSLC_EXECUTABLE)
        message(FATAL_ERROR "glslc executable not found. Set Vulkan_GLSLC_EXECUTABLE to the path of your glslc executable.")
    endif()
    message(STATUS "Found glslc executable: ${Vulkan_GLSLC_EXECUTABLE}")

    # Find all shader source files
    file(GLOB_RECURSE shader_sources
            "${shader_dir}/*.vert"
            "${shader_dir}/*.frag"
            "${shader_dir}/*.comp"
            "${shader_dir}/*.geom"
            "${shader_dir}/*.tesc"
            "${shader_dir}/*.tese"
    )

    # Create a list to hold compiled shader outputs
    set(shader_outputs "")

    # Iterate over each shader source file
    foreach(shader_source ${shader_sources})
        # Determine output file name
        get_filename_component(shader_name ${shader_source} NAME_WE)
        get_filename_component(shader_extension ${shader_source} EXT)

        set(shader_output "${shader_dir}/${shader_name}${shader_extension}.spv")

        # Add a command to compile shader
        add_custom_command(
                OUTPUT ${shader_output}
                COMMAND ${Vulkan_GLSLC_EXECUTABLE} -o ${shader_output} ${shader_source}
                DEPENDS ${shader_source}
                COMMENT "Compiling ${shader_source} to SPIR-V"
        )

        # Append compiled shader output to the list
        list(APPEND shader_outputs ${shader_output})
    endforeach()

    # Add a custom target to trigger shader compilation
    add_custom_target(ShadersTarget DEPENDS ${shader_outputs})

    # Add the shaders target to default build
    add_dependencies(SlimeOdyssey ShadersTarget)
endfunction()
