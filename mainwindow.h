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

    void setModelWidget(Ui::ModelWidget *widget);

private slots:
    void on_actionOpen_triggered();
    void on_actionSaveAs_triggered();
    void on_actionNew_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionShowGrid_triggered();
};

#endif // MAINWINDOW_H
