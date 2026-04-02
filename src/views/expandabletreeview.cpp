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

#include "views/expandabletreeview.h"
#include "models/reloadabletreemodel.h"

TreeViewEx::TreeViewEx(QWidget* parent)
    : QTreeView(parent)
{
}

void TreeViewEx::reloadPreservingExpanded()
{
    auto* m = qobject_cast<ReloadableTreeModel*>(model());
    if (!m) return;

    const QSet<int> ids = snapshotExpanded();
    m->reload();
    restoreExpanded(ids);
}

QSet<int> TreeViewEx::snapshotExpanded() const
{
    QSet<int> ids;
    collectExpanded(QModelIndex{}, ids);
    return ids;
}

void TreeViewEx::collectExpanded(const QModelIndex& parent, QSet<int>& ids) const
{
    for (int r = 0; r < model()->rowCount(parent); ++r) {
        const QModelIndex idx = model()->index(r, 0, parent);
        if (isExpanded(idx))
            ids.insert(model()->data(idx, IdRole).toInt());
        collectExpanded(idx, ids);
    }
}

void TreeViewEx::restoreExpanded(const QSet<int>& ids)
{
    doRestoreExpanded(QModelIndex{}, ids);
}

void TreeViewEx::doRestoreExpanded(const QModelIndex& parent, const QSet<int>& ids)
{
    for (int r = 0; r < model()->rowCount(parent); ++r) {
        const QModelIndex idx = model()->index(r, 0, parent);
        if (ids.contains(model()->data(idx, IdRole).toInt()))
            expand(idx);
        doRestoreExpanded(idx, ids);
    }
}
