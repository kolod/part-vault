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
#include <QFileInfo>
#include <QDebug>

namespace {

QByteArray dummyFileContents(const QString& path, const QString& type, const QString& description) {
    if (path.endsWith(".pdf", Qt::CaseInsensitive)) {
        return QByteArrayLiteral(
            "%PDF-1.1\n"
            "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
            "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
            "3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 300 144]/Contents 4 0 R"
            "/Resources<</Font<</F1 5 0 R>>>>>>endobj\n"
            "4 0 obj<</Length 44>>stream\n"
            "BT /F1 18 Tf 36 96 Td (PartVault dummy PDF) Tj ET\n"
            "endstream endobj\n"
            "5 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
            "xref\n0 6\n"
            "0000000000 65535 f \n"
            "0000000009 00000 n \n"
            "0000000058 00000 n \n"
            "0000000115 00000 n \n"
            "0000000241 00000 n \n"
            "0000000335 00000 n \n"
            "trailer<</Size 6/Root 1 0 R>>\nstartxref\n405\n%%EOF\n"
        );
    }

    if (path.endsWith(".step", Qt::CaseInsensitive)) {
        return QByteArrayLiteral(
            "ISO-10303-21;\n"
            "HEADER;\n"
            "FILE_DESCRIPTION(('PartVault dummy STEP'),'2;1');\n"
            "FILE_NAME('dummy.step','2026-04-01T00:00:00',('PartVault'),('PartVault'),'','','');\n"
            "FILE_SCHEMA(('CONFIG_CONTROL_DESIGN'));\n"
            "ENDSEC;\n"
            "DATA;\n"
            "ENDSEC;\n"
            "END-ISO-10303-21;\n"
        );
    }

    return QString(
        "PartVault dummy file\n"
        "Type: %1\n"
        "Path: %2\n"
        "Description: %3\n"
    ).arg(type, path, description).toUtf8();
}

bool createDummyFile(const QString& absPath, const QString& relativePath, const QString& type, const QString& description) {
    const QFileInfo fileInfo(absPath);
    QDir parentDir = fileInfo.dir();
    if (!parentDir.exists() && !QDir().mkpath(parentDir.path())) {
        qWarning() << "Failed to create dummy file directory:" << parentDir.path();
        return false;
    }

    if (QFile::exists(absPath)) {
        return true;
    }

    QFile file(absPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to create dummy file:" << absPath;
        return false;
    }

    const QByteArray contents = dummyFileContents(relativePath, type, description);
    if (file.write(contents) != contents.size()) {
        qWarning() << "Failed to write dummy file:" << absPath;
        return false;
    }

    return true;
}

} // namespace

DatabaseManager::DatabaseManager(const QString& dbPath)
    : mDbPath(dbPath), mDbDir(QFileInfo(dbPath).absolutePath())
{
    mDatabase = QSqlDatabase::addDatabase("QSQLITE");
    mDatabase.setDatabaseName(mDbPath);
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::ensureStorageDirectories() {
    QDir dbDir(mDbDir);
    const QStringList requiredDirs = {"cad", "datasheets", "models"};

    for (const QString& dirName : requiredDirs) {
        if (!dbDir.mkpath(dirName)) {
            qCritical() << "Failed to create storage directory:" << dbDir.filePath(dirName);
            return false;
        }
    }

    return true;
}

bool DatabaseManager::openDatabase(bool reset) {
    if (reset) {
        closeDatabase();
        if (QFile::exists(mDbPath) && !QFile::remove(mDbPath)) {
            qCritical() << "openDatabase(reset): could not delete" << mDbPath;
            return false;
        }
        qDebug() << "Database file removed, re-opening...";
    }

    if (!ensureStorageDirectories()) {
        return false;
    }

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
    if (!executeScript(":/sql/dummy.sql")) {
        return false;
    }

    QSqlQuery query(mDatabase);
    if (!query.exec("SELECT path, type, COALESCE(description, '') FROM files ORDER BY id")) {
        qWarning() << "addDummyData failed to query dummy files:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        const QString relativePath = query.value(0).toString();
        const QString type = query.value(1).toString();
        const QString description = query.value(2).toString();
        const QString absPath = absolutePath(relativePath);
        if (!createDummyFile(absPath, relativePath, type, description)) {
            return false;
        }
    }

    return true;
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

bool DatabaseManager::removePart(int partId) {
    QSqlQuery q(mDatabase);
    q.prepare("DELETE FROM parts WHERE id = ?");
    q.addBindValue(partId);
    if (!q.exec()) {
        qWarning() << "removePart failed:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

int DatabaseManager::addStorageLocation(const QString& name, int parentId) {
    QSqlQuery q(mDatabase);
    q.prepare("INSERT INTO storage_locations (name, parent_id) VALUES (?, ?)");
    q.addBindValue(name);
    if (parentId < 0)
        q.addBindValue(QVariant(QMetaType::fromType<int>()));
    else
        q.addBindValue(parentId);
    if (!q.exec()) {
        qWarning() << "addStorageLocation failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

int DatabaseManager::removeUnusedCategories() {
    // A category is "used" if it directly holds a part OR is an ancestor of one.
    // We walk UP from every category that has a part to mark all ancestors as used,
    // then delete everything that is not used (excluding the virtual root id=0).
    QSqlQuery q(mDatabase);
    const bool ok = q.exec(
        "WITH RECURSIVE used(id) AS ("
        "  SELECT DISTINCT category_id FROM parts WHERE category_id IS NOT NULL"
        "  UNION ALL"
        "  SELECT c.parent_id FROM categories c JOIN used u ON c.id = u.id"
        "   WHERE c.parent_id IS NOT NULL AND c.parent_id > 0"
        ")"
        "DELETE FROM categories WHERE id > 0 AND id NOT IN (SELECT id FROM used)"
    );
    if (!ok) {
        qWarning() << "removeUnusedCategories failed:" << q.lastError().text();
        return -1;
    }
    return q.numRowsAffected();
}

int DatabaseManager::removeUnusedFiles() {
    QSqlQuery selectQuery(mDatabase);
    const bool selectOk = selectQuery.exec(
        "SELECT f.id, f.path "
        "FROM files AS f "
        "LEFT JOIN part_files AS pf ON pf.file_id = f.id "
        "WHERE pf.file_id IS NULL"
    );
    if (!selectOk) {
        qWarning() << "removeUnusedFiles select failed:" << selectQuery.lastError().text();
        return -1;
    }

    QList<QPair<int, QString>> unusedFiles;
    while (selectQuery.next()) {
        unusedFiles.append({selectQuery.value(0).toInt(), selectQuery.value(1).toString()});
    }

    if (unusedFiles.isEmpty()) {
        return 0;
    }

    if (!mDatabase.transaction()) {
        qWarning() << "removeUnusedFiles transaction start failed:" << mDatabase.lastError().text();
        return -1;
    }

    QSqlQuery deleteQuery(mDatabase);
    deleteQuery.prepare("DELETE FROM files WHERE id = ?");

    int removedCount = 0;
    for (const auto& unusedFile : unusedFiles) {
        const QString absPath = absolutePath(unusedFile.second);
        if (QFile::exists(absPath) && !QFile::remove(absPath)) {
            qWarning() << "removeUnusedFiles failed to delete file:" << absPath;
            mDatabase.rollback();
            return -1;
        }

        deleteQuery.bindValue(0, unusedFile.first);
        if (!deleteQuery.exec()) {
            qWarning() << "removeUnusedFiles delete failed:" << deleteQuery.lastError().text();
            mDatabase.rollback();
            return -1;
        }
        ++removedCount;
    }

    if (!mDatabase.commit()) {
        qWarning() << "removeUnusedFiles commit failed:" << mDatabase.lastError().text();
        return -1;
    }

    return removedCount;
}

int DatabaseManager::removeUnusedStorageLocations() {
    QSqlQuery q(mDatabase);
    const bool ok = q.exec(
        "WITH RECURSIVE used(id) AS ("
        "  SELECT DISTINCT storage_location_id FROM parts WHERE storage_location_id IS NOT NULL"
        "  UNION ALL"
        "  SELECT s.parent_id FROM storage_locations s JOIN used u ON s.id = u.id"
        "   WHERE s.parent_id IS NOT NULL AND s.parent_id > 0"
        ")"
        "DELETE FROM storage_locations WHERE id > 0 AND id NOT IN (SELECT id FROM used)"
    );
    if (!ok) {
        qWarning() << "removeUnusedStorageLocations failed:" << q.lastError().text();
        return -1;
    }
    return q.numRowsAffected();
}

bool DatabaseManager::resetDatabase() {
    return openDatabase(true);
}