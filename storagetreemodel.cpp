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

#include "storagetreemodel.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QPalette>
#include <QApplication>
#include <QTreeView>

StorageTreeModel::StorageTreeModel(const QString& connectionName, QObject* parent)
    : QAbstractItemModel(parent), mConnectionName(connectionName)
{
    mRoot = new StorageNode{-1, QString{}};
    buildTree();
}

StorageTreeModel::~StorageTreeModel()
{
    delete mRoot;
}

void StorageTreeModel::reload()
{
    beginResetModel();
    delete mRoot;
    mRoot = new StorageNode{-1, QString{}};
    buildTree();
    endResetModel();
}

void StorageTreeModel::reload(QTreeView* view)
{
    QSet<int> expandedIds;
    QList<QModelIndex> stack;
    for (int r = 0; r < rowCount(); ++r)
        stack.append(index(r, 0));
    while (!stack.isEmpty()) {
        const QModelIndex idx = stack.takeLast();
        if (view->isExpanded(idx)) {
            expandedIds.insert(locationId(idx));
            for (int r = 0; r < rowCount(idx); ++r)
                stack.append(index(r, 0, idx));
        }
    }

    reload();

    for (int id : std::as_const(expandedIds)) {
        const QModelIndex idx = indexForId(id);
        if (idx.isValid())
            view->expand(idx);
    }
}

void StorageTreeModel::buildTree()
{
    QHash<int, StorageNode*> nodeMap;

    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QSqlQuery query(db);
    query.prepare("SELECT id, name, parent_id FROM storage_locations ORDER BY name");
    if (!query.exec()) {
        qWarning() << "StorageTreeModel: query failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        auto* node   = new StorageNode;
        node->id     = query.value(0).toInt();
        node->name   = query.value(1).toString();
        node->parent = nullptr;
        nodeMap.insert(node->id, node);
    }

    query.prepare("SELECT id, parent_id FROM storage_locations");
    if (!query.exec()) return;

    while (query.next()) {
        const int id       = query.value(0).toInt();
        const int parentId = query.value(1).isNull() ? -1 : query.value(1).toInt();

        StorageNode* node = nodeMap.value(id, nullptr);
        if (!node) continue;

        StorageNode* parentNode = (parentId != -1) ? nodeMap.value(parentId, nullptr) : nullptr;
        if (parentNode) {
            node->parent = parentNode;
            parentNode->children.append(node);
        } else {
            node->parent = mRoot;
            mRoot->children.append(node);
        }
    }

    markActiveNodes();
}

void StorageTreeModel::markActiveNodes()
{
    QSet<int> directlyUsed;
    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QSqlQuery q(db);
    q.prepare("SELECT DISTINCT storage_location_id FROM parts WHERE storage_location_id IS NOT NULL");
    if (q.exec()) {
        while (q.next())
            directlyUsed.insert(q.value(0).toInt());
    }

    QHash<int, StorageNode*> nodeMap;
    QList<StorageNode*> stack = mRoot->children;
    while (!stack.isEmpty()) {
        StorageNode* n = stack.takeFirst();
        nodeMap.insert(n->id, n);
        stack.append(n->children);
    }

    for (int id : std::as_const(directlyUsed)) {
        StorageNode* node = nodeMap.value(id, nullptr);
        while (node && node != mRoot) {
            if (node->active) break;
            node->active = true;
            node = node->parent;
        }
    }
}

StorageNode* StorageTreeModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return mRoot;
    return static_cast<StorageNode*>(index.internalPointer());
}

int StorageTreeModel::locationId(const QModelIndex& index) const
{
    if (!index.isValid()) return -1;
    return nodeFromIndex(index)->id;
}

QModelIndex StorageTreeModel::indexForId(int id) const
{
    QList<QPair<StorageNode*, QModelIndex>> queue;
    for (int r = 0; r < mRoot->children.size(); ++r)
        queue.append({mRoot->children.at(r), index(r, 0)});

    while (!queue.isEmpty()) {
        auto [node, idx] = queue.takeFirst();
        if (node->id == id)
            return idx;
        for (int r = 0; r < node->children.size(); ++r)
            queue.append({node->children.at(r), index(r, 0, idx)});
    }
    return {};
}

// ── QAbstractItemModel ───────────────────────────────────────────────────────

QModelIndex StorageTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    StorageNode* parentNode = nodeFromIndex(parent);
    if (row < 0 || row >= parentNode->children.size())
        return {};

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex StorageTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return {};

    StorageNode* node       = nodeFromIndex(child);
    StorageNode* parentNode = node->parent;

    if (!parentNode || parentNode == mRoot)
        return {};

    StorageNode* grandParent = parentNode->parent ? parentNode->parent : mRoot;
    const int row = grandParent->children.indexOf(parentNode);
    if (row < 0) return {};

    return createIndex(row, 0, parentNode);
}

int StorageTreeModel::rowCount(const QModelIndex& parent) const
{
    return nodeFromIndex(parent)->children.size();
}

int StorageTreeModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant StorageTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    StorageNode* node = nodeFromIndex(index);

    switch (role) {
    case Qt::DisplayRole:
        return node->name;
    case Qt::ForegroundRole:
        if (!node->active)
            return QApplication::palette().color(QPalette::Disabled, QPalette::Text);
        return {};
    case IdRole:
        return node->id;
    default:
        return {};
    }
}

QVariant StorageTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
        return tr("Storage Location");
    return {};
}

Qt::ItemFlags StorageTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
