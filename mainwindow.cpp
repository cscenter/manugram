#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "model.h"
#include <fstream>

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

    std::ifstream("a.model") >> this->modelWidget->model;
}
