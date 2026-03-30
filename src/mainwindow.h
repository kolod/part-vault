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

#include <QMainWindow>
#include <QCloseEvent>
#include <QSettings>
#include <QTreeView>
#include <QMouseEvent>
#include <QAction>
#include <QMessageBox>

#include "database.h"
#include "models/partsmodel.h"
#include "models/categorytreemodel.h"
#include "models/storagetreemodel.h"

// QTreeView subclass that prevents the parent QComboBox from closing its
// popup when the user clicks a branch expand/collapse indicator.
class ComboTreeView : public QTreeView
{
    Q_OBJECT
public:
    using QTreeView::QTreeView;
protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        const QModelIndex idx = indexAt(e->pos());
        // If the index is valid but the click is in the indent/branch area
        // (i.e. to the left of the visual rect), treat it as expand/collapse
        // only — do not forward to QComboBox so it won't close the popup.
        if (idx.isValid()) {
            const QRect vr = visualRect(idx);
            if (e->pos().x() < vr.left()) {
                setExpanded(idx, !isExpanded(idx));
                return;
            }
        }
        QTreeView::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        const QModelIndex idx = indexAt(e->pos());
        if (idx.isValid()) {
            const QRect vr = visualRect(idx);
            if (e->pos().x() < vr.left()) {
                // Do not forward to QComboBox, so it won't close the popup.
                return;
            }
        }
        QTreeView::mouseReleaseEvent(e);
    }
};

QT_BEGIN_NAMESPACE
namespace Ui {class MainWindow;}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(DatabaseManager &databaseManager, QWidget *parent = nullptr);
    ~MainWindow() override;
    void restoreSession();
    void saveSession();

private:
    void closeEvent(QCloseEvent *event) override;

    Ui::MainWindow    *ui;
    DatabaseManager   &mDatabaseManager;
    PartsModel        *mPartsModel    = nullptr;
    CategoryTreeModel *mCategoryModel = nullptr;
    StorageTreeModel  *mStorageModel  = nullptr;
};
