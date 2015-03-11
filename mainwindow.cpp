#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "model.h"
#include "model_io.h"
#include "figurepainter.h"
#include <fstream>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modelWidget(nullptr),
    redoExtraShortcut(QKeySequence("Ctrl+Shift+Z"), this),
    zoomInExtraShortcut(QKeySequence("Ctrl+="), this)
{
    ui->setupUi(this);
    connect(&redoExtraShortcut  , &QShortcut::activated, ui->actionRedo  , &QAction::trigger);
    connect(&zoomInExtraShortcut, &QShortcut::activated, ui->actionZoomIn, &QAction::trigger);
    setModelWidget(new Ui::ModelWidget());
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_actionNew_triggered() {
    setModelWidget(new Ui::ModelWidget());
    currentFileName = QString();
}

void MainWindow::setModelWidget(Ui::ModelWidget *newWidget) {
    if (this->modelWidget) {
        ui->wrapperFrame->layout()->removeWidget(this->modelWidget);
        delete this->modelWidget;
    }

    this->modelWidget = newWidget;
    ui->wrapperFrame->layout()->addWidget(this->modelWidget);
    ui->actionSaveAs->setEnabled(true);
    ui->actionUndo->setEnabled(modelWidget->canUndo());
    connect(modelWidget, &Ui::ModelWidget::canUndoChanged, [this]() {
        ui->actionUndo->setEnabled(modelWidget->canUndo());
    });
    ui->actionRedo->setEnabled(modelWidget->canRedo());
    connect(modelWidget, &Ui::ModelWidget::canRedoChanged, [this]() {
        ui->actionRedo->setEnabled(modelWidget->canRedo());
    });
    connect(modelWidget, &Ui::ModelWidget::scaleFactorChanged, [this]() {
        ui->actionZoomOut->setEnabled(modelWidget->scaleFactor() > SCALE_FACTOR_STEP + 1e-8);
    });
    modelWidget->setGridStep(ui->actionShowGrid->isChecked() ? defaultGridStep : 0);
}

void MainWindow::on_actionOpen_triggered() {
    QString filename = QFileDialog::getOpenFileName(
                           this,
                           "Select file with a model",
                           QDir::currentPath(),
                           "Models (*.model)"
                       );
    if (filename == "") {
        return;
    }

    std::unique_ptr<Ui::ModelWidget> modelWidget(new Ui::ModelWidget());
    std::ifstream file(filename.toStdString());
    try {
        Model model;
        file >> model;
        modelWidget->setModel(std::move(model));
    } catch (model_format_error &e) {
        QMessageBox::critical(this, "Error while opening model", e.what());
        return;
    }

    setModelWidget(modelWidget.release());
    currentFileName = filename;
}

void MainWindow::on_actionSave_triggered() {
    if (currentFileName == QString()) {
        ui->actionSaveAs->trigger();
        return;
    }
    Model &model = this->modelWidget->getModel();
    std::ofstream file(currentFileName.toStdString());
    file << model;
}

void MainWindow::on_actionSaveAs_triggered() {
    QString filename = QFileDialog::getSaveFileName(
                           this,
                           "Select file to save in",
                           QDir::currentPath(),
                           "Models (*.model);;SVG (*.svg);;PNG (*.png)"
                       );
    if (filename == "") {
        return;
    }
    Model &model = this->modelWidget->getModel();
    std::ofstream file(filename.toStdString());
    if (filename.toLower().endsWith(".svg")) {
        exportModelToSvg(model, file);
    } else if (filename.toLower().endsWith(".png")) {
        file.close();
        exportModelToImageFile(model, filename);
    } else {
        file << model;
        currentFileName = filename;
    }
}

void MainWindow::on_actionUndo_triggered() {
    // Extra check is added for symmetry with actionRedo
    if (!modelWidget->canUndo()) { return; }
    modelWidget->undo();
}

void MainWindow::on_actionRedo_triggered() {
    // Extra check is added because there is extra shortcut
    if (!modelWidget->canRedo()) { return; }
    modelWidget->redo();
}

void MainWindow::on_actionShowGrid_triggered() {
    if (ui->actionShowGrid->isChecked()) {
        defaultGridStep = QInputDialog::getInt(this, "Grid step", "Specify grid step in pixels", defaultGridStep, 1, int(1e9));
    }
    modelWidget->setGridStep(ui->actionShowGrid->isChecked() ? defaultGridStep : 0);
}

void MainWindow::on_actionZoomIn_triggered() {
    modelWidget->setScaleFactor(modelWidget->scaleFactor() + SCALE_FACTOR_STEP);
}

void MainWindow::on_actionZoomOut_triggered() {
    modelWidget->setScaleFactor(modelWidget->scaleFactor() - SCALE_FACTOR_STEP);
}

void MainWindow::on_actionExit_triggered() {
    QApplication::exit();
}
