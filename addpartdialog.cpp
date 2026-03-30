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
#include <QLabel>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPushButton>
#include <QDebug>

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

    const QString catPath = buildPath(connectionName, "categories", categoryId);
    const QString locPath = buildPath(connectionName, "storage_locations", locationId);

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

QString AddPartDialog::buildPath(const QString& connectionName, const QString& table, int id) const {
    if (id <= 0) return {};

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery q(db);
    QStringList parts;
    int current = id;
    while (current > 0) {
        q.prepare(QString("SELECT name, parent_id FROM %1 WHERE id = ?").arg(table));
        q.addBindValue(current);
        if (!q.exec() || !q.next()) {
            qWarning() << "AddPartDialog: row" << current << "not found in" << table;
            break;
        }
        parts.prepend(q.value(0).toString());
        current = q.value(1).isNull() ? 0 : q.value(1).toInt();
    }
    return parts.join(QString(" \u2192 "));
}

void AddPartDialog::validate() {
    mButtons->button(QDialogButtonBox::Ok)->setEnabled(!name().isEmpty());
}
