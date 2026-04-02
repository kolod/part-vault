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

#include "reloadabletreemodel.h"
#include <QString>
#include <QList>

// Internal tree node — one per category row.
struct CategoryNode
{
    int      id;
    QString  name;
    bool     active  = false;  // true if this node or any descendant has parts
    CategoryNode*        parent   = nullptr;
    QList<CategoryNode*> children;

    ~CategoryNode() { qDeleteAll(children); }
};

class CategoryTreeModel : public ReloadableTreeModel
{
    Q_OBJECT

public:
    // Custom roles
    enum Roles {
        IdRole = Qt::UserRole + 1
    };

    explicit CategoryTreeModel(const QString& connectionName, QObject* parent = nullptr);
    ~CategoryTreeModel() override;

    // Reloads the entire tree from the database.
    void reload() override;

    // Returns the category id for the given index, or -1 if invalid.
    int categoryId(const QModelIndex& index) const;

    // Returns the model index for a given category id, or invalid if not found.
    QModelIndex indexForId(int categoryId) const;

    // Restricts "active" highlighting to categories that have parts at
    // locationId (and its descendants). Pass 0 or -1 to clear the filter.
    void setLocationFilter(int locationId);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int  rowCount  (const QModelIndex& parent = QModelIndex()) const override;
    int  columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Drag & drop
    Qt::DropActions supportedDropActions() const override;
    QStringList     mimeTypes() const override;
    QMimeData*      mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action,
                         int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action,
                      int row, int column, const QModelIndex& parent) override;

    // Call this from the controller after the user confirms the reparent.
    // Returns true on success; caller is responsible for reloading the view.
    bool reparentCategory(int categoryId, int newParentId);

signals:
    // Emitted when a drag-drop reparent is requested; controller should confirm
    // and call reparentCategory() if accepted.
    void reparentRequested(int categoryId, int newParentId);

private:
    QString        mConnectionName;
    CategoryNode*  mRoot = nullptr;   // invisible root; its children are top-level categories
    int            mLocationFilter = 0;  // 0/−1 = no filter

    void buildTree();
    void markActiveNodes();
    void refreshActiveNodes();
    CategoryNode* nodeFromIndex(const QModelIndex& index) const;
};
