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
#include <QList>

class QLineEdit;
class QComboBox;
class QPushButton;
class QDialogButtonBox;
class QVBoxLayout;

// Dialog for adding a new category.
// The parent is selected via a cascading drill-down:
//  - The first combo shows top-level categories (children of id=0).
//  - Selecting a category that has children adds a new combo below.
//  - "Back" removes the last combo level.
//  - "New subcategory" creates an intermediate category (child of the
//    currently selected item) and descends into it automatically.
// OK is enabled when name is non-empty and a category (id > 0) is selected
// in the last combo.  parentId() always returns a value > 0 on accept.
class AddCategoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCategoryDialog(const QString& connectionName, QWidget* parent = nullptr);

    QString name()     const;
    int     parentId() const;   // > 0 when accepted

    // Pre-selects the cascade path to categoryId before exec() is called.
    // Does nothing if categoryId <= 0 or not found in the DB.
    void setCategory(int categoryId);

private:
    QString            m_connectionName;
    QLineEdit*         m_nameEdit;
    QVBoxLayout*       m_cascadeLayout;
    QList<QComboBox*>  m_combos;
    QPushButton*       m_backButton;
    QDialogButtonBox*  m_buttons;

    void addCascadeLevel(int parentId);
    void removeCascadeLevel();
    bool hasChildren(int categoryId) const;
    void loadChildren(int parentId, QComboBox* combo);
    void onComboChanged(QComboBox* combo);
    void validate();
};
