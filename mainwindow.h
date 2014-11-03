#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "modelwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Ui::ModelWidget *modelWidget;

private slots:
    void on_actionOpen_triggered();
    void on_actionSaveAs_triggered();
};

#endif // MAINWINDOW_H
