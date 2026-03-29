#pragma once

#include <QtSql>

class DatabaseManager
{
public:
    DatabaseManager(const QString& dbPath);
    ~DatabaseManager();

    bool openDatabase();
    void closeDatabase();
    QSqlDatabase& database();

private:
    QSqlDatabase m_database;
    QString m_dbPath;

    bool executeScript(const QString& path);
    bool initializeDatabase();
    bool addDummyData();
};

