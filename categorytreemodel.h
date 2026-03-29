#pragma once

#include <QAbstractItemModel>
#include <QSqlDatabase>
#include <QString>
#include <QList>

// Internal tree node — one per category row.
struct CategoryNode
{
    int      id;
    QString  name;
    CategoryNode*        parent   = nullptr;
    QList<CategoryNode*> children;

    ~CategoryNode() { qDeleteAll(children); }
};

class CategoryTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // Custom roles
    enum Roles {
        IdRole = Qt::UserRole + 1
    };

    explicit CategoryTreeModel(QSqlDatabase& db, QObject* parent = nullptr);
    ~CategoryTreeModel() override;

    // Reloads the entire tree from the database.
    void reload();

    // Returns the category id for the given index, or -1 if invalid.
    int categoryId(const QModelIndex& index) const;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int  rowCount  (const QModelIndex& parent = QModelIndex()) const override;
    int  columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    QSqlDatabase&  m_db;
    CategoryNode*  m_root = nullptr;   // invisible root; its children are top-level categories

    void buildTree();
    CategoryNode* nodeFromIndex(const QModelIndex& index) const;
};
