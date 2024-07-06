include(FindPackageHandleStandardArgs)

# Try to find Vulkan SDK
find_package(Vulkan QUIET)

if(NOT Vulkan_FOUND)
    # If CMake couldn't find Vulkan, let's try to locate it manually
    if(WIN32)
        find_path(Vulkan_INCLUDE_DIR
                NAMES vulkan/vulkan.h
                PATHS
                "$ENV{VULKAN_SDK}/Include"
                "$ENV{VK_SDK_PATH}/Include"
        )
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            find_library(Vulkan_LIBRARY
                    NAMES vulkan-1
                    PATHS
                    "$ENV{VULKAN_SDK}/Lib"
                    "$ENV{VK_SDK_PATH}/Lib"
            )
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            find_library(Vulkan_LIBRARY
                    NAMES vulkan-1
                    PATHS
                    "$ENV{VULKAN_SDK}/Lib32"
                    "$ENV{VK_SDK_PATH}/Lib32"
            )
        endif()
    else()
        find_path(Vulkan_INCLUDE_DIR
                NAMES vulkan/vulkan.h
                PATHS
                "$ENV{VULKAN_SDK}/include"
                "/usr/include"
                "/usr/local/include"
        )
        find_library(Vulkan_LIBRARY
                NAMES vulkan
                PATHS
                "$ENV{VULKAN_SDK}/lib"
                "/usr/lib"
                "/usr/local/lib"
        )
    endif()

    if(Vulkan_INCLUDE_DIR AND Vulkan_LIBRARY)
        set(Vulkan_FOUND TRUE)
        set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})
        set(Vulkan_LIBRARIES ${Vulkan_LIBRARY})
    endif()

    mark_as_advanced(Vulkan_INCLUDE_DIR Vulkan_LIBRARY)
endif()

# Handle the QUIETLY and REQUIRED arguments and set VULKAN_FOUND to TRUE if all listed variables are TRUE
find_package_handle_standard_args(Vulkan
        DEFAULT_MSG
        Vulkan_LIBRARY Vulkan_INCLUDE_DIR
)

if(Vulkan_FOUND)
    set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
    if(WIN32)
        string(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
    else()
        string(REGEX REPLACE "/include" "" VULKAN_PATH ${VULKAN_PATH})
    endif()
    message(STATUS "Vulkan SDK found: ${VULKAN_PATH}")
    message(STATUS "Vulkan include directory: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan library: ${Vulkan_LIBRARIES}")
else()
    message(FATAL_ERROR "
        Unable to locate Vulkan SDK. Please ensure that:
        1. Vulkan SDK is installed on your system.
        2. VULKAN_SDK environment variable is set to the installation path.
        3. On Windows, VK_SDK_PATH environment variable is set as an alternative.

        If Vulkan is installed in a non-standard location, you can specify it manually:
        -DVulkan_INCLUDE_DIR=/path/to/vulkan/include
        -DVulkan_LIBRARY=/path/to/vulkan/lib/libvulkan.so (or vulkan-1.lib on Windows)
    ")
endif()

# Add Vulkan as a target
if(NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
            IMPORTED_LOCATION "${Vulkan_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}"
    )
endif()