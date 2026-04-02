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

#include "partsmodel.h"


#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QPalette>
#include <QApplication>

PartsModel::PartsModel(const QString& connectionName, QObject* parent)
    : QAbstractTableModel(parent), mConnectionName(connectionName)
{
    fetchParts();
}

void PartsModel::setCategory(int categoryId) {
    if (mCategoryFilter == categoryId) return;
    mCategoryFilter = categoryId;
    reload();
}

void PartsModel::setStorageLocation(int locationId) {
    if (mStorageFilter == locationId) return;
    mStorageFilter = locationId;
    reload();
}

void PartsModel::reload() {
    beginResetModel();
    fetchParts();
    endResetModel();
}

int PartsModel::partId(int row) const {
    if (row < 0 || row >= mParts.size()) return -1;
    return mParts.at(row).id;
}

// ── data loading ─────────────────────────────────────────────────────────────

QList<int> PartsModel::categoryDescendants(int rootId) const {
    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QSqlQuery q(db);
    q.prepare(
        "WITH RECURSIVE desc(id) AS ("
        "  SELECT :root"
        "  UNION ALL"
        "  SELECT c.id FROM categories c JOIN desc d ON c.parent_id = d.id"
        ") SELECT id FROM desc");
    q.bindValue(":root", rootId);
    QList<int> result;
    if (q.exec()) {
        while (q.next())
            result.append(q.value(0).toInt());
    } else {
        qWarning() << "PartsModel: categoryDescendants failed:" << q.lastError().text();
    }
    return result;
}

QList<int> PartsModel::storageDescendants(int rootId) const {
    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QSqlQuery q(db);
    q.prepare(
        "WITH RECURSIVE desc(id) AS ("
        "  SELECT :root"
        "  UNION ALL"
        "  SELECT s.id FROM storage_locations s JOIN desc d ON s.parent_id = d.id"
        ") SELECT id FROM desc");
    q.bindValue(":root", rootId);
    QList<int> result;
    if (q.exec()) {
        while (q.next())
            result.append(q.value(0).toInt());
    } else {
        qWarning() << "PartsModel: storageDescendants failed:" << q.lastError().text();
    }
    return result;
}

void PartsModel::fetchParts() {
    mParts.clear();

    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    if (!db.isOpen()) {
        qWarning() << "PartsModel: database not open (connection:" << mConnectionName << ")";
        return;
    }

    // loc_path pre-computes the full ancestor path string (e.g. "Cabinet > Drawer") for
    // every storage location.  The anchor seeds from virtual-root children (parent_id = 0)
    // AND from orphaned locations whose parent was deleted (parent_id IS NULL, excluding
    // the virtual root itself at id = 0).  The recursive step appends child names with →.
    QString sql =
        "WITH RECURSIVE loc_path(id, path) AS ("
        "  SELECT id, name FROM storage_locations"
        "  WHERE parent_id = 0 OR (parent_id IS NULL AND id != 0)"
        "  UNION ALL"
        "  SELECT s.id, lp.path || ' \u2192 ' || s.name"
        "  FROM storage_locations AS s JOIN loc_path AS lp ON s.parent_id = lp.id"
        ") "
        "SELECT p.id, p.name, p.quantity, COALESCE(c.name, ''), COALESCE(lp.path, ''), p.last_change "
        "FROM parts AS p "
        "LEFT JOIN categories AS c  ON c.id  = p.category_id "
        "LEFT JOIN loc_path   AS lp ON lp.id = p.storage_location_id";

    QSqlQuery query(db);
    query.setForwardOnly(true);

    QStringList conditions;
    QList<int> categoryIds;
    QList<int> storageIds;

    if (mCategoryFilter > 0) {
        categoryIds = categoryDescendants(mCategoryFilter);
        QStringList placeholders(categoryIds.size(), "?");
        conditions += QString("p.category_id IN (%1)").arg(placeholders.join(", "));
    }

    if (mStorageFilter > 0) {
        storageIds = storageDescendants(mStorageFilter);
        QStringList placeholders(storageIds.size(), "?");
        conditions += QString("p.storage_location_id IN (%1)").arg(placeholders.join(", "));
    }

    if (!conditions.isEmpty())
        sql += " WHERE " + conditions.join(" AND ");
    sql += " ORDER BY p.name";

    if (!query.prepare(sql)) {
        qWarning() << "PartsModel: prepare failed:" << query.lastError().text();
        return;
    }
    for (const int id : categoryIds)
        query.addBindValue(id);
    for (const int id : storageIds)
        query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "PartsModel: exec failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        mParts.append({
            query.value(0).toInt(),
            query.value(1).toString(),
            query.value(2).toInt(),
            query.value(3).toString(),
            query.value(4).toString(),
            query.value(5).toString()
        });
    }
}

// ── QAbstractTableModel ───────────────────────────────────────────────────────

int PartsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return mParts.size();
}

int PartsModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant PartsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= mParts.size())
        return {};

    const PartRecord& p = mParts.at(index.row());

    // For the custom IdRole, return the part id regardless of the column.
    if (role == IdRole) return p.id;

    if (role == Qt::DisplayRole || role == Qt::EditRole) switch (index.column()) {
        case ColName:       return p.name;
        case ColQuantity:   return p.quantity;
        case ColCategory:   return p.categoryName;
        case ColLocation:   return p.locationName;
        case ColLastChange: return p.lastChange;
    }

    // Align the quantity column to the right for better readability.
    if (role == Qt::TextAlignmentRole && index.column() == ColQuantity)
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);

    // If the quantity is zero, show the text in a disabled color to indicate that the part is out of stock.
    if (role == Qt::ForegroundRole && p.quantity == 0)
        return QApplication::palette().color(QPalette::Disabled, QPalette::Text);

    return {};
}

QVariant PartsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) switch (section) {
        case ColName:       return tr("Name");
        case ColQuantity:   return tr("Qty");
        case ColCategory:   return tr("Category");
        case ColLocation:   return tr("Location");
        case ColLastChange: return tr("Last Change");
    }

    // For vertical headers, we could return row numbers, 
    // but it's not very useful and just adds clutter.
    return {};
}

Qt::ItemFlags PartsModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
