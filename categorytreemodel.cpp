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

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QPalette>
#include <QApplication>

CategoryTreeModel::CategoryTreeModel(QSqlDatabase& db, QObject* parent)
    : QAbstractItemModel(parent), m_db(db)
{
    m_root = new CategoryNode{-1, QString{}};
    buildTree();
}

CategoryTreeModel::~CategoryTreeModel() {
    delete m_root;
}

void CategoryTreeModel::reload() {
    beginResetModel();
    delete m_root;
    m_root = new CategoryNode{-1, QString{}};
    buildTree();
    endResetModel();
}

void CategoryTreeModel::buildTree() {
    // Load every category row into a flat map keyed by id.
    QHash<int, CategoryNode*> nodeMap;

    QSqlQuery query(m_db);
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
            node->parent = m_root;
            m_root->children.append(node);
        }
    }

    markActiveNodes();
}

// ── helpers ──────────────────────────────────────────────────────────────────

void CategoryTreeModel::markActiveNodes() {
    // Collect the set of category IDs that have at least one part directly.
    QSet<int> directlyUsed;
    QSqlQuery q(m_db);
    q.prepare("SELECT DISTINCT category_id FROM parts WHERE category_id IS NOT NULL");
    if (q.exec()) {
        while (q.next())
            directlyUsed.insert(q.value(0).toInt());
    }

    // For each directly-used category, walk up the ancestor chain and mark all
    // nodes on the path as active (so parents of used categories are also highlighted).
    // Build a flat id→node map by traversing the tree we just built.
    QHash<int, CategoryNode*> nodeMap;
    QList<CategoryNode*> stack = m_root->children;
    while (!stack.isEmpty()) {
        CategoryNode* n = stack.takeFirst();
        nodeMap.insert(n->id, n);
        stack.append(n->children);
    }

    for (int id : std::as_const(directlyUsed)) {
        CategoryNode* node = nodeMap.value(id, nullptr);
        while (node && node != m_root) {
            if (node->active) break;   // already marked — ancestors are marked too
            node->active = true;
            node = node->parent;
        }
    }
}

CategoryNode* CategoryTreeModel::nodeFromIndex(const QModelIndex& index) const {
    if (!index.isValid())
        return m_root;
    return static_cast<CategoryNode*>(index.internalPointer());
}

int CategoryTreeModel::categoryId(const QModelIndex& index) const {
    if (!index.isValid()) return -1;
    return nodeFromIndex(index)->id;
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

    if (!parentNode || parentNode == m_root)
        return {};

    CategoryNode* grandParent = parentNode->parent ? parentNode->parent : m_root;
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
    if (!index.isValid()) return Qt::NoItemFlags;
    CategoryNode* node = nodeFromIndex(index);
    if (!node->active)
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
