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

#include <stdint.h>

#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

#include <QFile>
#include <QStringList>
#include "utils.h"

QString stripSqlComments(const QString& sql) {
    const QStringList lines = sql.split(QLatin1Char('\n'));
    QString result;
    for (const QString& line : lines) {
        QString stripped;
        bool inString = false;
        for (int i = 0; i < line.size(); ++i) {
            const QChar c = line[i];
            if (inString) {
                stripped += c;
                // A pair of '' inside a string is an escaped quote, not end-of-string
                if (c == QLatin1Char('\'')) {
                    if (i + 1 < line.size() && line[i + 1] == QLatin1Char('\''))
                        stripped += line[++i];   // consume the second ' and stay in string
                    else
                        inString = false;
                }
            } else {
                if (c == QLatin1Char('\'')) {
                    inString = true;
                    stripped += c;
                } else if (c == QLatin1Char('-') && i + 1 < line.size() && line[i + 1] == QLatin1Char('-')) {
                    break;   // rest of line is a comment
                } else {
                    stripped += c;
                }
            }
        }
        stripped = stripped.trimmed();
        if (!stripped.isEmpty())
            result += stripped + "\n";
    }
    return result;
}

bool createZipArchive(const QString& archivePath, const QString& sourceDirPath, QString* errorMessage) {
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

bool extractZipArchive(const QString& archivePath, const QString& destinationDirPath, QString* errorMessage) {
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
