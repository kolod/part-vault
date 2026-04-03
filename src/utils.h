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

#include <QString>
#include <QList>

// Strips SQL line comments (-- ...) from every line of sql,
// respecting single-quoted string literals so that '--' inside
// a string value is preserved.  Returns the cleaned SQL text.
QString stripSqlComments(const QString& sql);

// Creates a ZIP archive at archivePath containing all files under sourceDirPath.
bool createZipArchive(const QString& archivePath, const QString& sourceDirPath, QString* errorMessage);

// Extracts the ZIP archive at archivePath into destinationDirPath.
bool extractZipArchive(const QString& archivePath, const QString& destinationDirPath, QString* errorMessage);

// Walks the parent_id chain upward from id in the given table and returns the
// full ancestor path as "Root → Child → …".  Returns an empty string if id ≤ 0.
// table must be either "categories" or "storage_locations".
