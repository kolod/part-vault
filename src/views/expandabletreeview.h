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

#include <QTreeView>
#include <QSet>

class ReloadableTreeModel;

// QTreeView subclass that reloads its ReloadableTreeModel while preserving
// which nodes were expanded.  Both the category tree and the storage-location
// tree use this so the expand/collapse logic lives in exactly one place.
class TreeViewEx : public QTreeView
{
    Q_OBJECT
public:
    explicit TreeViewEx(QWidget* parent = nullptr);

    // Saves expanded node IDs (via IdRole), calls model->reload(), then
    // re-expands the same nodes.  Does nothing if the model is not a
    // ReloadableTreeModel.
    void reloadPreservingExpanded();

private:
    QSet<int> snapshotExpanded() const;
    void restoreExpanded(const QSet<int>& ids);

    // Must match the IdRole defined in both tree models.
    static constexpr int IdRole = Qt::UserRole + 1;
};
