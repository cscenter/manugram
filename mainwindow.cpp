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
    if (this->modelWidget) {
        ui->wrapperLayout->removeWidget(this->modelWidget);
        delete this->modelWidget;
    }

    this->modelWidget = new Ui::ModelWidget();
    ui->wrapperLayout->addWidget(this->modelWidget);

    QString filename = QFileDialog::getOpenFileName(
                this,
                "Select file with a model",
                QDir::currentPath(),
                "Models (*.model)"
                );
    if (filename == "") {
        return;
    }
    std::ifstream file(filename.toStdString());
    file >> this->modelWidget->model;
}
