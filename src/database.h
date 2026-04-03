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

/**
 * @file database.h
 * @brief Database access and storage management API.
 */

/**
 * @brief Central business/data service for PartVault.
 *
 * Owns the SQLite connection, schema initialization, archive import/export,
 * and file lifecycle operations in the managed storage tree.
 */
class DatabaseManager
{
public:
    /**
     * @brief Constructs the manager for a database file.
     * @param dbPath Absolute path to SQLite database file.
     * @param initSqlPath SQL script path used for schema initialization.
     */
    DatabaseManager(const QString& dbPath, const QString& initSqlPath = QStringLiteral(":/sql/init.sql"));
    ~DatabaseManager();

    /** @brief Opens the database and initializes schema if needed. */
    bool openDatabase(bool reset = false);
    /** @brief Closes the active database connection if open. */
    void closeDatabase();
    /** @brief Returns mutable access to the underlying Qt SQL database handle. */
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

    // Reparents an existing category under newParentId (0 = top-level).
    bool reparentCategory(int categoryId, int newParentId);

    // Walks parent_id in categories/storage_locations and returns a path for UI labels.
    QString buildAncestorPath(const QString& table, int id) const;

    // Links absoluteFilePath to partId. Inserts into files if not already present.
    // Returns the file row id on success, or -1 on failure.
    int  addFile(int partId, const QString& absoluteFilePath);

    // Returns true on success.
    bool removePart(int partId);

    // Deletes the file record (and physical file inside dbDir) when no other
    // part references it. Returns true on success.
    bool removeFile(int fileId);

    // Returns number of rows deleted, or -1 on error.
    int  removeUnusedCategories();
    int  removeUnusedFiles();
    int  removeUnusedStorageLocations();

    // Gets the directory where the database file is located.
    QString databaseDirectory() const { return mDbDir; }

    // Gets the full path to the database file.
    QString absolutePath(const QString& relativePath) const { return QDir(mDbDir).filePath(relativePath); }

private:
    /** @brief File metadata plus reference count from part_files. */
    struct FileRefInfo {
        QString path;
        int refCount = 0;
    };

    QSqlDatabase mDatabase;
    QString mDbPath;
    QString mDbDir;

    bool ensureStorageDirectories();
    bool executeScript(const QString& path);
    bool initializeDatabase();

    // Deletes the file row and the physical file when it lives inside mDbDir.
    bool deleteFileRecord(int fileId, const QString& relativePath);

    bool linkFileToPart(int partId, int fileId);
    bool collectExclusiveFilesForPart(int partId, QList<QPair<int, QString>>* exclusiveFiles);
    bool deletePartRow(int partId);
    int findReusableManagedFileId(const QString& relativePath, int partId);
    int insertFileRecord(const QString& relativePath, const QString& detectedType);
    int findFileIdByPath(const QString& relativePath);
    bool fetchFileRefInfo(int fileId, FileRefInfo* info);

    QString mInitSqlPath;
};

