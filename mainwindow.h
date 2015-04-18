#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include "modelwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void openFile(const QString &filename);

private:
    static constexpr double SCALE_FACTOR_STEP = 0.2;
    Ui::MainWindow *ui;
    Ui::ModelWidget *modelWidget;
    QString currentFileName;
    int defaultGridStep = 30;

    QShortcut redoExtraShortcut;
    QShortcut zoomInExtraShortcut;

    void setModelWidget(Ui::ModelWidget *widget);

private slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionNew_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionShowGrid_triggered();
    void on_actionZoomIn_triggered();
    void on_actionZoomOut_triggered();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();
    void on_actionStoreTracks_triggered();
};

#endif // MAINWINDOW_H
