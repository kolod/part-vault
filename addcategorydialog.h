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
class QLabel;
class QDialogButtonBox;

// Dialog for adding a new category as a child of a pre-determined parent.
// The parent is passed at construction time (taken from the current tree selection).
// The full ancestor path is shown as read-only text.
// parentId() == 0 means top-level (child of the virtual "All" root).
class AddCategoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCategoryDialog(const QString& connectionName, int parentId, QWidget* parent = nullptr);

    QString name()     const;
    int     parentId() const;   // the id passed at construction

private:
    int                m_parentId;
    QLineEdit*         m_nameEdit;
    QDialogButtonBox*  m_buttons;

    QString buildPath(const QString& connectionName) const;
    void    validate();
};
