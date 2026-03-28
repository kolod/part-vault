#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
