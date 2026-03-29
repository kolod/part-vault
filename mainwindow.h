#pragma once

#include <QMainWindow>
#include <QCloseEvent>
#include <QSettings>

#include "database.h"

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

private:
    Ui::MainWindow *ui;
    DatabaseManager &m_databaseManager;
};
