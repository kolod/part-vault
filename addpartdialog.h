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
class QSpinBox;
class QComboBox;
class QDialogButtonBox;

// Dialog for adding a new part.
// Presents fields for name, quantity, category, and storage location.
// On accept, retrieve values via name(), quantity(), categoryId(), locationId().
// categoryId() / locationId() return -1 when no selection is made.
class AddPartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPartDialog(const QString& connectionName, QWidget* parent = nullptr);

    QString name()       const;
    int     quantity()   const;
    int     categoryId() const;   // -1 → uncategorised
    int     locationId() const;   // -1 → no location

private:
    QLineEdit*        m_nameEdit;
    QSpinBox*         m_quantitySpin;
    QComboBox*        m_categoryCombo;
    QComboBox*        m_locationCombo;
    QDialogButtonBox* m_buttons;

    void populateCategories(const QString& connectionName);
    void populateLocations(const QString& connectionName);
    void validate();
};
