#include "addcategorydialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

AddCategoryDialog::AddCategoryDialog(const QString& connectionName, QWidget* parent)
    : QDialog(parent), m_connectionName(connectionName)
{
    setWindowTitle(tr("Add Category"));
    setMinimumWidth(380);

    m_nameEdit = new QLineEdit(this);

    auto* form = new QFormLayout;
    form->addRow(tr("Name:"), m_nameEdit);

    auto* cascadeLabel     = new QLabel(tr("Parent category:"), this);
    auto* cascadeContainer = new QWidget(this);
    m_cascadeLayout        = new QVBoxLayout(cascadeContainer);
    m_cascadeLayout->setContentsMargins(0, 0, 0, 0);
    m_cascadeLayout->setSpacing(4);

    m_backButton      = new QPushButton(tr("\u2190 Back"), this);
    m_addSubcatButton = new QPushButton(tr("New subcategory\u2026"), this);

    auto* btnRow = new QHBoxLayout;
    btnRow->addWidget(m_backButton);
    btnRow->addWidget(m_addSubcatButton);
    btnRow->addStretch();

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(cascadeLabel);
    mainLayout->addWidget(cascadeContainer);
    mainLayout->addLayout(btnRow);
    mainLayout->addWidget(m_buttons);

    // Bootstrap: first level shows children of the virtual "All" root (id = 0)
    addCascadeLevel(0);

    connect(m_backButton,      &QPushButton::clicked,       this, &AddCategoryDialog::removeCascadeLevel);
    connect(m_addSubcatButton, &QPushButton::clicked,       this, &AddCategoryDialog::onAddSubcategoryClicked);
    connect(m_nameEdit,        &QLineEdit::textChanged,     this, &AddCategoryDialog::validate);
    connect(m_buttons,         &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons,         &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString AddCategoryDialog::name() const
{
    return m_nameEdit->text().trimmed();
}

int AddCategoryDialog::parentId() const
{
    if (m_combos.isEmpty()) return -1;
    return m_combos.last()->currentData().toInt();
}

// ── private ──────────────────────────────────────────────────────────────────

void AddCategoryDialog::addCascadeLevel(int parentId)
{
    auto* combo = new QComboBox(this);
    combo->addItem(tr("(select category)"), -1);
    loadChildren(parentId, combo);
    m_cascadeLayout->addWidget(combo);
    m_combos.append(combo);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, combo](int) { onComboChanged(combo); });

    validate();
}

void AddCategoryDialog::removeCascadeLevel()
{
    if (m_combos.size() <= 1) return;

    QComboBox* last = m_combos.takeLast();
    m_cascadeLayout->removeWidget(last);
    last->deleteLater();

    validate();
}

bool AddCategoryDialog::hasChildren(int categoryId) const
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare("SELECT 1 FROM categories WHERE parent_id = ? LIMIT 1");
    q.addBindValue(categoryId);
    return q.exec() && q.next();
}

void AddCategoryDialog::loadChildren(int parentId, QComboBox* combo)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare("SELECT id, name FROM categories WHERE parent_id = ? ORDER BY name");
    q.addBindValue(parentId);
    if (!q.exec()) {
        qWarning() << "AddCategoryDialog loadChildren failed:" << q.lastError().text();
        return;
    }
    while (q.next())
        combo->addItem(q.value(1).toString(), q.value(0).toInt());
}

void AddCategoryDialog::onComboChanged(QComboBox* combo)
{
    const int level = m_combos.indexOf(combo);
    if (level < 0) return;

    // Discard every combo that was below this one
    while (m_combos.size() > level + 1) {
        QComboBox* last = m_combos.takeLast();
        m_cascadeLayout->removeWidget(last);
        last->deleteLater();
    }

    const int selectedId = combo->currentData().toInt();
    if (selectedId > 0 && hasChildren(selectedId))
        addCascadeLevel(selectedId);    // will call validate()
    else
        validate();
}

void AddCategoryDialog::onAddSubcategoryClicked()
{
    const int pid = parentId();
    if (pid <= 0) return;

    bool ok = false;
    const QString newName = QInputDialog::getText(
        this, tr("New Subcategory"), tr("Name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || newName.trimmed().isEmpty()) return;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare("INSERT INTO categories (name, parent_id) VALUES (?, ?)");
    q.addBindValue(newName.trimmed());
    q.addBindValue(pid);
    if (!q.exec()) {
        qWarning() << "AddCategoryDialog: new subcategory insert failed:" << q.lastError().text();
        return;
    }
    const int newId = q.lastInsertId().toInt();

    // Add a level showing children of pid (which now includes the new one)
    addCascadeLevel(pid);

    // Auto-select the new item so the user can continue drilling down
    QComboBox* newCombo = m_combos.last();
    for (int i = 0; i < newCombo->count(); ++i) {
        if (newCombo->itemData(i).toInt() == newId) {
            newCombo->setCurrentIndex(i);
            break;
        }
    }
}

void AddCategoryDialog::validate()
{
    const bool nameOk   = !name().isEmpty();
    const bool parentOk = parentId() > 0;
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(nameOk && parentOk);
    m_backButton->setEnabled(m_combos.size() > 1);
    m_addSubcatButton->setEnabled(parentId() > 0);
}
