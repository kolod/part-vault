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

#include "addstoragelocationdialog.h"


#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>

AddStorageLocationDialog::AddStorageLocationDialog(const QString& connectionName, int parentId, QWidget* parent)
    : QDialog(parent), mParentId(parentId)
{
    setWindowTitle(tr("Add Storage Location"));
    setMinimumWidth(320);

    mNameEdit = new QLineEdit(this);

    const QString path = buildPath(connectionName);
    auto* pathLabel = new QLabel(path.isEmpty() ? tr("(top level)") : path, this);
    pathLabel->setWordWrap(true);

    auto* form = new QFormLayout;
    form->addRow(tr("Parent:"), pathLabel);
    form->addRow(tr("Name:"),   mNameEdit);

    mButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtons->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(mButtons);

    connect(mNameEdit, &QLineEdit::textChanged, this, &AddStorageLocationDialog::validate);
    connect(mButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString AddStorageLocationDialog::name() const
{
    return mNameEdit->text().trimmed();
}

int AddStorageLocationDialog::parentId() const
{
    return mParentId;
}

QString AddStorageLocationDialog::buildPath(const QString& connectionName) const
{
    if (mParentId <= 0) return {};

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery q(db);

    QStringList parts;
    int current = mParentId;
    while (current > 0) {
        q.prepare("SELECT name, parent_id FROM storage_locations WHERE id = ?");
        q.addBindValue(current);
        if (!q.exec() || !q.next()) {
            qWarning() << "AddStorageLocationDialog: location" << current << "not found";
            break;
        }
        parts.prepend(q.value(0).toString());
        current = q.value(1).isNull() ? 0 : q.value(1).toInt();
    }
    return parts.join(QString(" \u2192 "));
}

void AddStorageLocationDialog::validate()
{
    mButtons->button(QDialogButtonBox::Ok)->setEnabled(!name().isEmpty());
}
