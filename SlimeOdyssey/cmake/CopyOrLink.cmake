if(BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build detected.")
    message(STATUS "Copying resources: ${SOURCE_DIR} -> ${DESTINATION_DIR}")
    
    # Remove the destination directory if it exists
    if(EXISTS "${DESTINATION_DIR}/resources")
        file(REMOVE_RECURSE "${DESTINATION_DIR}/resources")
    endif()
    
    # Create the destination directory
    file(MAKE_DIRECTORY "${DESTINATION_DIR}")
    
    # Copy SOURCE_DIR to DESTINATION_DIR
    file(COPY "${SOURCE_DIR}" DESTINATION "${DESTINATION_DIR}")
else()
    message(STATUS "Debug build detected.")
    if(WIN32)
        # Convert forward slashes to backslashes for Windows
        string(REPLACE "/" "\\" SOURCE_DIR_WIN "${SOURCE_DIR}")
        string(REPLACE "/" "\\" DESTINATION_DIR_WIN "${DESTINATION_DIR}")
        
        message(STATUS "Creating junction: ${SOURCE_DIR_WIN} -> ${DESTINATION_DIR_WIN}")
        execute_process(
            COMMAND cmd /c mklink /J "${DESTINATION_DIR_WIN}" "${SOURCE_DIR_WIN}"
            RESULT_VARIABLE link_result
            ERROR_VARIABLE link_error
        )
    else()
        message(STATUS "Creating symlink: ${SOURCE_DIR} -> ${DESTINATION_DIR}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${SOURCE_DIR}" "${DESTINATION_DIR}"
            RESULT_VARIABLE link_result
            ERROR_VARIABLE link_error
        )
    endif()
    
    if(NOT link_result EQUAL 0)
        message(WARNING "Failed to create junction/symlink: ${link_error}")
    endif()
endif()