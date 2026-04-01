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

#include <QAbstractTableModel>
#include <QString>
#include <QList>

struct FileRecord
{
    int     id;
    QString path;
    QString type;
    QString description;
};

class FilesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColPath        = 0,
        ColType        = 1,
        ColDescription = 2,
        ColCount       = 3
    };

    enum Roles {
        IdRole = Qt::UserRole + 1
    };

    explicit FilesModel(const QString& connectionName, QObject* parent = nullptr);

    // Set the part whose files are displayed.
    // Pass 0 or -1 to clear the view (show nothing).
    void setPart(int partId);

    // Force a reload from the database (call after INSERT/UPDATE/DELETE on files or part_files).
    void reload();

    // Returns the file id for the given row, or -1 if out of range.
    int fileId(int row) const;

    // QAbstractTableModel interface
    int           rowCount   (const QModelIndex& parent = QModelIndex()) const override;
    int           columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant      data       (const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant      headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags      (const QModelIndex& index) const override;

private:
    void fetchFiles();

    QString          mConnectionName;
    int              mPartFilter = 0;   // 0 → no part selected, list is empty
    QList<FileRecord> mFiles;
};
