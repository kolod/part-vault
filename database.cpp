//    PartVault — simple inventory manager for electronic components
//    Copyright (C) 2026-...  Oleksandr Kolodkin <oleksandr.kolodkin@ukr.net>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "database.h"
#include <QFile>
#include <QDebug>

DatabaseManager::DatabaseManager(const QString& dbPath) : m_dbPath(dbPath) {
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(m_dbPath);
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase() {
    // Check if the database file exists; if not, it will be created when we open it
    if (!m_database.open()) {
        qWarning() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }

    // Initialize the database schema if it's a new database
    if (!initializeDatabase()) {
        qWarning() << "Failed to initialize database";
        return false;
    }

    // Database opened and initialized successfully
    qDebug() << "Database opened successfully";
    return true;
}

void DatabaseManager::closeDatabase() {
    if (m_database.isOpen()) m_database.close();
}

QSqlDatabase& DatabaseManager::database() {
    return m_database;
}

bool DatabaseManager::executeScript(const QString& path) {

    // Open the SQL script file
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Failed to open SQL script:" << path;
        return false;
    }

    // Read the entire SQL script into a QString
    const QString sql = QString::fromUtf8(file.readAll()).trimmed();
    file.close();

    // Remove comments from the SQL script (lines starting with '--')
    const QStringList lines = sql.split(QLatin1Char('\n'));
    QString cleanedSql;
    for (const QString& line : lines) {
        const QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("--") || trimmedLine.isEmpty()) continue;
        cleanedSql += line + "\n"; // Preserve original formatting for non-comment lines
    }

    // Execute the SQL script to initialize the database schema.
    // QSqlQuery::exec() handles one statement at a time, so split on ';
    QSqlQuery query(m_database);
    const QStringList statements = cleanedSql.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString& statement : statements) {

        // Trim the statement to remove leading/trailing whitespace
        const QString trimmed = statement.trimmed();

        // Skip empty statements that may result from splitting
        if (trimmed.isEmpty()) continue;

        // Execute the SQL statement
        if (!query.exec(trimmed)) {
            qCritical() << "Failed to execute SQL statement:" << query.lastError().text()
                        << "\nStatement:" << trimmed;
            return false;
        }
    }

    // All statements executed successfully
    return true;
}

bool DatabaseManager::initializeDatabase() {

    // Initialize the database schema
    if (!executeScript(":/sql/init.sql")) {
        qCritical() << "Database initialization failed";
        return false;
    }

    qDebug() << "Database initialized successfully";

    if (!addDummyData())
        qWarning() << "Failed to load dummy data (non-fatal)";

    return true;
}

bool DatabaseManager::addDummyData() {
    return executeScript(":/sql/dummy.sql");
}