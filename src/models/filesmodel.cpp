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

/**
 * @file filesmodel.cpp
 * @brief FilesModel implementation.
 */

#include "filesmodel.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

FilesModel::FilesModel(const QString& connectionName, QObject* parent)
    : QAbstractTableModel(parent), mConnectionName(connectionName)
{
}

void FilesModel::setPart(int partId)
{
    if (mPartFilter == partId) return;
    qDebug() << "FilesModel: part filter changed" << mPartFilter << "->" << partId;
    mPartFilter = partId;
    reload();
}

void FilesModel::reload()
{
    beginResetModel();
    fetchFiles();
    endResetModel();
}

int FilesModel::fileId(int row) const
{
    if (row < 0 || row >= mFiles.size()) return -1;
    return mFiles.at(row).id;
}

// ── data loading ─────────────────────────────────────────────────────────────

void FilesModel::fetchFiles()
{
    mFiles.clear();

    if (mPartFilter <= 0) return;

    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    if (!db.isOpen()) {
        qWarning() << "FilesModel: database not open (connection:" << mConnectionName << ")";
        return;
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);

    const QString sql =
        "SELECT f.id, f.path, f.type, COALESCE(f.description, '') "
        "FROM files AS f "
        "INNER JOIN part_files AS pf ON pf.file_id = f.id "
        "WHERE pf.part_id = :partId "
        "ORDER BY f.type, f.path";

    if (!query.prepare(sql)) {
        qWarning() << "FilesModel: prepare failed:" << query.lastError().text();
        return;
    }
    query.bindValue(":partId", mPartFilter);

    if (!query.exec()) {
        qWarning() << "FilesModel: exec failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        mFiles.append({
            query.value(0).toInt(),
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toString()
        });
    }

    qDebug() << "FilesModel: loaded" << mFiles.size() << "files for part" << mPartFilter;
}

// ── QAbstractTableModel ───────────────────────────────────────────────────────

int FilesModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return mFiles.size();
}

int FilesModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant FilesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= mFiles.size())
        return {};

    const FileRecord& f = mFiles.at(index.row());

    if (role == IdRole) return f.id;

    if (role == Qt::DisplayRole || role == Qt::EditRole) switch (index.column()) {
        case ColPath:        return f.path;
        case ColType:        return f.type;
        case ColDescription: return f.description;
    }

    return {};
}

QVariant FilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) switch (section) {
        case ColPath:        return tr("Path");
        case ColType:        return tr("Type");
        case ColDescription: return tr("Description");
    }
    return {};
}

Qt::ItemFlags FilesModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
