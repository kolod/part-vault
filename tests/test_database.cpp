//    PartVault - simple inventory manager for electronic components
//    Copyright (C) 2026-...  Oleksandr Kolodkin <oleksandr.kolodkin@ukr.net>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include "../src/database.h"
#include "../src/utils.h"

class TstDatabase : public QObject
{
    Q_OBJECT

private:
    static bool writeTextFile(const QString& absolutePath, const QByteArray& data)
    {
        const QFileInfo info(absolutePath);
        if (!QDir().mkpath(info.absolutePath())) {
            return false;
        }

        QFile file(absolutePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }
        return file.write(data) == data.size();
    }

private slots:
    void exportImportRoundTrip()
    {
        QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

        QTemporaryDir tempDir;
        QVERIFY2(tempDir.isValid(), "Failed to create temporary test directory");

        const QString dbPath = QDir(tempDir.path()).filePath(QStringLiteral("parts.db"));
        DatabaseManager db(dbPath);
        QVERIFY2(db.openDatabase(true), "Failed to open test database");

        const int passiveId = db.addCategory(QStringLiteral("Passive"), 0);
        QVERIFY(passiveId > 0);
        const int resistorId = db.addCategory(QStringLiteral("Resistors"), passiveId);
        QVERIFY(resistorId > 0);

        const int shelfId = db.addStorageLocation(QStringLiteral("Shelf A"), 0);
        QVERIFY(shelfId > 0);
        const int drawerId = db.addStorageLocation(QStringLiteral("Drawer 1"), shelfId);
        QVERIFY(drawerId > 0);

        const int partId = db.addPart(QStringLiteral("R1 10k"), 123, resistorId, drawerId);
        QVERIFY(partId > 0);

        QVERIFY(writeTextFile(db.absolutePath(QStringLiteral("datasheets/r1.pdf")), QByteArrayLiteral("dummy")));

        QSqlQuery insertFile(db.database());
        insertFile.prepare(QStringLiteral("INSERT INTO files (path, type, description) VALUES (?, ?, ?)"));
        insertFile.addBindValue(QStringLiteral("datasheets/r1.pdf"));
        insertFile.addBindValue(QStringLiteral("datasheet"));
        insertFile.addBindValue(QStringLiteral("R1 datasheet"));
        QVERIFY(insertFile.exec());
        const int fileId = insertFile.lastInsertId().toInt();
        QVERIFY(fileId > 0);

        QSqlQuery linkFile(db.database());
        linkFile.prepare(QStringLiteral("INSERT INTO part_files (part_id, file_id) VALUES (?, ?)"));
        linkFile.addBindValue(partId);
        linkFile.addBindValue(fileId);
        QVERIFY(linkFile.exec());

        const QString archivePath = QDir(tempDir.path()).filePath(QStringLiteral("backup.zip"));
        QString errorMessage;
        QVERIFY2(db.exportArchive(archivePath, &errorMessage), qPrintable(errorMessage));
        QVERIFY(QFileInfo::exists(archivePath));

        QTemporaryDir extracted;
        QVERIFY(extracted.isValid());
        QVERIFY2(extractZipArchive(archivePath, extracted.path(), &errorMessage), qPrintable(errorMessage));

        const QString jsonPath = QDir(extracted.path()).filePath(QStringLiteral("partvault.json"));
        QVERIFY2(QFileInfo::exists(jsonPath), "partvault.json was not found in archive");

        QFile jsonFile(jsonPath);
        QVERIFY(jsonFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
        QVERIFY(jsonDoc.isObject());

        const QJsonObject root = jsonDoc.object();
        QCOMPARE(root.value(QStringLiteral("appVersion")).toString(), QStringLiteral("1.0"));
        QVERIFY(root.value(QStringLiteral("exportedAtUtc")).isString());
        QCOMPARE(root.value(QStringLiteral("formatVersion")).toInt(), 1);
        QVERIFY(root.value(QStringLiteral("sqliteUserVersion")).isDouble());
        QVERIFY(root.value(QStringLiteral("parts")).isArray());

        const QJsonArray parts = root.value(QStringLiteral("parts")).toArray();
        QCOMPARE(parts.size(), 1);
        const QJsonObject p0 = parts.at(0).toObject();
        QCOMPARE(p0.value(QStringLiteral("name")).toString(), QStringLiteral("R1 10k"));
        QCOMPARE(p0.value(QStringLiteral("quantity")).toInt(), 123);
        QCOMPARE(p0.value(QStringLiteral("category")).toString(), QStringLiteral("Passive/Resistors"));
        QCOMPARE(p0.value(QStringLiteral("location")).toString(), QStringLiteral("Shelf A/Drawer 1"));
        QVERIFY(p0.value(QStringLiteral("files")).isArray());
        QCOMPARE(p0.value(QStringLiteral("files")).toArray().size(), 1);
        QCOMPARE(p0.value(QStringLiteral("files")).toArray().at(0).toString(), QStringLiteral("datasheets/r1.pdf"));

        QVERIFY(db.resetDatabase());
        QVERIFY(db.addPart(QStringLiteral("temp"), 1, -1, -1) > 0);

        QVERIFY2(db.importArchive(archivePath, &errorMessage), qPrintable(errorMessage));

        QSqlQuery verifyPart(db.database());
        QVERIFY(verifyPart.exec(
            QStringLiteral("SELECT p.name, p.quantity, c.name, pc.name, s.name, ps.name "
                           "FROM parts p "
                           "LEFT JOIN categories c ON c.id = p.category_id "
                           "LEFT JOIN categories pc ON pc.id = c.parent_id "
                           "LEFT JOIN storage_locations s ON s.id = p.storage_location_id "
                           "LEFT JOIN storage_locations ps ON ps.id = s.parent_id")
        ));
        QVERIFY(verifyPart.next());
        QCOMPARE(verifyPart.value(0).toString(), QStringLiteral("R1 10k"));
        QCOMPARE(verifyPart.value(1).toInt(), 123);
        QCOMPARE(verifyPart.value(2).toString(), QStringLiteral("Resistors"));
        QCOMPARE(verifyPart.value(3).toString(), QStringLiteral("Passive"));
        QCOMPARE(verifyPart.value(4).toString(), QStringLiteral("Drawer 1"));
        QCOMPARE(verifyPart.value(5).toString(), QStringLiteral("Shelf A"));
        QVERIFY(!verifyPart.next());

        QSqlQuery verifyLinks(db.database());
        QVERIFY(verifyLinks.exec(
            QStringLiteral("SELECT f.path FROM files f JOIN part_files pf ON pf.file_id = f.id ORDER BY f.path")
        ));
        QVERIFY(verifyLinks.next());
        QCOMPARE(verifyLinks.value(0).toString(), QStringLiteral("datasheets/r1.pdf"));
        QVERIFY(!verifyLinks.next());

        // Physical datasheet file must have been restored to the new DB directory
        QVERIFY2(QFileInfo::exists(db.absolutePath(QStringLiteral("datasheets/r1.pdf"))),
                 "Datasheet file was not restored by importArchive");
    }

    // exportArchive fails gracefully when the archive path is empty
    void exportArchiveEmptyPath()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        DatabaseManager db(QDir(tempDir.path()).filePath(QStringLiteral("parts.db")));
        QVERIFY(db.openDatabase(true));

        QString err;
        QVERIFY(!db.exportArchive(QString(), &err));
        QVERIFY(!err.isEmpty());
    }

    // importArchive fails gracefully when the archive file does not exist
    void importArchiveNotFound()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        DatabaseManager db(QDir(tempDir.path()).filePath(QStringLiteral("parts.db")));
        QVERIFY(db.openDatabase(true));

        QString err;
        QVERIFY(!db.importArchive(
            QDir(tempDir.path()).filePath(QStringLiteral("noexist.zip")), &err));
        QVERIFY(!err.isEmpty());
    }
};

QTEST_MAIN(TstDatabase)
#include "test_database.moc"
