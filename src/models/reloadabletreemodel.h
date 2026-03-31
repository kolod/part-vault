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

#include <QAbstractItemModel>

// Abstract base for tree models that support a plain reload().
// TreeViewEx calls reload() through this interface so it can
// preserve the expanded state without knowing the concrete model type.
class ReloadableTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    using QAbstractItemModel::QAbstractItemModel;
    virtual void reload() = 0;
};
