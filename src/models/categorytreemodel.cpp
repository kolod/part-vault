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

#include "categorytreemodel.h"


#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QPalette>
#include <QApplication>
#include <QMimeData>
#include <QDataStream>
#include <QIODevice>
#include <QTimer>
#include <functional>

CategoryTreeModel::CategoryTreeModel(const QString& connectionName, QObject* parent)
    : ReloadableTreeModel(parent), mConnectionName(connectionName)
{
    mRoot = new CategoryNode{-1, QString{}};
    buildTree();
}

CategoryTreeModel::~CategoryTreeModel() {
    delete mRoot;
}

void CategoryTreeModel::reload() {
    beginResetModel();
    delete mRoot;
    mRoot = new CategoryNode{-1, QString{}};
    buildTree();
    endResetModel();
}

void CategoryTreeModel::buildTree() {
    // Load every category row into a flat map keyed by id.
    QHash<int, CategoryNode*> nodeMap;

    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QSqlQuery query(db);
    query.prepare("SELECT id, name, parent_id FROM categories ORDER BY name");
    if (!query.exec()) {
        qWarning() << "CategoryTreeModel: query failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        auto* node    = new CategoryNode;
        node->id      = query.value(0).toInt();
        node->name    = query.value(1).toString();
        node->parent  = nullptr;
        nodeMap.insert(node->id, node);
    }

    // Wire up parent/child relationships; orphans go under the invisible root.
    query.prepare("SELECT id, parent_id FROM categories");
    if (!query.exec()) return;

    while (query.next()) {
        const int id       = query.value(0).toInt();
        const int parentId = query.value(1).isNull() ? -1 : query.value(1).toInt();

        CategoryNode* node = nodeMap.value(id, nullptr);
        if (!node) continue;

        CategoryNode* parentNode = (parentId != -1) ? nodeMap.value(parentId, nullptr) : nullptr;
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

// ── helpers ──────────────────────────────────────────────────────────────────

void CategoryTreeModel::markActiveNodes() {
    // Collect the set of category IDs that have at least one part directly.
    // When mLocationFilter is set, restrict to parts in that location subtree.
    QSet<int> directlyUsed;
    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QSqlQuery q(db);

    if (mLocationFilter > 0) {
        q.prepare(
            "WITH RECURSIVE loc_desc(id) AS ("
            "  SELECT :loc"
            "  UNION ALL"
            "  SELECT s.id FROM storage_locations s JOIN loc_desc d ON s.parent_id = d.id"
            ")"
            "SELECT DISTINCT p.category_id FROM parts p"
            " INNER JOIN loc_desc d ON p.storage_location_id = d.id"
            " WHERE p.category_id IS NOT NULL");
        q.bindValue(":loc", mLocationFilter);
    } else {
        q.prepare("SELECT DISTINCT category_id FROM parts WHERE category_id IS NOT NULL");
    }

    if (q.exec()) {
        while (q.next())
            directlyUsed.insert(q.value(0).toInt());
    }

    // For each directly-used category, walk up the ancestor chain and mark all
    // nodes on the path as active (so parents of used categories are also highlighted).
    // Build a flat id→node map by traversing the tree we just built.
    QHash<int, CategoryNode*> nodeMap;
    QList<CategoryNode*> stack = mRoot->children;
    while (!stack.isEmpty()) {
        CategoryNode* n = stack.takeFirst();
        nodeMap.insert(n->id, n);
        stack.append(n->children);
    }

    for (int id : std::as_const(directlyUsed)) {
        CategoryNode* node = nodeMap.value(id, nullptr);
        while (node && node != mRoot) {
            if (node->active) break;   // already marked — ancestors are marked too
            node->active = true;
            node = node->parent;
        }
    }
}

CategoryNode* CategoryTreeModel::nodeFromIndex(const QModelIndex& index) const {
    if (!index.isValid())
        return mRoot;
    return static_cast<CategoryNode*>(index.internalPointer());
}

int CategoryTreeModel::categoryId(const QModelIndex& index) const {
    if (!index.isValid()) return -1;
    return nodeFromIndex(index)->id;
}

void CategoryTreeModel::setLocationFilter(int locationId)
{
    if (mLocationFilter == locationId) return;
    mLocationFilter = locationId;
    refreshActiveNodes();
}

void CategoryTreeModel::refreshActiveNodes()
{
    // Reset all active flags
    QList<CategoryNode*> stack = mRoot->children;
    while (!stack.isEmpty()) {
        CategoryNode* n = stack.takeFirst();
        n->active = false;
        stack.append(n->children);
    }

    markActiveNodes();

    // Notify the view — emit dataChanged for ForegroundRole across the whole tree
    std::function<void(const QModelIndex&)> notify = [&](const QModelIndex& parent) {
        const int n = rowCount(parent);
        if (n == 0) return;
        emit dataChanged(index(0, 0, parent), index(n - 1, 0, parent), {Qt::ForegroundRole});
        for (int r = 0; r < n; ++r)
            notify(index(r, 0, parent));
    };
    notify(QModelIndex{});
}

QModelIndex CategoryTreeModel::indexForId(int id) const {
    // BFS over the whole tree
    QList<QPair<CategoryNode*, QModelIndex>> queue;
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

QModelIndex CategoryTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent))
        return {};

    CategoryNode* parentNode = nodeFromIndex(parent);
    if (row < 0 || row >= parentNode->children.size())
        return {};

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex CategoryTreeModel::parent(const QModelIndex& child) const {
    if (!child.isValid())
        return {};

    CategoryNode* node       = nodeFromIndex(child);
    CategoryNode* parentNode = node->parent;

    if (!parentNode || parentNode == mRoot)
        return {};

    CategoryNode* grandParent = parentNode->parent ? parentNode->parent : mRoot;
    const int row = grandParent->children.indexOf(parentNode);
    if (row < 0) return {};

    return createIndex(row, 0, parentNode);
}

int CategoryTreeModel::rowCount(const QModelIndex& parent) const {
    return nodeFromIndex(parent)->children.size();
}

int CategoryTreeModel::columnCount(const QModelIndex&) const {
    return 1;
}

QVariant CategoryTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};

    CategoryNode* node = nodeFromIndex(index);

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

QVariant CategoryTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
        return tr("Category");
    return {};
}

Qt::ItemFlags CategoryTreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;   // allow drop onto empty viewport

    CategoryNode* node = nodeFromIndex(index);
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
    if (node->id > 0)                   // "All" (id=0) is not draggable
        f |= Qt::ItemIsDragEnabled;
    return f;
}

// ── Drag & drop ──────────────────────────────────────────────────────────────

static const QString kMimeType = QStringLiteral("application/x-partvault-category-id");

Qt::DropActions CategoryTreeModel::supportedDropActions() const {
    return Qt::MoveAction;
}

QStringList CategoryTreeModel::mimeTypes() const {
    return {kMimeType};
}

QMimeData* CategoryTreeModel::mimeData(const QModelIndexList& indexes) const {
    if (indexes.isEmpty()) return nullptr;
    const int id = categoryId(indexes.first());
    if (id <= 0) return nullptr;
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds << id;
    auto* mime = new QMimeData;
    mime->setData(kMimeType, ba);
    return mime;
}

bool CategoryTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
                                         int, int, const QModelIndex& parent) const {
    if (action != Qt::MoveAction) return false;
    if (!data->hasFormat(kMimeType)) return false;

    QByteArray ba = data->data(kMimeType);
    QDataStream ds(&ba, QIODevice::ReadOnly);
    int draggedId; ds >> draggedId;
    if (draggedId <= 0) return false;

    // Can't drop onto itself or onto one of its descendants
    CategoryNode* target = parent.isValid() ? nodeFromIndex(parent) : nullptr;
    while (target && target != mRoot) {
        if (target->id == draggedId) return false;
        target = target->parent;
    }
    return true;
}

bool CategoryTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                      int row, int column, const QModelIndex& parent) {
    if (!canDropMimeData(data, action, row, column, parent)) return false;

    QByteArray ba = data->data(kMimeType);
    QDataStream ds(&ba, QIODevice::ReadOnly);
    int draggedId; ds >> draggedId;

    const int newParentId = parent.isValid() ? categoryId(parent) : 0;

    // Check it's actually a change
    CategoryNode* dragged = nullptr;
    QList<CategoryNode*> stack = mRoot->children;
    while (!stack.isEmpty()) {
        CategoryNode* n = stack.takeFirst();
        if (n->id == draggedId) { dragged = n; break; }
        stack.append(n->children);
    }
    if (!dragged) return false;
    const int currentParentId = dragged->parent ? dragged->parent->id : 0;
    if (currentParentId == newParentId) return false;

    // Delegate to controller for confirmation; return false so Qt does nothing
    emit reparentRequested(draggedId, newParentId);
    return false;
}

bool CategoryTreeModel::reparentCategory(int categoryId, int newParentId) {
    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QSqlQuery q(db);
    q.prepare("UPDATE categories SET parent_id = ? WHERE id = ?");
    q.addBindValue(newParentId);
    q.addBindValue(categoryId);
    if (!q.exec()) {
        qWarning() << "CategoryTreeModel: reparent failed:" << q.lastError().text();
        return false;
    }
    return true;
}
