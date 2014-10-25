#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "model.h"
#include <fstream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_buttonLoad_clicked()
{
    Model m;
    std::cout << "Loading model:\n";
    std::ifstream("a.model") >> m;
    for (PFigure f : m) {
      std::cout << f->str() << "\n";
    }
}
