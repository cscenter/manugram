#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "model.h"
#include <fstream>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modelWidget(nullptr)

{
    ui->setupUi(this);
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
    modelWidget->setGridStep(ui->actionShowGrid->isChecked() ? 30 : 0);
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

void MainWindow::on_actionSaveAs_triggered() {
    QString filename = QFileDialog::getSaveFileName(
                           this,
                           "Select file to save in",
                           QDir::currentPath(),
                           "Models (*.model);;SVG (*.svg)"
                       );
    if (filename == "") {
        return;
    }
    Model &model = this->modelWidget->getModel();
    std::ofstream file(filename.toStdString());
    if (filename.toLower().endsWith(".svg")) {
        exportModelToSvg(model, file);
    } else {
        file << model;
        currentFileName = filename;
    }
}

void MainWindow::on_actionUndo_triggered() {
    modelWidget->undo();
}

void MainWindow::on_actionRedo_triggered() {
    modelWidget->redo();
}

void MainWindow::on_actionShowGrid_triggered() {
    modelWidget->setGridStep(ui->actionShowGrid->isChecked() ? 30 : 0);
}

void MainWindow::on_actionExit_triggered() {
    QApplication::exit();
}
