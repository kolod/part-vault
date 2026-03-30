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
#include "storagetreemodel.h"
#include "partsmodel.h"
#include "addpartdialog.h"
#include "addcategorydialog.h"
#include "addstoragelocationdialog.h"
#include <QHeaderView>
#include <QDebug>
#include <functional>

MainWindow::MainWindow(DatabaseManager &databaseManager, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mDatabaseManager(databaseManager)
{
    ui->setupUi(this);

    // Parts table
    const QString conn = mDatabaseManager.database().connectionName();

    // Create models
    mPartsModel = new PartsModel(conn, this);
    mCategoryModel = new CategoryTreeModel(conn, this);
    mStorageModel  = new StorageTreeModel(conn, this);

    // Category tree view
    ui->viewCategories->setModel(mCategoryModel);

    // Storage locations tree view
    ui->viewStorageLocations->setModel(mStorageModel);

    // Parts table view
    ui->tableView->setModel(mPartsModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColName,       QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColQuantity,   QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColCategory,   QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColLocation,   QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(PartsModel::ColLastChange, QHeaderView::ResizeToContents);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->verticalHeader()->setVisible(false);

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
        const QString conn = mDatabaseManager.database().connectionName();
        auto index = ui->viewCategories->currentIndex();
        auto categoryId = mCategoryModel->categoryId(index);

        AddCategoryDialog dlg(conn, categoryId, this);
        if (dlg.exec() != QDialog::Accepted) return;

        const int newId = mDatabaseManager.addCategory(dlg.name(), dlg.parentId());
        if (newId >= 0) {
            mCategoryModel->reload(ui->viewCategories);
            const QModelIndex newIndex = mCategoryModel->indexForId(newId);
            if (newIndex.isValid()) {
                ui->viewCategories->expand(newIndex.parent());
                ui->viewCategories->setCurrentIndex(newIndex);
                ui->viewCategories->scrollTo(newIndex);
            }
        }
    });

    // Add Part action
    connect(ui->actionAddPart, &QAction::triggered, this, [this]() {
        const QString conn = mDatabaseManager.database().connectionName();
        AddPartDialog dlg(conn, this);
        if (dlg.exec() != QDialog::Accepted) return;

        if (mDatabaseManager.addPart(dlg.name(), dlg.quantity(), dlg.categoryId(), dlg.locationId()) >= 0)
            mPartsModel->reload();
    });

    // Add Storage Location action
    connect(ui->actionAddSorageLocation, &QAction::triggered, this, [this]() {
        const QString conn = mDatabaseManager.database().connectionName();
        auto index = ui->viewStorageLocations->currentIndex();
        auto locationId = mStorageModel->locationId(index);

        AddStorageLocationDialog dlg(conn, locationId, this);
        if (dlg.exec() != QDialog::Accepted) return;

        const int newId = mDatabaseManager.addStorageLocation(dlg.name(), dlg.parentId());
        if (newId >= 0) {
            mStorageModel->reload(ui->viewStorageLocations);
            const QModelIndex newIndex = mStorageModel->indexForId(newId);
            if (newIndex.isValid()) {
                ui->viewStorageLocations->expand(newIndex.parent());
                ui->viewStorageLocations->setCurrentIndex(newIndex);
                ui->viewStorageLocations->scrollTo(newIndex);
            }
        }
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

    // View Show/Hide Storage Locations Dock action
    connect(ui->actionStorageLocations, &QAction::triggered, this, [this](bool checked) {
        ui->dockStorageLocations->setVisible(checked);
    });

    // Keep the "Show Storage Locations" menu action in sync with the actual visibility of the dock widget
    connect(ui->dockStorageLocations, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        ui->actionStorageLocations->setChecked(visible);
    });

    // Filter by category when a category is selected in the tree view
    connect( ui->viewCategories->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&) {
        mPartsModel->setCategory(mCategoryModel->categoryId(current));
    });

    // Filter by storage location when a location is selected in the tree view
    connect(ui->viewStorageLocations->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&) {
        mPartsModel->setStorageLocation(mStorageModel->locationId(current));
    });
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::restoreSession(){
    QSettings settings;

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());

    // Restore expanded categories
    const QList<QVariant> expandedRaw = settings.value("categories/expanded").toList();
    for (const QVariant& v : expandedRaw) {
        const QModelIndex idx = mCategoryModel->indexForId(v.toInt());
        if (idx.isValid())
            ui->viewCategories->expand(idx);
    }

    // Restore selected category
    const int selectedId = settings.value("categories/selected", 0).toInt();
    if (selectedId > 0) {
        const QModelIndex idx = mCategoryModel->indexForId(selectedId);
        if (idx.isValid()) {
            ui->viewCategories->setCurrentIndex(idx);
            ui->viewCategories->scrollTo(idx);
        }
    }

    // Restore expanded storage locations
    const QList<QVariant> expandedStorageRaw = settings.value("storage/expanded").toList();
    for (const QVariant& v : expandedStorageRaw) {
        const QModelIndex idx = mStorageModel->indexForId(v.toInt());
        if (idx.isValid())
            ui->viewStorageLocations->expand(idx);
    }

    // Restore selected storage location
    const int selectedStorageId = settings.value("storage/selected", 0).toInt();
    if (selectedStorageId > 0) {
        const QModelIndex idx = mStorageModel->indexForId(selectedStorageId);
        if (idx.isValid()) {
            ui->viewStorageLocations->setCurrentIndex(idx);
            ui->viewStorageLocations->scrollTo(idx);
        }
    }
}

void MainWindow::saveSession(){
    QSettings settings;

    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());

    // Save expanded category IDs
    QList<QVariant> expandedIds;
    std::function<void(const QModelIndex&)> collectExpanded = [&](const QModelIndex& parent) {
        for (int r = 0; r < mCategoryModel->rowCount(parent); ++r) {
            const QModelIndex idx = mCategoryModel->index(r, 0, parent);
            if (ui->viewCategories->isExpanded(idx))
                expandedIds.append(mCategoryModel->categoryId(idx));
            collectExpanded(idx);
        }
    };
    collectExpanded(QModelIndex{});
    settings.setValue("categories/expanded", expandedIds);

    // Save selected category ID
    const int selectedId = mCategoryModel->categoryId(ui->viewCategories->currentIndex());
    settings.setValue("categories/selected", selectedId);

    // Save expanded storage location IDs
    QList<QVariant> expandedStorageIds;
    std::function<void(const QModelIndex&)> collectExpandedStorage = [&](const QModelIndex& parent) {
        for (int r = 0; r < mStorageModel->rowCount(parent); ++r) {
            const QModelIndex idx = mStorageModel->index(r, 0, parent);
            if (ui->viewStorageLocations->isExpanded(idx))
                expandedStorageIds.append(mStorageModel->locationId(idx));
            collectExpandedStorage(idx);
        }
    };
    collectExpandedStorage(QModelIndex{});
    settings.setValue("storage/expanded", expandedStorageIds);

    // Save selected storage location ID
    const int selectedStorageId = mStorageModel->locationId(ui->viewStorageLocations->currentIndex());
    settings.setValue("storage/selected", selectedStorageId);
}

void MainWindow::closeEvent(QCloseEvent *event){
    saveSession();
    event->accept();
}
