cmake_minimum_required(VERSION 3.19)
project(PartVault LANGUAGES CXX)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(PartVault
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

qt_add_translations(
    TARGETS PartVault
    TS_FILES PartVault_uk.ts
)

target_link_libraries(PartVault
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS PartVault
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET PartVault
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
