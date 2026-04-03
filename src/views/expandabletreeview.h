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

/**
 * @file expandabletreeview.h
 * @brief Tree view helper that preserves expanded state across reload.
 */

class ReloadableTreeModel;

/**
 * @brief QTreeView extension with reload-and-restore-expanded behavior.
 */
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

    void collectExpanded(const QModelIndex& parent, QSet<int>& ids) const;
    void doRestoreExpanded(const QModelIndex& parent, const QSet<int>& ids);

    // Must match the IdRole defined in both tree models.
    static constexpr int IdRole = Qt::UserRole + 1;
};
