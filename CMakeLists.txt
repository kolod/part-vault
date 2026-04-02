cmake_minimum_required(VERSION 3.19)
project(PartVault VERSION 1.0.0 LANGUAGES CXX)

# print cmake prefix paths for sanity checking
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

option(ENABLE_TESTS "Build unit tests" ON)
option(ENABLE_IFW_INSTALLER "Build Qt Installer Framework installer" ON)
option(ENABLE_DUMMY_DATA "Populate database with dummy data on startup (for testing/demo)" OFF)

find_package(Qt6 6.7...6.11 REQUIRED COMPONENTS Core Widgets Sql LinguistTools Test)

qt_standard_project_setup()

# ---------- minizip-ng (archive export/import) ----------
include(FetchContent)

set(MZ_COMPAT          OFF CACHE BOOL "" FORCE)
set(MZ_BZIP2           OFF CACHE BOOL "" FORCE)
set(MZ_LZMA            OFF CACHE BOOL "" FORCE)
set(MZ_ZSTD            OFF CACHE BOOL "" FORCE)
set(MZ_PKCRYPT         OFF CACHE BOOL "" FORCE)
set(MZ_WZAES           OFF CACHE BOOL "" FORCE)
set(MZ_OPENSSL         OFF CACHE BOOL "" FORCE)
set(MZ_LIBBSD          OFF CACHE BOOL "" FORCE)
set(MZ_ICONV           OFF CACHE BOOL "" FORCE)
set(MZ_FETCH_LIBS      ON  CACHE BOOL "" FORCE)
set(MZ_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(MZ_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
# Prevent minizip-ng from installing its fetched zlib-ng alias target,
# which CMake 4.x rejects. PartVault links minizip statically so no
# install of the library itself is needed.
set(SKIP_INSTALL_LIBRARIES ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_FILES     ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_HEADERS   ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    minizip-ng
    GIT_REPOSITORY https://github.com/zlib-ng/minizip-ng.git
    GIT_TAG        4.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(minizip-ng)
# --------------------------------------------------------

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
