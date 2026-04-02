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

#include "stdint.h"

#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

#include "database.h"
#include "utils.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>
#include <QDateTime>
#include <QDirIterator>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QStringList>
#include <QTemporaryDir>

namespace {

const QString kManifestFileName = QStringLiteral("partvault.json");
const int kArchiveFormatVersion = 1;

bool copyDirectoryContents(const QString& sourceDirPath, const QString& targetDirPath, QString* errorMessage) {
    const QDir sourceDir(sourceDirPath);
    if (!sourceDir.exists()) {
        if (errorMessage) *errorMessage = QStringLiteral("Source directory does not exist: %1").arg(sourceDirPath);
        return false;
    }

    QDir targetDir(targetDirPath);
    if (!targetDir.exists() && !QDir().mkpath(targetDirPath)) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create target directory: %1").arg(targetDirPath);
        return false;
    }

    QDirIterator iterator(sourceDirPath, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        const QString relativePath = sourceDir.relativeFilePath(info.absoluteFilePath());
        const QString destinationPath = targetDir.filePath(relativePath);

        if (info.isDir()) {
            if (!QDir().mkpath(destinationPath)) {
                if (errorMessage) *errorMessage = QStringLiteral("Failed to create directory: %1").arg(destinationPath);
                return false;
            }
            continue;
        }

        const QFileInfo destinationInfo(destinationPath);
        if (!QDir().mkpath(destinationInfo.absolutePath())) {
            if (errorMessage) *errorMessage = QStringLiteral("Failed to create parent directory: %1").arg(destinationInfo.absolutePath());
            return false;
        }
        if (QFile::exists(destinationPath) && !QFile::remove(destinationPath)) {
            if (errorMessage) *errorMessage = QStringLiteral("Failed to overwrite file: %1").arg(destinationPath);
            return false;
        }
        if (!QFile::copy(info.absoluteFilePath(), destinationPath)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Failed to copy file: %1 -> %2")
                    .arg(info.absoluteFilePath(), destinationPath);
            }
            return false;
        }
    }

    return true;
}

bool runZipCommand(const QString& archivePath, const QString& sourceDirPath, QString* errorMessage) {
    QFile::remove(archivePath);

    void* writer = mz_zip_writer_create();
    if (!writer) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to allocate zip writer");
        return false;
    }

    mz_zip_writer_set_compress_method(writer, MZ_COMPRESS_METHOD_DEFLATE);
    mz_zip_writer_set_compress_level(writer, MZ_COMPRESS_LEVEL_DEFAULT);

    const QByteArray archiveBytes   = archivePath.toUtf8();
    const QByteArray sourceDirBytes = sourceDirPath.toUtf8();

    int32_t err = mz_zip_writer_open_file(writer, archiveBytes.constData(), 0, 0);
    if (err != MZ_OK) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create archive (mz error %1)").arg(err);
        mz_zip_writer_delete(&writer);
        return false;
    }

    err = mz_zip_writer_add_path(writer, sourceDirBytes.constData(), sourceDirBytes.constData(), 0, 1);
    if (err != MZ_OK && err != MZ_END_OF_LIST) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to add files to archive (mz error %1)").arg(err);
        mz_zip_writer_close(writer);
        mz_zip_writer_delete(&writer);
        return false;
    }

    err = mz_zip_writer_close(writer);
    mz_zip_writer_delete(&writer);
    if (err != MZ_OK) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to finalise archive (mz error %1)").arg(err);
        return false;
    }

    return true;
}

bool runUnzipCommand(const QString& archivePath, const QString& destinationDirPath, QString* errorMessage) {
    void* reader = mz_zip_reader_create();
    if (!reader) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to allocate zip reader");
        return false;
    }

    const QByteArray archiveBytes = archivePath.toUtf8();
    const QByteArray destDirBytes = destinationDirPath.toUtf8();

    int32_t err = mz_zip_reader_open_file(reader, archiveBytes.constData());
    if (err != MZ_OK) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to open archive (mz error %1)").arg(err);
        mz_zip_reader_delete(&reader);
        return false;
    }

    err = mz_zip_reader_save_all(reader, destDirBytes.constData());
    if (err != MZ_OK && err != MZ_END_OF_LIST) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to extract archive (mz error %1)").arg(err);
        mz_zip_reader_close(reader);
        mz_zip_reader_delete(&reader);
        return false;
    }

    mz_zip_reader_close(reader);
    mz_zip_reader_delete(&reader);
    return true;
}

QString inferFileType(const QString& relativePath) {
    const QString lower = relativePath.toLower();
    if (lower.endsWith(QStringLiteral(".pdf"))) {
        return QStringLiteral("datasheet");
    }
    if (lower.endsWith(QStringLiteral(".step")) || lower.endsWith(QStringLiteral(".stp"))
        || lower.endsWith(QStringLiteral(".iges")) || lower.endsWith(QStringLiteral(".igs"))
        || lower.endsWith(QStringLiteral(".stl"))) {
        return QStringLiteral("model");
    }
    return QStringLiteral("cad");
}

QJsonArray exportPartsArray(QSqlDatabase& database) {
    QJsonArray partsArray;

    QSqlQuery partsQuery(database);
    const bool ok = partsQuery.exec(
        "WITH RECURSIVE "
        "category_path(id, path) AS ("
        "  SELECT id, '' FROM categories WHERE id = 0 "
        "  UNION ALL "
        "  SELECT c.id, CASE WHEN cp.path = '' THEN c.name ELSE cp.path || '/' || c.name END "
        "  FROM categories c JOIN category_path cp ON c.parent_id = cp.id "
        "), "
        "location_path(id, path) AS ("
        "  SELECT id, '' FROM storage_locations WHERE id = 0 "
        "  UNION ALL "
        "  SELECT s.id, CASE WHEN lp.path = '' THEN s.name ELSE lp.path || '/' || s.name END "
        "  FROM storage_locations s JOIN location_path lp ON s.parent_id = lp.id "
        ") "
        "SELECT p.id, p.name, p.quantity, "
        "       COALESCE(cp.path, ''), "
        "       COALESCE(lp.path, '') "
        "FROM parts p "
        "LEFT JOIN category_path cp ON cp.id = p.category_id "
        "LEFT JOIN location_path lp ON lp.id = p.storage_location_id "
        "ORDER BY p.id"
    );
    if (!ok) {
        qWarning() << "exportPartsArray failed:" << partsQuery.lastError().text();
        return partsArray;
    }

    QSqlQuery filesQuery(database);
    filesQuery.prepare(
        "SELECT f.path "
        "FROM files f "
        "JOIN part_files pf ON pf.file_id = f.id "
        "WHERE pf.part_id = ? "
        "ORDER BY f.path"
    );

    while (partsQuery.next()) {
        const int partId = partsQuery.value(0).toInt();
        QJsonObject partObject;
        partObject.insert(QStringLiteral("name"), partsQuery.value(1).toString());
        partObject.insert(QStringLiteral("quantity"), partsQuery.value(2).toInt());
        partObject.insert(QStringLiteral("category"), partsQuery.value(3).toString());
        partObject.insert(QStringLiteral("location"), partsQuery.value(4).toString());

        QJsonArray filesArray;
        filesQuery.bindValue(0, partId);
        if (filesQuery.exec()) {
            while (filesQuery.next()) {
                filesArray.append(filesQuery.value(0).toString());
            }
        } else {
            qWarning() << "exportPartsArray files query failed:" << filesQuery.lastError().text();
        }
        if (!filesArray.isEmpty()) {
            partObject.insert(QStringLiteral("files"), filesArray);
        }
        partsArray.append(partObject);
    }

    return partsArray;
}

bool ensurePathNode(
    QSqlDatabase& database,
    const QString& tableName,
    const QString& fullPath,
    QHash<QString, int>& cache,
    int* outId,
    QString* errorMessage
) {
    if (fullPath.trimmed().isEmpty()) {
        if (outId) *outId = -1;
        return true;
    }

    if (cache.contains(fullPath)) {
        if (outId) *outId = cache.value(fullPath);
        return true;
    }

    int parentId = 0;
    QString currentPath;
    const QStringList segments = fullPath.split('/', Qt::SkipEmptyParts);
    for (const QString& rawSegment : segments) {
        const QString segment = rawSegment.trimmed();
        if (segment.isEmpty()) {
            continue;
        }

        currentPath = currentPath.isEmpty() ? segment : currentPath + QStringLiteral("/") + segment;
        if (cache.contains(currentPath)) {
            parentId = cache.value(currentPath);
            continue;
        }

        QSqlQuery selectQuery(database);
        selectQuery.prepare(QStringLiteral("SELECT id FROM %1 WHERE name = ? AND parent_id = ?").arg(tableName));
        selectQuery.addBindValue(segment);
        selectQuery.addBindValue(parentId);
        if (!selectQuery.exec()) {
            if (errorMessage) *errorMessage = selectQuery.lastError().text();
            return false;
        }

        int nodeId = -1;
        if (selectQuery.next()) {
            nodeId = selectQuery.value(0).toInt();
        } else {
            QSqlQuery insertQuery(database);
            insertQuery.prepare(QStringLiteral("INSERT INTO %1 (name, parent_id) VALUES (?, ?)").arg(tableName));
            insertQuery.addBindValue(segment);
            insertQuery.addBindValue(parentId);
            if (!insertQuery.exec()) {
                if (errorMessage) *errorMessage = insertQuery.lastError().text();
                return false;
            }
            nodeId = insertQuery.lastInsertId().toInt();
        }

        cache.insert(currentPath, nodeId);
        parentId = nodeId;
    }

    if (outId) *outId = parentId > 0 ? parentId : -1;
    return true;
}

bool importPartsArray(QSqlDatabase& database, const QJsonArray& partsArray, QString* errorMessage) {
    QHash<QString, int> categoryPathToId;
    QHash<QString, int> locationPathToId;
    QHash<QString, int> filePathToId;

    QSqlQuery clearQuery(database);
    if (!clearQuery.exec(QStringLiteral("DELETE FROM part_files"))
        || !clearQuery.exec(QStringLiteral("DELETE FROM files"))
        || !clearQuery.exec(QStringLiteral("DELETE FROM parts"))
        || !clearQuery.exec(QStringLiteral("DELETE FROM categories WHERE id <> 0"))
        || !clearQuery.exec(QStringLiteral("DELETE FROM storage_locations WHERE id <> 0"))) {
        if (errorMessage) *errorMessage = clearQuery.lastError().text();
        return false;
    }

    QSqlQuery insertPartQuery(database);
    insertPartQuery.prepare(
        QStringLiteral("INSERT INTO parts (name, quantity, category_id, storage_location_id) VALUES (?, ?, ?, ?)")
    );

    QSqlQuery insertFileQuery(database);
    insertFileQuery.prepare(
        QStringLiteral("INSERT INTO files (path, type, description) VALUES (?, ?, ?)")
    );

    QSqlQuery linkFileQuery(database);
    linkFileQuery.prepare(
        QStringLiteral("INSERT OR IGNORE INTO part_files (part_id, file_id) VALUES (?, ?)")
    );

    for (const QJsonValue& partValue : partsArray) {
        if (!partValue.isObject()) {
            continue;
        }
        const QJsonObject partObject = partValue.toObject();
        const QString partName = partObject.value(QStringLiteral("name")).toString();
        const int quantity = partObject.value(QStringLiteral("quantity")).toInt(0);
        const QString categoryPath = partObject.value(QStringLiteral("category")).toString();
        const QString locationPath = partObject.value(QStringLiteral("location")).toString();

        int categoryId = -1;
        int locationId = -1;
        if (!ensurePathNode(database, QStringLiteral("categories"), categoryPath, categoryPathToId, &categoryId, errorMessage)
            || !ensurePathNode(database, QStringLiteral("storage_locations"), locationPath, locationPathToId, &locationId, errorMessage)) {
            return false;
        }

        insertPartQuery.bindValue(0, partName);
        insertPartQuery.bindValue(1, quantity);
        if (categoryId > 0) {
            insertPartQuery.bindValue(2, categoryId);
        } else {
            insertPartQuery.bindValue(2, QVariant(QMetaType::fromType<int>()));
        }
        if (locationId > 0) {
            insertPartQuery.bindValue(3, locationId);
        } else {
            insertPartQuery.bindValue(3, QVariant(QMetaType::fromType<int>()));
        }
        if (!insertPartQuery.exec()) {
            if (errorMessage) *errorMessage = insertPartQuery.lastError().text();
            return false;
        }
        const int partId = insertPartQuery.lastInsertId().toInt();

        QJsonArray filesArray;
        const QJsonValue filesValue = partObject.value(QStringLiteral("files"));
        if (filesValue.isArray()) {
            filesArray = filesValue.toArray();
        } else if (filesValue.isObject()) {
            const QJsonObject filesObject = filesValue.toObject();
            if (filesObject.value(QStringLiteral("paths")).isArray()) {
                filesArray = filesObject.value(QStringLiteral("paths")).toArray();
            }
        }
        for (const QJsonValue& fileValue : filesArray) {
            const QString filePath = fileValue.toString().trimmed();
            if (filePath.isEmpty()) {
                continue;
            }

            int fileId = filePathToId.value(filePath, -1);
            if (fileId < 0) {
                insertFileQuery.bindValue(0, filePath);
                insertFileQuery.bindValue(1, inferFileType(filePath));
                insertFileQuery.bindValue(2, QVariant(QMetaType::fromType<QString>()));
                if (!insertFileQuery.exec()) {
                    if (errorMessage) *errorMessage = insertFileQuery.lastError().text();
                    return false;
                }
                fileId = insertFileQuery.lastInsertId().toInt();
                filePathToId.insert(filePath, fileId);
            }

            linkFileQuery.bindValue(0, partId);
            linkFileQuery.bindValue(1, fileId);
            if (!linkFileQuery.exec()) {
                if (errorMessage) *errorMessage = linkFileQuery.lastError().text();
                return false;
            }
        }
    }

    return true;
}

QString buildJson(const QString& appVersion, int sqliteUserVersion, QSqlDatabase& database) {
    QJsonObject manifest;
    manifest.insert(QStringLiteral("formatVersion"), kArchiveFormatVersion);
    manifest.insert(QStringLiteral("appVersion"), appVersion);
    manifest.insert(QStringLiteral("sqliteUserVersion"), sqliteUserVersion);
    manifest.insert(QStringLiteral("exportedAtUtc"), QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss'Z'")));
    manifest.insert(QStringLiteral("parts"), exportPartsArray(database));
    return QString::fromUtf8(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
}

bool readManifest(const QString& manifestPath, QJsonObject* manifestObject, QString* errorMessage) {
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot open manifest file: %1").arg(manifestPath);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid manifest JSON: %1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject object = document.object();
    if (!object.contains(QStringLiteral("appVersion")) || !object.value(QStringLiteral("appVersion")).isString()) {
        if (errorMessage) *errorMessage = QStringLiteral("Manifest does not contain valid appVersion");
        return false;
    }

    if (manifestObject) *manifestObject = object;
    return true;
}

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

bool DatabaseManager::exportArchive(const QString& archivePath, QString* errorMessage) {
    if (archivePath.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Archive path is empty");
        return false;
    }

    QTemporaryDir stagingRoot;
    if (!stagingRoot.isValid()) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create temporary directory");
        return false;
    }

    const QString stagingDirPath = QDir(stagingRoot.path()).filePath(QStringLiteral("partvault-export"));
    if (!QDir().mkpath(stagingDirPath)) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create staging directory");
        return false;
    }

    if (!copyDirectoryContents(mDbDir, stagingDirPath, errorMessage)) {
        return false;
    }

    int sqliteUserVersion = 0;
    QSqlQuery versionQuery(mDatabase);
    if (versionQuery.exec(QStringLiteral("PRAGMA user_version")) && versionQuery.next()) {
        sqliteUserVersion = versionQuery.value(0).toInt();
    }

    const QString appVersion = QCoreApplication::applicationVersion();
    const QString manifestPath = QDir(stagingDirPath).filePath(kManifestFileName);
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to write manifest: %1").arg(manifestPath);
        return false;
    }
    manifestFile.write(buildJson(appVersion, sqliteUserVersion, mDatabase).toUtf8());
    manifestFile.close();

    if (!runZipCommand(archivePath, stagingDirPath, errorMessage)) {
        return false;
    }

    return true;
}

bool DatabaseManager::importArchive(const QString& archivePath, QString* errorMessage) {
    if (archivePath.isEmpty() || !QFileInfo::exists(archivePath)) {
        if (errorMessage) *errorMessage = QStringLiteral("Archive file does not exist: %1").arg(archivePath);
        return false;
    }

    QTemporaryDir extractRoot;
    if (!extractRoot.isValid()) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create temporary directory");
        return false;
    }

    const QString extractDirPath = QDir(extractRoot.path()).filePath(QStringLiteral("partvault-import"));
    if (!QDir().mkpath(extractDirPath)) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create extraction directory");
        return false;
    }

    if (!runUnzipCommand(archivePath, extractDirPath, errorMessage)) {
        return false;
    }

    QJsonObject manifest;
    const QString manifestPath = QDir(extractDirPath).filePath(kManifestFileName);
    if (!readManifest(manifestPath, &manifest, errorMessage)) {
        return false;
    }

    closeDatabase();

    QDir dbDir(mDbDir);
    const QFileInfoList currentEntries = dbDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (const QFileInfo& entry : currentEntries) {
        if (entry.isDir()) {
            if (!QDir(entry.absoluteFilePath()).removeRecursively()) {
                if (errorMessage) *errorMessage = QStringLiteral("Failed to remove directory: %1").arg(entry.absoluteFilePath());
                return false;
            }
        } else if (!QFile::remove(entry.absoluteFilePath())) {
            if (errorMessage) *errorMessage = QStringLiteral("Failed to remove file: %1").arg(entry.absoluteFilePath());
            return false;
        }
    }

    if (!copyDirectoryContents(extractDirPath, mDbDir, errorMessage)) {
        return false;
    }

    // The imported database schema may differ, so rebuild current schema and restore from plain JSON data.
    if (!openDatabase(true)) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to reset database for import");
        return false;
    }

    if (!manifest.contains(QStringLiteral("parts")) || !manifest.value(QStringLiteral("parts")).isArray()) {
        if (errorMessage) *errorMessage = QStringLiteral("Manifest does not contain valid parts array");
        return false;
    }
    const QJsonArray partsArray = manifest.value(QStringLiteral("parts")).toArray();
    if (!mDatabase.transaction()) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to start import transaction");
        return false;
    }

    if (!importPartsArray(mDatabase, partsArray, errorMessage)) {
        mDatabase.rollback();
        return false;
    }

    if (!mDatabase.commit()) {
        if (errorMessage) *errorMessage = mDatabase.lastError().text();
        return false;
    }

    return true;
}