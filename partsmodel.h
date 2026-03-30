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

struct PartRecord
{
    int     id;
    QString name;
    int     quantity;
    QString categoryName;
    QString locationName;
    QString lastChange;
};

class PartsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColName       = 0,
        ColQuantity   = 1,
        ColCategory   = 2,
        ColLocation   = 3,
        ColLastChange = 4,
        ColCount      = 5
    };

    enum Roles {
        IdRole = Qt::UserRole + 1
    };

    explicit PartsModel(const QString& connectionName, QObject* parent = nullptr);

    // Replace the current category filter.
    // Pass -1 or 0 ("All" category) to show all parts.
    // Passing any other category id shows parts in that category AND all its descendants.
    void setCategory(int categoryId);

    // Replace the current storage location filter.
    // Pass -1 or 0 to show all parts regardless of location.
    // Passing any other id shows parts in that location AND all its descendants.
    void setStorageLocation(int locationId);

    // Force a reload from the database (call after INSERT/UPDATE/DELETE).
    void reload();

    // Returns the part id for the given row, or -1 if out of range.
    int partId(int row) const;

    // QAbstractTableModel interface
    int      rowCount   (const QModelIndex& parent = QModelIndex()) const override;
    int      columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data       (const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags (const QModelIndex& index) const override;

private:
    QString          mConnectionName;
    int              mCategoryFilter = 0;   // 0 → no filter
    int              mStorageFilter  = 0;   // 0 → no filter
    QList<PartRecord> mParts;

    void fetchParts();
    // Returns all descendant category ids for the given root id (inclusive).
    QList<int> categoryDescendants(int rootId) const;
    // Returns all descendant storage location ids for the given root id (inclusive).
    QList<int> storageDescendants(int rootId) const;
};
