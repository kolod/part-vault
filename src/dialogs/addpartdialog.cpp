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
#include "../utils.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>

AddPartDialog::AddPartDialog(const QString& connectionName, int categoryId, int locationId, QWidget* parent)
    : QDialog(parent), mCategoryId(categoryId), mLocationId(locationId)
{
    setWindowTitle(tr("Add Part"));
    setMinimumWidth(360);

    mNameEdit     = new QLineEdit(this);
    mQuantitySpin = new QSpinBox(this);
    mButtons      = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    mQuantitySpin->setRange(0, 999999);
    mQuantitySpin->setValue(1);

    const QString catPath = buildAncestorPath(connectionName, QStringLiteral("categories"),        categoryId);
    const QString locPath = buildAncestorPath(connectionName, QStringLiteral("storage_locations"), locationId);

    auto* catLabel = new QLabel(catPath.isEmpty() ? tr("(none)") : catPath, this);
    auto* locLabel = new QLabel(locPath.isEmpty() ? tr("(none)") : locPath, this);
    catLabel->setWordWrap(true);
    locLabel->setWordWrap(true);

    auto* form = new QFormLayout;
    form->addRow(tr("Name:"),             mNameEdit);
    form->addRow(tr("Quantity:"),         mQuantitySpin);
    form->addRow(tr("Category:"),         catLabel);
    form->addRow(tr("Storage location:"), locLabel);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(mButtons);

    mButtons->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(mNameEdit, &QLineEdit::textChanged, this, &AddPartDialog::validate);
    connect(mButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString AddPartDialog::name() const {
    return mNameEdit->text().trimmed();
}

int AddPartDialog::quantity() const {
    return mQuantitySpin->value();
}

int AddPartDialog::categoryId() const {
    return mCategoryId;
}

int AddPartDialog::locationId() const {
    return mLocationId;
}

void AddPartDialog::validate() {
    mButtons->button(QDialogButtonBox::Ok)->setEnabled(!name().isEmpty());
}
