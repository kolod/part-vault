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
#include "utils.h"
#include <QFile>
#include <QDebug>

DatabaseManager::DatabaseManager(const QString& dbPath) : mDbPath(dbPath) {
    mDatabase = QSqlDatabase::addDatabase("QSQLITE");
    mDatabase.setDatabaseName(mDbPath);
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase() {
    // Check if the database file exists; if not, it will be created when we open it
    if (!mDatabase.open()) {
        qWarning() << "Failed to open database:" << mDatabase.lastError().text();
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
    if (mDatabase.isOpen()) mDatabase.close();
}

QSqlDatabase& DatabaseManager::database() {
    return mDatabase;
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

    const QString cleanedSql = stripSqlComments(sql);

    // Execute the SQL script to initialize the database schema.
    // QSqlQuery::exec() handles one statement at a time, so split on ';
    QSqlQuery query(mDatabase);
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
    return true;
}

bool DatabaseManager::addDummyData() {
    return executeScript(":/sql/dummy.sql");
}

int DatabaseManager::addCategory(const QString& name, int parentId) {
    QSqlQuery q(mDatabase);
    q.prepare("INSERT INTO categories (name, parent_id) VALUES (?, ?)");
    q.addBindValue(name);
    if (parentId < 0)
        q.addBindValue(QVariant(QMetaType::fromType<int>()));
    else
        q.addBindValue(parentId);
    if (!q.exec()) {
        qWarning() << "addCategory failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

int DatabaseManager::addPart(const QString& name, int quantity, int categoryId, int locationId) {
    QSqlQuery q(mDatabase);
    q.prepare("INSERT INTO parts (name, quantity, category_id, storage_location_id) VALUES (?, ?, ?, ?)");
    q.addBindValue(name);
    q.addBindValue(quantity);
    if (categoryId < 0)
        q.addBindValue(QVariant(QMetaType::fromType<int>()));
    else
        q.addBindValue(categoryId);
    if (locationId < 0)
        q.addBindValue(QVariant(QMetaType::fromType<int>()));
    else
        q.addBindValue(locationId);
    if (!q.exec()) {
        qWarning() << "addPart failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

int DatabaseManager::addStorageLocation(const QString& name) {
    QSqlQuery q(mDatabase);
    q.prepare("INSERT INTO storage_locations (name) VALUES (?)");
    q.addBindValue(name);
    if (!q.exec()) {
        qWarning() << "addStorageLocation failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool DatabaseManager::resetDatabase() {
    closeDatabase();

    // Delete the physical file so the next open starts from scratch
    if (QFile::exists(mDbPath) && !QFile::remove(mDbPath)) {
        qCritical() << "resetDatabase: could not delete" << mDbPath;
        return false;
    }

    qDebug() << "Database file removed, re-opening...";
    return openDatabase();
}