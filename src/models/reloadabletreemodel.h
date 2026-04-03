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

/**
 * @file reloadabletreemodel.h
 * @brief Shared interface for tree models supporting full reload.
 */

/**
 * @brief Abstract base for tree models that can reload themselves.
 */
class ReloadableTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    using QAbstractItemModel::QAbstractItemModel;
    virtual void reload() = 0;
};
