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
#include "models/categorytreemodel.h"
#include "models/storagetreemodel.h"
#include "models/partsmodel.h"
#include "dialogs/addpartdialog.h"
#include "dialogs/addcategorydialog.h"
#include "dialogs/addstoragelocationdialog.h"
#include <QHeaderView>
#include <QMenu>
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
    ui->viewCategories->setDragEnabled(true);
    ui->viewCategories->setAcceptDrops(true);
    ui->viewCategories->setDropIndicatorShown(true);
    ui->viewCategories->setDragDropMode(QAbstractItemView::DragDrop);
    ui->viewCategories->setDefaultDropAction(Qt::MoveAction);

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
            ui->viewCategories->reloadPreservingExpanded();
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
        const int catId = mCategoryModel->categoryId(ui->viewCategories->currentIndex());
        const int locId = mStorageModel->locationId(ui->viewStorageLocations->currentIndex());
        AddPartDialog dlg(conn, catId, locId, this);
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
            ui->viewStorageLocations->reloadPreservingExpanded();
            const QModelIndex newIndex = mStorageModel->indexForId(newId);
            if (newIndex.isValid()) {
                ui->viewStorageLocations->expand(newIndex.parent());
                ui->viewStorageLocations->setCurrentIndex(newIndex);
                ui->viewStorageLocations->scrollTo(newIndex);
            }
        }
    });

    // Remove Part action
    connect(ui->actionRemovePart, &QAction::triggered, this, [this]() {
        const QModelIndex idx = ui->tableView->currentIndex();
        if (!idx.isValid()) return;
        const int partId = mPartsModel->partId(idx.row());
        const QString name = mPartsModel->data(mPartsModel->index(idx.row(), PartsModel::ColName)).toString();

        // Confirm deletion
        if (QMessageBox::question(this, tr("Remove Part"), tr("Delete \"%1\"?").arg(name)) != QMessageBox::Yes) 
            return;

        // Attempt deletion
        if (!mDatabaseManager.removePart(partId)) {
            QMessageBox::warning(this, tr("Error"), tr("Failed to delete the part."));
            return;
        }

        // Refresh the parts list
        mPartsModel->reload();
    });

    // Remove Unused Category action
    connect(ui->actionRemoveUnusedCategories, &QAction::triggered, this, [this]() {
        if (QMessageBox::question(this, tr("Remove Unused Categories"),
                tr("Delete all categories that have no parts assigned (directly or in any sub-category)?"))
            != QMessageBox::Yes) return;
        const int n = mDatabaseManager.removeUnusedCategories();
        if (n < 0) {
            QMessageBox::warning(this, tr("Error"), tr("Failed to remove unused categories."));
        } else if (n == 0) {
            QMessageBox::information(this, tr("Remove Unused Categories"), tr("No unused categories found."));
        } else {
            ui->viewCategories->reloadPreservingExpanded();
            QMessageBox::information(this, tr("Remove Unused Categories"),
                tr("%n category(ies) removed.", nullptr, n));
        }
    });

    // Remove Unused Storage Location action
    connect(ui->actionRemoveUnusedStorageLocations, &QAction::triggered, this, [this]() {
        if (QMessageBox::question(this, tr("Remove Unused Storage Locations"),
                tr("Delete all storage locations that have no parts assigned (directly or in any sub-location)?"))
            != QMessageBox::Yes) return;
        const int n = mDatabaseManager.removeUnusedStorageLocations();
        if (n < 0) {
            QMessageBox::warning(this, tr("Error"), tr("Failed to remove unused storage locations."));
        } else if (n == 0) {
            QMessageBox::information(this, tr("Remove Unused Storage Locations"), tr("No unused storage locations found."));
        } else {
            ui->viewStorageLocations->reloadPreservingExpanded();
            QMessageBox::information(this, tr("Remove Unused Storage Locations"),
                tr("%n location(s) removed.", nullptr, n));
        }
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

    // Context menu for category tree
    ui->viewCategories->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->viewCategories, &QTreeView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        QAction* act = menu.addAction(tr("Add Category…"));
        if (menu.exec(ui->viewCategories->viewport()->mapToGlobal(pos)) == act)
            ui->actionAddCategory->trigger();
    });

    // Drag & drop reparent confirmation
    connect(mCategoryModel, &CategoryTreeModel::reparentRequested, this, [this](int categoryId, int newParentId) {
        const QString catName    = mCategoryModel->data(mCategoryModel->indexForId(categoryId)).toString();
        const QString parentName = (newParentId == 0)
            ? tr("top level")
            : mCategoryModel->data(mCategoryModel->indexForId(newParentId)).toString();
        if (QMessageBox::question(this, tr("Move Category"),
                tr("Move \"%1\" into \"%2\"?").arg(catName, parentName))
            != QMessageBox::Yes) return;
        mCategoryModel->reparentCategory(categoryId, newParentId);
        ui->viewCategories->reloadPreservingExpanded();
        ui->viewCategories->setCurrentIndex(mCategoryModel->indexForId(categoryId));
    });

    // Context menu for storage locations tree
    ui->viewStorageLocations->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->viewStorageLocations, &QTreeView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        QAction* act = menu.addAction(tr("Add Storage Location…"));
        if (menu.exec(ui->viewStorageLocations->viewport()->mapToGlobal(pos)) == act)
            ui->actionAddSorageLocation->trigger();
    });

    // Filter by category when a category is selected in the tree view
    connect( ui->viewCategories->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&) {
        const int catId = mCategoryModel->categoryId(current);
        mPartsModel->setCategory(catId);
        mStorageModel->setCategoryFilter(catId);
    });

    // Filter by storage location when a location is selected in the tree view
    connect(ui->viewStorageLocations->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&) {
        const int locId = mStorageModel->locationId(current);
        mPartsModel->setStorageLocation(locId);
        mCategoryModel->setLocationFilter(locId);
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::restoreSession() {
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

void MainWindow::saveSession() {
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

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSession();
    event->accept();
}
