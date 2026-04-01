foreach(required_var IN ITEMS
    IFW_BINARYCREATOR
    IFW_BUILD_DIR
    IFW_STAGING_DIR
    IFW_PACKAGES_DIR
    IFW_PACKAGE_DATA_DIR
    IFW_CONFIG_FILE
    IFW_OUTPUT_FILE
)
    if(NOT DEFINED ${required_var})
        message(FATAL_ERROR "Missing required variable: ${required_var}")
    endif()
endforeach()

if(IFW_BINARYCREATOR STREQUAL "IFW_BINARYCREATOR-NOTFOUND" OR IFW_BINARYCREATOR STREQUAL "")
    message(FATAL_ERROR "Qt Installer Framework binarycreator not found. Configure PARTVAULT_BINARYCREATOR_EXECUTABLE first.")
endif()

file(REMOVE_RECURSE "${IFW_STAGING_DIR}")
file(REMOVE_RECURSE "${IFW_PACKAGE_DATA_DIR}")
file(MAKE_DIRECTORY "${IFW_STAGING_DIR}")
file(MAKE_DIRECTORY "${IFW_PACKAGE_DATA_DIR}")

set(_install_command "${CMAKE_COMMAND}" "--install" "${IFW_BUILD_DIR}" "--prefix" "${IFW_STAGING_DIR}")
if(DEFINED IFW_BUILD_CONFIG AND NOT IFW_BUILD_CONFIG STREQUAL "")
    list(APPEND _install_command "--config" "${IFW_BUILD_CONFIG}")
endif()

execute_process(
    COMMAND ${_install_command}
    RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Failed to stage install tree for IFW packaging.")
endif()

file(COPY "${IFW_STAGING_DIR}/" DESTINATION "${IFW_PACKAGE_DATA_DIR}")

get_filename_component(_ifw_output_dir "${IFW_OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${_ifw_output_dir}")
file(REMOVE "${IFW_OUTPUT_FILE}")

execute_process(
    COMMAND "${IFW_BINARYCREATOR}" --offline-only -c "${IFW_CONFIG_FILE}" -p "${IFW_PACKAGES_DIR}" "${IFW_OUTPUT_FILE}"
    RESULT_VARIABLE binarycreator_result
)
if(NOT binarycreator_result EQUAL 0)
    message(FATAL_ERROR "Qt Installer Framework binarycreator failed.")
endif()

message(STATUS "Created installer: ${IFW_OUTPUT_FILE}")