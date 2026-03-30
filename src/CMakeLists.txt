qt_add_executable(PartVault
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database.cpp
    database.h
    models/categorytreemodel.cpp
    models/categorytreemodel.h
    models/storagetreemodel.cpp
    models/storagetreemodel.h
    models/partsmodel.cpp
    models/partsmodel.h
    dialogs/addpartdialog.cpp
    dialogs/addpartdialog.h
    dialogs/addcategorydialog.cpp
    dialogs/addcategorydialog.h
    dialogs/addstoragelocationdialog.cpp
    dialogs/addstoragelocationdialog.h
    utils.cpp
    utils.h
    $<$<PLATFORM_ID:Windows>:app.rc>
)

qt_add_resources(PartVault "sql"
    FILES
        sql/init.sql
        sql/dummy.sql
        icons/logo.svg
)

target_include_directories(PartVault PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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
