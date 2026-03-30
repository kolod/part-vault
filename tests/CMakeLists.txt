qt_add_executable(tst_stripsqlcomments
    tst_stripsqlcomments.cpp
    ../src/utils.cpp
    ../src/utils.h
)

target_link_libraries(tst_stripsqlcomments PRIVATE Qt::Core Qt::Test)

add_test(NAME tst_stripsqlcomments COMMAND tst_stripsqlcomments)
