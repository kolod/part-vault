qt_add_executable(test_utils
    test_utils.cpp
    ../src/utils.cpp
    ../src/utils.h
)

target_link_libraries(test_utils PRIVATE Qt::Core Qt::Test MINIZIP::minizip)

add_test(NAME test_utils COMMAND test_utils)

qt_add_executable(test_database
    test_database.cpp
    ../src/database.cpp
    ../src/database.h
    ../src/utils.cpp
    ../src/utils.h
)

target_compile_definitions(test_database PRIVATE
    TEST_INIT_SQL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../src/sql/init.sql"
)

target_link_libraries(test_database PRIVATE Qt::Core Qt::Sql Qt::Test MINIZIP::minizip)

add_test(NAME test_database COMMAND test_database)
