include_guard(GLOBAL)

set(PARTVAULT_IFW_VERSION "1.0.0" CACHE STRING "PartVault installer version")
set(PARTVAULT_IFW_PUBLISHER "Oleksandr Kolodkin" CACHE STRING "PartVault installer publisher")
set(PARTVAULT_IFW_PRODUCT_URL "https://github.com/kolod/part-vault" CACHE STRING "PartVault product URL shown by the installer")
set(PARTVAULT_IFW_PACKAGE_NAME "com.partvault.app")
set(PARTVAULT_IFW_OUTPUT_BASENAME "PartVault-Installer")

if(WIN32)
    set(PARTVAULT_IFW_APP_EXECUTABLE "PartVault.exe")
    set(PARTVAULT_IFW_TARGET_DIR "@ApplicationsDir@/PartVault")
    set(_partvault_ifw_output_suffix ".exe")
else()
    set(PARTVAULT_IFW_APP_EXECUTABLE "PartVault")
    set(PARTVAULT_IFW_TARGET_DIR "@HomeDir@/PartVault")
    set(_partvault_ifw_output_suffix "")
endif()

string(TIMESTAMP PARTVAULT_IFW_RELEASE_DATE "%Y-%m-%d")

file(GLOB _partvault_ifw_candidate_dirs LIST_DIRECTORIES true
    "$ENV{QTDIR}/Tools/QtInstallerFramework/*/bin"
    "$ENV{QT_ROOT_DIR}/Tools/QtInstallerFramework/*/bin"
    "C:/Qt/Tools/QtInstallerFramework/*/bin"
    "$ENV{HOME}/Qt/Tools/QtInstallerFramework/*/bin"
)

find_program(PARTVAULT_BINARYCREATOR_EXECUTABLE
    NAMES binarycreator binarycreator.exe
    HINTS ${_partvault_ifw_candidate_dirs}
    DOC "Qt Installer Framework binarycreator executable"
)

set(PARTVAULT_IFW_ROOT_DIR "${CMAKE_BINARY_DIR}/ifw")
set(PARTVAULT_IFW_CONFIG_DIR "${PARTVAULT_IFW_ROOT_DIR}/config")
set(PARTVAULT_IFW_PACKAGES_DIR "${PARTVAULT_IFW_ROOT_DIR}/packages")
set(PARTVAULT_IFW_META_DIR "${PARTVAULT_IFW_PACKAGES_DIR}/${PARTVAULT_IFW_PACKAGE_NAME}/meta")
set(PARTVAULT_IFW_PACKAGE_DATA_DIR "${PARTVAULT_IFW_PACKAGES_DIR}/${PARTVAULT_IFW_PACKAGE_NAME}/data")
set(PARTVAULT_IFW_STAGING_DIR "${PARTVAULT_IFW_ROOT_DIR}/staging")
set(PARTVAULT_IFW_CONFIG_FILE "${PARTVAULT_IFW_CONFIG_DIR}/config.xml")
set(PARTVAULT_IFW_PACKAGE_FILE "${PARTVAULT_IFW_META_DIR}/package.xml")
set(PARTVAULT_IFW_OUTPUT_FILE "${CMAKE_BINARY_DIR}/${PARTVAULT_IFW_OUTPUT_BASENAME}-${PARTVAULT_IFW_VERSION}${_partvault_ifw_output_suffix}")

file(MAKE_DIRECTORY "${PARTVAULT_IFW_CONFIG_DIR}" "${PARTVAULT_IFW_META_DIR}")

configure_file(
    "${CMAKE_SOURCE_DIR}/installer/config/config.xml.in"
    "${PARTVAULT_IFW_CONFIG_FILE}"
    @ONLY
)

configure_file(
    "${CMAKE_SOURCE_DIR}/installer/packages/com.partvault.app/meta/package.xml.in"
    "${PARTVAULT_IFW_PACKAGE_FILE}"
    @ONLY
)

configure_file(
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "${PARTVAULT_IFW_META_DIR}/LICENSE"
    COPYONLY
)

add_custom_target(ifw_installer
    COMMAND "${CMAKE_COMMAND}"
        -DIFW_BINARYCREATOR=${PARTVAULT_BINARYCREATOR_EXECUTABLE}
        -DIFW_BUILD_DIR=${CMAKE_BINARY_DIR}
        -DIFW_BUILD_CONFIG=$<CONFIG>
        -DIFW_STAGING_DIR=${PARTVAULT_IFW_STAGING_DIR}
        -DIFW_PACKAGES_DIR=${PARTVAULT_IFW_PACKAGES_DIR}
        -DIFW_PACKAGE_DATA_DIR=${PARTVAULT_IFW_PACKAGE_DATA_DIR}
        -DIFW_CONFIG_FILE=${PARTVAULT_IFW_CONFIG_FILE}
        -DIFW_OUTPUT_FILE=${PARTVAULT_IFW_OUTPUT_FILE}
        -P "${CMAKE_SOURCE_DIR}/cmake/BuildIfwInstaller.cmake"
    DEPENDS PartVault
    USES_TERMINAL
    COMMENT "Building Qt Installer Framework installer"
)

if(PARTVAULT_BINARYCREATOR_EXECUTABLE)
    message(STATUS "Qt IFW binarycreator: ${PARTVAULT_BINARYCREATOR_EXECUTABLE}")
else()
    message(WARNING "Qt Installer Framework binarycreator not found. Set PARTVAULT_BINARYCREATOR_EXECUTABLE to build the ifw_installer target.")
endif()