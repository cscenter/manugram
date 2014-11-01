#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "model.h"
#include <fstream>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    modelWidget(nullptr)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_buttonLoad_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                "Select file with a model",
                QDir::currentPath(),
                "Models (*.model)"
                );
    if (filename == "") {
        return;
    }

    if (this->modelWidget) {
        ui->wrapperLayout->removeWidget(this->modelWidget);
        delete this->modelWidget;
    }

    this->modelWidget = new Ui::ModelWidget();
    ui->wrapperLayout->addWidget(this->modelWidget);

    std::ifstream file(filename.toStdString());
    file >> this->modelWidget->model;
    ui->buttonSave->setEnabled(true);
}

void MainWindow::on_buttonSave_clicked()
{
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
