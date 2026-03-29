#include "categorytreemodel.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHash>

CategoryTreeModel::CategoryTreeModel(QSqlDatabase& db, QObject* parent)
    : QAbstractItemModel(parent), m_db(db)
{
    m_root = new CategoryNode{-1, QString{}};
    buildTree();
}

CategoryTreeModel::~CategoryTreeModel()
{
    delete m_root;
}

void CategoryTreeModel::reload()
{
    beginResetModel();
    delete m_root;
    m_root = new CategoryNode{-1, QString{}};
    buildTree();
    endResetModel();
}

void CategoryTreeModel::buildTree()
{
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
}

// ── helpers ──────────────────────────────────────────────────────────────────

CategoryNode* CategoryTreeModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return m_root;
    return static_cast<CategoryNode*>(index.internalPointer());
}

int CategoryTreeModel::categoryId(const QModelIndex& index) const
{
    if (!index.isValid()) return -1;
    return nodeFromIndex(index)->id;
}

// ── QAbstractItemModel ───────────────────────────────────────────────────────

QModelIndex CategoryTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    CategoryNode* parentNode = nodeFromIndex(parent);
    if (row < 0 || row >= parentNode->children.size())
        return {};

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex CategoryTreeModel::parent(const QModelIndex& child) const
{
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

int CategoryTreeModel::rowCount(const QModelIndex& parent) const
{
    return nodeFromIndex(parent)->children.size();
}

int CategoryTreeModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant CategoryTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    CategoryNode* node = nodeFromIndex(index);

    switch (role) {
    case Qt::DisplayRole:
        return node->name;
    case IdRole:
        return node->id;
    default:
        return {};
    }
}

QVariant CategoryTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
        return tr("Category");
    return {};
}

Qt::ItemFlags CategoryTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
