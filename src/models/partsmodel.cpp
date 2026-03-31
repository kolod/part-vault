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
#include <QDebug>

PartsModel::PartsModel(const QString& connectionName, QObject* parent)
    : QAbstractTableModel(parent), mConnectionName(connectionName)
{
    fetchParts();
}

void PartsModel::setCategory(int categoryId)
{
    if (mCategoryFilter == categoryId) return;
    qDebug() << "PartsModel: category filter changed" << mCategoryFilter << "->" << categoryId;
    mCategoryFilter = categoryId;
    reload();
}

void PartsModel::setStorageLocation(int locationId)
{
    if (mStorageFilter == locationId) return;
    qDebug() << "PartsModel: storage filter changed" << mStorageFilter << "->" << locationId;
    mStorageFilter = locationId;
    reload();
}

void PartsModel::reload()
{
    beginResetModel();
    fetchParts();
    endResetModel();
}

int PartsModel::partId(int row) const
{
    if (row < 0 || row >= mParts.size()) return -1;
    return mParts.at(row).id;
}

// ── data loading ─────────────────────────────────────────────────────────────

QList<int> PartsModel::categoryDescendants(int rootId) const
{
    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QList<int> result;
    QList<int> queue = {rootId};

    QSqlQuery query(db);
    while (!queue.isEmpty()) {
        const int current = queue.takeFirst();
        result.append(current);
        query.prepare("SELECT id FROM categories WHERE parent_id = :pid");
        query.bindValue(":pid", current);
        if (query.exec()) {
            while (query.next())
                queue.append(query.value(0).toInt());
        } else {
            qWarning() << "PartsModel: categoryDescendants query failed for id" << current
                       << ":" << query.lastError().text();
        }
    }
    qDebug() << "PartsModel: category" << rootId << "expands to" << result.size() << "ids:" << result;
    return result;
}

QList<int> PartsModel::storageDescendants(int rootId) const
{
    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    QList<int> result;
    QList<int> queue = {rootId};

    QSqlQuery query(db);
    while (!queue.isEmpty()) {
        const int current = queue.takeFirst();
        result.append(current);
        query.prepare("SELECT id FROM storage_locations WHERE parent_id = :pid");
        query.bindValue(":pid", current);
        if (query.exec()) {
            while (query.next())
                queue.append(query.value(0).toInt());
        } else {
            qWarning() << "PartsModel: storageDescendants query failed for id" << current
                       << ":" << query.lastError().text();
        }
    }
    qDebug() << "PartsModel: storage" << rootId << "expands to" << result.size() << "ids:" << result;
    return result;
}

void PartsModel::fetchParts()
{
    mParts.clear();

    QSqlDatabase db = QSqlDatabase::database(mConnectionName);
    if (!db.isOpen()) {
        qWarning() << "PartsModel: database not open (connection:" << mConnectionName << ")";
        return;
    }
    qDebug() << "PartsModel: fetching parts, category filter =" << mCategoryFilter;

    // Fetch all columns needed to populate PartRecord.
    // p.id, p.name, p.quantity — core part fields.
    // c.name  — resolved via LEFT JOIN so parts with no category return an empty string.
    // sl.name — resolved via LEFT JOIN so parts with no storage location return an empty string.
    // A WHERE clause is appended below when a category filter is active.
    QString sql =
        "SELECT p.id, p.name, p.quantity, COALESCE(c.name, ''), COALESCE(s.name, ''), p.last_change "
        "FROM parts AS p "
        "LEFT JOIN categories        AS c ON c.id = p.category_id "
        "LEFT JOIN storage_locations AS s ON s.id = p.storage_location_id";

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

    qDebug() << "PartsModel: SQL:" << sql;

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

    qDebug() << "PartsModel: loaded" << mParts.size() << "parts";
}

// ── QAbstractTableModel ───────────────────────────────────────────────────────

int PartsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return mParts.size();
}

int PartsModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant PartsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= mParts.size())
        return {};

    const PartRecord& p = mParts.at(index.row());

    if (role == IdRole)
        return p.id;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColName:       return p.name;
        case ColQuantity:   return p.quantity;
        case ColCategory:   return p.categoryName;
        case ColLocation:   return p.locationName;
        case ColLastChange: return p.lastChange;
        }
    }

    if (role == Qt::TextAlignmentRole && index.column() == ColQuantity)
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);

    if (role == Qt::ForegroundRole && p.quantity == 0)
        return QApplication::palette().color(QPalette::Disabled, QPalette::Text);

    return {};
}

QVariant PartsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColName:       return tr("Name");
    case ColQuantity:   return tr("Qty");
    case ColCategory:   return tr("Category");
    case ColLocation:   return tr("Location");
    case ColLastChange: return tr("Last Change");
    }
    return {};
}

Qt::ItemFlags PartsModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
