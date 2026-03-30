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

#include "addcategorydialog.h"

#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QPushButton>

AddCategoryDialog::AddCategoryDialog(const QString& connectionName, int parentId, QWidget* parent)
    : QDialog(parent), m_parentId(parentId)
{
    setWindowTitle(tr("Add Category"));
    setMinimumWidth(360);

    m_nameEdit = new QLineEdit(this);

    const QString path = buildPath(connectionName);
    auto* pathLabel = new QLabel(path.isEmpty() ? tr("(top level)") : path, this);
    pathLabel->setWordWrap(true);

    auto* form = new QFormLayout;
    form->addRow(tr("Parent:"), pathLabel);
    form->addRow(tr("Name:"),   m_nameEdit);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(m_buttons);

    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddCategoryDialog::validate);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString AddCategoryDialog::name() const {
    return m_nameEdit->text().trimmed();
}

int AddCategoryDialog::parentId() const {
    return m_parentId;
}

// ── private ──────────────────────────────────────────────────────────────────

// Walk parent_id up from m_parentId and build "Root → Child → …" display string.
QString AddCategoryDialog::buildPath(const QString& connectionName) const {
    if (m_parentId <= 0) return {};

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery q(db);

    QStringList parts;
    int current = m_parentId;
    while (current > 0) {
        q.prepare("SELECT name, parent_id FROM categories WHERE id = ?");
        q.addBindValue(current);
        if (!q.exec() || !q.next()) {
            qWarning() << "AddCategoryDialog: category" << current << "not found";
            break;
        }
        parts.prepend(q.value(0).toString());
        current = q.value(1).isNull() ? 0 : q.value(1).toInt();
    }
    return parts.join(QString(" > "));
}

void AddCategoryDialog::validate() {
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!name().isEmpty());
}
