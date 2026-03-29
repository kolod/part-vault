#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeView>
#include "categorytreemodel.h"

MainWindow::MainWindow(DatabaseManager &databaseManager, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_databaseManager(databaseManager)
{
    ui->setupUi(this);

    // Set up the category combo box with a tree view and model
    ui->cbCategory->setView(new QTreeView(this));
    ui->cbCategory->setModel(new CategoryTreeModel(m_databaseManager.database(), this));
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
