cmake_minimum_required(VERSION 3.19)
project(PartVault LANGUAGES CXX)

option(ENABLE_TESTS "Build unit tests" ON)

find_package(Qt6 6.7...6.11 REQUIRED COMPONENTS Core Widgets Sql LinguistTools Test)

qt_standard_project_setup()

add_subdirectory(src)

qt_add_translations(
    TARGETS PartVault
    TS_FILES PartVault_uk.ts
    SOURCE_TARGETS PartVault
)

# ── Unit tests ────────────────────────────────────────────────────────────────
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()