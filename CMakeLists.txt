cmake_minimum_required(VERSION 3.19)
project(PartVault LANGUAGES CXX)

find_package(Qt6 6.7...6.11 REQUIRED COMPONENTS Core Widgets Sql LinguistTools)

qt_standard_project_setup()

qt_add_executable(PartVault
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database.cpp
    database.h
    categorytreemodel.cpp
    categorytreemodel.h
    partsmodel.cpp
    partsmodel.h
    $<$<PLATFORM_ID:Windows>:app.rc>
)

qt_add_resources(PartVault "sql"
    FILES
        sql/init.sql
        sql/dummy.sql
        icons/logo.svg
)

qt_add_translations(
    TARGETS PartVault
    TS_FILES PartVault_uk.ts
)

target_link_libraries(PartVault
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
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
