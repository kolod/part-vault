cmake_minimum_required(VERSION 3.19)
project(PartVault VERSION 1.0.0 LANGUAGES CXX)

option(ENABLE_TESTS "Build unit tests" ON)
option(ENABLE_IFW_INSTALLER "Build Qt Installer Framework installer" ON)
option(ENABLE_DUMMY_DATA "Populate database with dummy data on startup (for testing/demo)" OFF)

find_package(Qt6 6.7...6.11 REQUIRED COMPONENTS Core Widgets Sql LinguistTools Test)

qt_standard_project_setup()

add_subdirectory(src)

# Installer
if(ENABLE_IFW_INSTALLER)
    include(cmake/QtIfwInstaller.cmake)
endif()

# Unit tests
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# CPack / Debian package
set(CPACK_PACKAGE_NAME                  "partvault")
set(CPACK_PACKAGE_VENDOR                "Oleksandr Kolodkin")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "Electronic components inventory manager")
set(CPACK_PACKAGE_VERSION               "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT               "Oleksandr Kolodkin")
set(CPACK_DEBIAN_PACKAGE_SECTION        "electronics")
set(CPACK_DEBIAN_PACKAGE_DEPENDS        "libgl1")
set(CPACK_STRIP_FILES                   TRUE)
include(CPack)
