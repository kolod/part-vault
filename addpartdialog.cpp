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

#include "addpartdialog.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPushButton>

AddPartDialog::AddPartDialog(const QString& connectionName, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add Part"));
    setMinimumWidth(360);

    mNameEdit      = new QLineEdit(this);
    mQuantitySpin  = new QSpinBox(this);
    mCategoryCombo = new QComboBox(this);
    mLocationCombo = new QComboBox(this);
    mButtons       = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    mQuantitySpin->setRange(0, 999999);
    mQuantitySpin->setValue(1);

    auto* form = new QFormLayout;
    form->addRow(tr("Name:"),             mNameEdit);
    form->addRow(tr("Quantity:"),         mQuantitySpin);
    form->addRow(tr("Category:"),         mCategoryCombo);
    form->addRow(tr("Storage location:"), mLocationCombo);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(mButtons);

    populateCategories(connectionName);
    populateLocations(connectionName);
    validate();

    connect(mNameEdit, &QLineEdit::textChanged, this, &AddPartDialog::validate);
    connect(mButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString AddPartDialog::name() const
{
    return mNameEdit->text().trimmed();
}

int AddPartDialog::quantity() const
{
    return mQuantitySpin->value();
}

int AddPartDialog::categoryId() const
{
    return mCategoryCombo->currentData().toInt();
}

int AddPartDialog::locationId() const
{
    return mLocationCombo->currentData().toInt();
}

void AddPartDialog::populateCategories(const QString& connectionName)
{
    mCategoryCombo->addItem(tr("(none)"), -1);

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);
    // Exclude the virtual "All" root (id = 0); show every real category sorted by name.
    query.prepare("SELECT id, name FROM categories WHERE id > 0 ORDER BY name");
    if (!query.exec()) return;

    while (query.next())
        mCategoryCombo->addItem(query.value(1).toString(), query.value(0).toInt());
}

void AddPartDialog::populateLocations(const QString& connectionName)
{
    mLocationCombo->addItem(tr("(none)"), -1);

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);
    query.prepare("SELECT id, name FROM storage_locations ORDER BY name");
    if (!query.exec()) return;

    while (query.next())
        mLocationCombo->addItem(query.value(1).toString(), query.value(0).toInt());
}

void AddPartDialog::validate()
{
    mButtons->button(QDialogButtonBox::Ok)->setEnabled(!name().isEmpty());
}
