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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "categorytreemodel.h"
#include "partsmodel.h"
#include "addpartdialog.h"
#include "addcategorydialog.h"
#include "addstoragelocationdialog.h"
#include <QHeaderView>
#include <QDebug>

MainWindow::MainWindow(DatabaseManager &databaseManager, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_databaseManager(databaseManager)
{
    ui->setupUi(this);


    // Category tree
    m_categoryModel = new CategoryTreeModel(m_databaseManager.database(), this);
    ui->viewCategories->setModel(m_categoryModel);

    // Parts table
    const QString conn = m_databaseManager.database().connectionName();

    m_partsModel = new PartsModel(conn, this);
    ui->tableView->setModel(m_partsModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColName,     QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColQuantity, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColCategory, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColLocation, QHeaderView::ResizeToContents);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->verticalHeader()->setVisible(false);

    // Filter parts when a category is selected in the combo box
    // connect(ui->treeView, qOverload<int>(&QComboBox::currentIndexChanged),
    //         this, [this, categoryModel]() {
    //             const QModelIndex idx = ui->cbCategory->view()->currentIndex();
    //             m_partsModel->setCategory(categoryModel->categoryId(idx));
    //         });


    // Menu actions

    // Exit action
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    
    // About action
    connect(ui->actionAbout, &QAction::triggered, this, []() {
        QMessageBox::about(nullptr, tr("About PartVault"),
            tr("PartVault — simple inventory manager for electronic components\n"
               "Copyright (C) 2026-...  Oleksandr Kolodkin")
        );
    });

    // About Qt action
    connect(ui->actionAboutQt, &QAction::triggered, this, []() {
        QMessageBox::aboutQt(nullptr, tr("About Qt"));
    });

    // Add Category action
    connect(ui->actionAddCategory, &QAction::triggered, this, [this]() {
        const QString conn = m_databaseManager.database().connectionName();
        auto index = ui->viewCategories->currentIndex();
        auto categoryId = m_categoryModel->categoryId(index);

        AddCategoryDialog dlg(conn, this);
        dlg.setCategory(categoryId);
        if (dlg.exec() != QDialog::Accepted) return;

        if (m_databaseManager.addCategory(dlg.name(), dlg.parentId()) >= 0)
            m_categoryModel->reload();
    });

    // Add Part action
    connect(ui->actionAddPart, &QAction::triggered, this, [this]() {
        const QString conn = m_databaseManager.database().connectionName();
        AddPartDialog dlg(conn, this);
        if (dlg.exec() != QDialog::Accepted) return;

        if (m_databaseManager.addPart(dlg.name(), dlg.quantity(), dlg.categoryId(), dlg.locationId()) >= 0)
            m_partsModel->reload();
    });

    // Add Storage Location action
    connect(ui->actionAddSorageLocation, &QAction::triggered, this, [this]() {
        AddStorageLocationDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;

        m_databaseManager.addStorageLocation(dlg.name());
    });

    // Remove Part action
    connect(ui->actionRemovePart, &QAction::triggered, this, []() {
        QMessageBox::information(nullptr, tr("Remove Part"), tr("Remove Part action triggered"));
    });

    // Remove Unused Category action
    connect(ui->actionRemoveUnusedCategories, &QAction::triggered, this, []() {
        QMessageBox::information(nullptr, tr("Remove Unused Category"), tr("Remove Unused Category action triggered"));
    });

    // Remove Unused Storage Location action
    connect(ui->actionRemoveUnusedStorageLocations, &QAction::triggered, this, []() {
        QMessageBox::information(nullptr, tr("Remove Unused Storage Location"), tr("Remove Unused Storage Location action triggered"));
    });

    // View Show/Hide Category Dock action
    connect(ui->actionCategories, &QAction::triggered, this, [this](bool checked) {
        ui->dockCategories->setVisible(checked);
    });

    // Keep the "Show Categories" menu action in sync with the actual visibility of the dock widget
    connect(ui->dockCategories, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        ui->actionCategories->setChecked(visible);
    });

    // Filter by category when a category is selected in the tree view
    connect( ui->viewCategories->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&) {
        m_partsModel->setCategory(m_categoryModel->categoryId(current));
    });
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::restoreSession(){
    QSettings settings;

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
}

void MainWindow::saveSession(){
    QSettings settings;

    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
}

void MainWindow::closeEvent(QCloseEvent *event){
    saveSession();
    event->accept();
}
