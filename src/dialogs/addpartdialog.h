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

#include <QDialog>
#include <QString>

/**
 * @file addpartdialog.h
 * @brief Dialog for creating a part with quantity and optional parent links.
 */

class DatabaseManager;
class QLineEdit;
class QSpinBox;
class QDialogButtonBox;

/**
 * @brief Modal dialog used to create one part row.
 */
class AddPartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPartDialog(const DatabaseManager& databaseManager, int categoryId, int locationId, QWidget* parent = nullptr);

    QString name()       const;
    int     quantity()   const;
    int     categoryId() const;   // -1 → uncategorised
    int     locationId() const;   // -1 → no location

private:
    QLineEdit*        mNameEdit;
    QSpinBox*         mQuantitySpin;
    QDialogButtonBox* mButtons;
    int               mCategoryId;
    int               mLocationId;

    void validate();
};
