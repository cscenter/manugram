#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "model.h"
#include "model_io.h"
#include "figurepainter.h"
#include "build_info.h"
#include "recognition.h"
#include <sstream>
#include <QByteArray>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QScreen>

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
    setWindowIcon(QIcon(":/icon.ico"));
    if (QApplication::arguments().size() > 1) {
        openFile(QApplication::arguments()[1]);
    }
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
    modelWidget->setStoreTracks(ui->actionStoreTracks->isChecked());

    QScreen *screen = QApplication::screens().at(0);
    modelWidget->setScaleFactor(screen->logicalDotsPerInch() / 96.0);
}

void MainWindow::on_actionOpen_triggered() {
    QString filename = QFileDialog::getOpenFileName(
                           this,
                           "Select file with a model",
                           QDir::currentPath(),
                           "Models (*.model);;Tracks (*.track)"
                       );
    if (filename == "") {
        return;
    }
    openFile(filename);
}

void MainWindow::openFile(const QString &filename) {
    std::unique_ptr<Ui::ModelWidget> modelWidget(new Ui::ModelWidget());
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::critical(this, "Error while opening model", "Unable to open file for reading");
        return;
    }
    std::stringstream data;
    data << file.readAll().toStdString();
    file.close();
    if (filename.toLower().endsWith(".track")) {
        try {
            Model model;
            Track track;
            data >> track;
            recognize(track, model);
            modelWidget->setModel(std::move(model));
            modelWidget->addModelExtraTrack(std::move(track));
        } catch (model_format_error &e) {
            QMessageBox::critical(this, "Error while opening track", e.what());
            return;
        }
    } else {
        try {
            Model model;
            data >> model;
            modelWidget->setModel(std::move(model));
        } catch (model_format_error &e) {
            QMessageBox::critical(this, "Error while opening model", e.what());
            return;
        }
    }

    setModelWidget(modelWidget.release());
    currentFileName = filename;
}

void saveDataToFile(const std::string &data, QString &fileName) {
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        throw io_error("Cannot open file for writing");
    }
    QByteArray buffer(data.data(), data.length());
    if (file.write(buffer) != buffer.size()) {
        throw io_error("Cannot write data to file");
    }
}

void MainWindow::on_actionSave_triggered() {
    if (currentFileName == QString()) {
        ui->actionSaveAs->trigger();
        return;
    }
    std::stringstream data;
    data << this->modelWidget->getModel();
    try {
        saveDataToFile(data.str(), currentFileName);
    } catch (io_error &e) {
        QMessageBox::critical(this, "Unable to save model", e.what());
    }
}

QString forceFileExtension(QString filename, QString selectedFilter) {
    QString extension = selectedFilter.mid(selectedFilter.indexOf("(*.") + 3);
    extension.chop(1); // chop trailing )
    extension = ("." + extension).toLower();
    if (!filename.endsWith(extension)) {
        filename += extension;
    }
    return filename;
}

void MainWindow::on_actionSaveAs_triggered() {
    QString selectedFilter;
    QString filename = QFileDialog::getSaveFileName(
                           this,
                           "Select file to save in",
                           QDir::currentPath(),
                           "Models (*.model);;SVG (*.svg);;PNG (*.png)",
                           &selectedFilter
                       );
    if (filename == "") {
        return;
    }
    filename = forceFileExtension(filename, selectedFilter);
    Model &model = this->modelWidget->getModel();
    try {
        if (filename.toLower().endsWith(".svg")) {
            std::stringstream data;
            exportModelToSvg(model, data);
            saveDataToFile(data.str(), filename);
        } else if (filename.toLower().endsWith(".png")) {
            exportModelToImageFile(model, filename);
        } else {
            std::stringstream data;
            data << model;
            saveDataToFile(data.str(), filename);
            currentFileName = filename;
        }
    } catch (io_error &e) {
        QMessageBox::critical(this, "Unable to save model", e.what());
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

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, "About Manugram",
                       QString()
                       + "Git commit hash: " + GIT_LAST_SHA1 + "\n"
                       + "Git commit date: " + GIT_LAST_TIME + "\n"
                       + "Build time: " + BUILD_TIME
                       );
}

void MainWindow::on_actionStoreTracks_triggered() {
    modelWidget->setStoreTracks(ui->actionStoreTracks->isChecked());
}
