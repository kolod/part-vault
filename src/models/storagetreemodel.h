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

#include <QAbstractItemModel>
#include <QString>
#include <QList>
#include <QSet>

class QTreeView;

// Internal tree node — one per storage_locations row.
struct StorageNode
{
    int      id;
    QString  name;
    bool     active  = false;  // true if this node or any descendant has parts
    StorageNode*        parent   = nullptr;
    QList<StorageNode*> children;

    ~StorageNode() { qDeleteAll(children); }
};

class StorageTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1
    };

    explicit StorageTreeModel(const QString& connectionName, QObject* parent = nullptr);
    ~StorageTreeModel() override;

    void reload();
    void reload(QTreeView* view);

    // Returns the location id for the given index, or -1 if invalid.
    int locationId(const QModelIndex& index) const;

    // Returns the model index for a given location id, or invalid if not found.
    QModelIndex indexForId(int locationId) const;

    // Restricts "active" highlighting to locations that have parts in
    // categoryId (and its descendants). Pass 0 or -1 to clear the filter.
    void setCategoryFilter(int categoryId);

    // QAbstractItemModel interface
    QModelIndex   index      (int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex   parent     (const QModelIndex& child) const override;
    int           rowCount   (const QModelIndex& parent = QModelIndex()) const override;
    int           columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant      data       (const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant      headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags      (const QModelIndex& index) const override;

private:
    QString       mConnectionName;
    StorageNode*  mRoot = nullptr;
    int           mCategoryFilter = 0;  // 0/-1 = no filter

    void buildTree();
    void markActiveNodes();
    void refreshActiveNodes();
    StorageNode* nodeFromIndex(const QModelIndex& index) const;
};
