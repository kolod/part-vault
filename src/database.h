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

#pragma once

#include <QtSql>
#include <QDir>

class DatabaseManager
{
public:
    DatabaseManager(const QString& dbPath);
    ~DatabaseManager();

    bool openDatabase(bool reset = false, const QString& initSqlPath = QStringLiteral(":/sql/init.sql"));
    void closeDatabase();
    QSqlDatabase& database();

    bool resetDatabase();   // convenience: openDatabase(true)
    bool addDummyData();    // populates sample data

    // Exports/imports the whole storage directory as a zip archive.
    // On failure, returns false and optionally writes a human-readable error.
    bool exportArchive(const QString& archivePath, QString* errorMessage = nullptr);
    bool importArchive(const QString& archivePath, QString* errorMessage = nullptr);

    // Returns the new row id on success, or -1 on failure.
    int  addCategory       (const QString& name, int parentId);   // parentId = -1 → no parent
    int  addPart           (const QString& name, int quantity, int categoryId, int locationId);  // -1 → NULL FK
    int  addStorageLocation(const QString& name, int parentId = -1);  // parentId = -1 → no parent

    // Returns true on success.
    bool removePart(int partId);

    // Returns number of rows deleted, or -1 on error.
    int  removeUnusedCategories();
    int  removeUnusedFiles();
    int  removeUnusedStorageLocations();

    // Gets the directory where the database file is located.
    QString databaseDirectory() const { return mDbDir; }

    // Gets the full path to the database file.
    QString absolutePath(const QString& relativePath) const { return QDir(mDbDir).filePath(relativePath); }

private:
    QSqlDatabase mDatabase;
    QString mDbPath;
    QString mDbDir;

    bool ensureStorageDirectories();
    bool executeScript(const QString& path);
    bool initializeDatabase();

    QString mInitSqlPath = QStringLiteral(":/sql/init.sql");
};

