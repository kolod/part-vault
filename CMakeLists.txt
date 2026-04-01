cmake_minimum_required(VERSION 3.19)
project(PartVault LANGUAGES CXX)

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
