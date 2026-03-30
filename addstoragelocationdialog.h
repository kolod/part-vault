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

class QLineEdit;
class QDialogButtonBox;

// Dialog for adding a new storage location (e.g. "Drawer A1", "Shelf C2").
// On accept, call name() to retrieve the entered value.
class AddStorageLocationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddStorageLocationDialog(QWidget* parent = nullptr);

    QString name() const;

private:
    QLineEdit*        mNameEdit;
    QDialogButtonBox* mButtons;

    void validate();
};
