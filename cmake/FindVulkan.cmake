# FindVulkan.cmake
# This file helps find and set up Vulkan SDK on Windows
# If Vulkan is not found, it attempts to download and install it

# Prevent multiple inclusion
if(DEFINED __FIND_VULKAN_CMAKE__)
    return()
endif()
set(__FIND_VULKAN_CMAKE__ TRUE)

include(FetchContent)

# Function to download and install Vulkan SDK
function(download_vulkan_sdk)
    set(VULKAN_VERSION "1.3.283.0")
    set(VULKAN_URL "https://sdk.lunarg.com/sdk/download/${VULKAN_VERSION}/windows/VulkanSDK-${VULKAN_VERSION}-Installer.exe")
    set(VULKAN_INSTALLER "${CMAKE_BINARY_DIR}/VulkanSDK-${VULKAN_VERSION}-Installer.exe")

    message(STATUS "Downloading Vulkan SDK...")
    file(DOWNLOAD ${VULKAN_URL} ${VULKAN_INSTALLER} SHOW_PROGRESS)

    message(STATUS "Please run the downloaded installer manually: ${VULKAN_INSTALLER}")
    message(STATUS "After installation, rerun CMake configuration.")
endfunction()

# Function to find Vulkan library
function(find_vulkan_library)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(VULKAN_LIB_PATH "${Vulkan_INCLUDE_DIRS}/../Lib/vulkan-1.lib")
    else()
        set(VULKAN_LIB_PATH "${Vulkan_INCLUDE_DIRS}/../Lib32/vulkan-1.lib")
    endif()

    if(EXISTS ${VULKAN_LIB_PATH})
        set(Vulkan_LIBRARIES ${VULKAN_LIB_PATH} PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Vulkan library not found at expected path: ${VULKAN_LIB_PATH}")
    endif()
endfunction()

# First, check if VULKAN_SDK is set in the environment
if(DEFINED ENV{VULKAN_SDK})
    set(Vulkan_INCLUDE_DIRS "$ENV{VULKAN_SDK}/Include")
    find_vulkan_library()
    set(Vulkan_FOUND TRUE)
else()
    # If VULKAN_SDK is not set, try to find Vulkan using CMake's built-in module
    find_package(Vulkan QUIET)
    
    if(NOT Vulkan_FOUND)
        # If Vulkan is still not found, attempt to download and install it
        download_vulkan_sdk()
        
        # After download, check again for VULKAN_SDK
        if(DEFINED ENV{VULKAN_SDK})
            set(Vulkan_INCLUDE_DIRS "$ENV{VULKAN_SDK}/Include")
            find_vulkan_library()
            set(Vulkan_FOUND TRUE)
        endif()
    endif()
endif()

# Verify that we found Vulkan
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found and couldn't be installed automatically. Please install Vulkan SDK manually and ensure it's in your system PATH or set the VULKAN_SDK environment variable.")
endif()

# Print Vulkan configuration
message(STATUS "Vulkan Include Dirs: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Libraries: ${Vulkan_LIBRARIES}")

# Function to set up Vulkan for a target
function(target_setup_vulkan target)
    target_include_directories(${target} PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(${target} PRIVATE ${Vulkan_LIBRARIES})
endfunction()

# Set up SPIRV-Tools paths
set(SPIRV-Tools_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS}/../)
set(SPIRV-Tools_LIBRARIES ${Vulkan_LIBRARIES}/../Lib)
