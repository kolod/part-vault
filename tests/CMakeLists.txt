qt_add_executable(tst_stripsqlcomments
    tst_stripsqlcomments.cpp
    ../src/utils.cpp
    ../src/utils.h
)

target_link_libraries(tst_stripsqlcomments PRIVATE Qt::Core Qt::Test)

add_test(NAME tst_stripsqlcomments COMMAND tst_stripsqlcomments)

qt_add_executable(tst_databasearchive
    tst_databasearchive.cpp
    ../src/database.cpp
    ../src/database.h
    ../src/utils.cpp
    ../src/utils.h
)

qt_add_resources(tst_databasearchive "sql"
    FILES
        ../src/sql/init.sql
)

target_link_libraries(tst_databasearchive PRIVATE Qt::Core Qt::Sql Qt::Test)

add_test(NAME tst_databasearchive COMMAND tst_databasearchive)
