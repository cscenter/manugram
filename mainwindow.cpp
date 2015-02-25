#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "model.h"
#include "figurepainter.h"
#include <fstream>
#include <QFileDialog>
#include <QInputDialog>
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

void exportModelToPng(Model &model, const QString &filename) {
    Point minPoint(INFINITY, INFINITY);
    Point maxPoint(-INFINITY, -INFINITY);
    for (const PFigure &fig : model) {
        BoundingBox box = fig->getBoundingBox();
        minPoint.x = std::min(minPoint.x, box.leftDown.x);
        minPoint.y = std::min(minPoint.y, box.leftDown.y);
        maxPoint.x = std::max(maxPoint.x, box.rightUp.x);
        maxPoint.y = std::max(maxPoint.y, box.rightUp.y);
    }
    if (minPoint.x > maxPoint.x) {
        minPoint = maxPoint = Point(0, 0);
    }
    {
        double w = maxPoint.x - minPoint.x;
        double h = maxPoint.y - minPoint.y;
        minPoint.x -= w * 0.05;
        minPoint.y -= w * 0.05;
        maxPoint.x += h * 0.05;
        maxPoint.y += h * 0.05;
    }

    QImage img(maxPoint.x - minPoint.x, maxPoint.y - minPoint.y, QImage::Format_ARGB32);
    QPainter painter(&img);
    painter.fillRect(QRect(QPoint(), img.size()), Qt::white);
    painter.setPen(Qt::black);
    FigurePainter fpainter(painter, minPoint);
    for (PFigure fig : model) {
        fig->visit(fpainter);
    }
    painter.end();
    if (!img.save(filename)) {
        throw std::runtime_error("Unable to save to PNG file");
    }
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
        exportModelToPng(model, filename);
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
    if (ui->actionShowGrid->isChecked()) {
        defaultGridStep = QInputDialog::getInt(this, "Grid step", "Specify grid step in pixels", defaultGridStep, 1, int(1e9));
    }
    modelWidget->setGridStep(ui->actionShowGrid->isChecked() ? defaultGridStep : 0);
}

void MainWindow::on_actionExit_triggered() {
    QApplication::exit();
}
