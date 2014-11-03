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
}

void MainWindow::setModelWidget(Ui::ModelWidget *newWidget) {
    if (this->modelWidget) {
        ui->wrapperFrame->layout()->removeWidget(this->modelWidget);
        delete this->modelWidget;
    }

    this->modelWidget = newWidget;
    ui->wrapperFrame->layout()->addWidget(this->modelWidget);
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
        file >> modelWidget->model;
    } catch (model_format_error &e) {
        QMessageBox::critical(this, "Error while opening model", e.what());
        return;
    }

    setModelWidget(modelWidget.release());
}

void MainWindow::on_actionSaveAs_triggered() {
    QString filename = QFileDialog::getSaveFileName(
                           this,
                           "Select file to save in",
                           QDir::currentPath(),
                           "Models (*.model)"
                       );
    if (filename == "") {
        return;
    }
    std::ofstream file(filename.toStdString());
    file << this->modelWidget->model;
}
