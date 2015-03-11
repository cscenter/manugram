#include "model.h"
#include "modelwidget.h"
#include "figurepainter.h"
#include "recognition.h"
#include "layouting.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QTimer>
#include <iostream>

Ui::ModelWidget::ModelWidget(QWidget *parent) :
    QWidget(parent), mouseAction(MouseAction::None), _gridStep(0) {
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
}

void drawTrack(QPainter &painter, Scaler &scaler, const Track &track) {
    for (size_t i = 0; i + 1 < track.size(); i++) {
        painter.drawLine(scaler(track[i]), scaler(track[i + 1]));
    }
}

int Ui::ModelWidget::gridStep() {
    return _gridStep;
}

void Ui::ModelWidget::setGridStep(int newGridStep) {
    if (newGridStep < 0) {
        throw std::runtime_error("Grid step should be >= 0");
    }
    _gridStep = newGridStep;
    repaint();
}

double Ui::ModelWidget::scaleFactor() {
    return scaler.scaleFactor;
}

void Ui::ModelWidget::setScaleFactor(double newScaleFactor) {
    if (!(newScaleFactor >= 0.01)) { // >= instead of < for NaNs
        throw std::runtime_error("Scale factor should be >= 0.01");
    }
    scaler.scaleFactor = newScaleFactor;
    emit scaleFactorChanged();
    repaint();
}

void Ui::ModelWidget::setModel(Model model) {
    commitedModel = std::move(model);
    previousModels.clear();
    redoModels.clear();
    emit canUndoChanged();
    emit canRedoChanged();
}
Model &Ui::ModelWidget::getModel() {
    return commitedModel;
}

bool Ui::ModelWidget::canUndo() {
    return !previousModels.empty();
}

void Ui::ModelWidget::undo() {
    if (!canUndo()) {
        throw std::runtime_error("Cannot undo");
    }
    redoModels.push_front(std::move(commitedModel));
    commitedModel = std::move(previousModels.back());
    previousModels.pop_back();
    if (!canUndo()) {
        emit canUndoChanged();
    }
    emit canRedoChanged();
    repaint();
}

bool Ui::ModelWidget::canRedo() {
    return !redoModels.empty();
}

void Ui::ModelWidget::redo() {
    if (!canRedo()) {
        throw std::runtime_error("Cannot redo");
    }
    previousModels.push_back(std::move(commitedModel));
    commitedModel = std::move(redoModels.front());
    redoModels.pop_front();
    if (!canRedo()) {
        canRedoChanged();
    }
    canUndoChanged();
    repaint();
}

double roundUpToMultiple(double x, double multiple) {
    return ceil(x / multiple) * multiple;
}

void Ui::ModelWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);

    painter.setPen(Qt::black);

    FigurePainter fpainter(painter, scaler);
    if (gridStep() > 0) {
        int step = gridStep();
        // Calculating visible area
        Point p1 = scaler(QPointF(0, 0));
        Point p2 = scaler(QPointF(width(), height()));
        if (p1.x > p2.x) { std::swap(p1.x, p2.x); }
        if (p1.y > p2.y) { std::swap(p1.y, p2.y); }

        // Finding starting point for the grid
        p1.x = roundUpToMultiple(p1.x, step);
        p1.y = roundUpToMultiple(p1.y, step);

        // Drawing
        QPen pen(QColor(192, 192, 192, 255));
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        for (int x = p1.x; x <= p2.x; x += step) {
            painter.drawLine(scaler(Point(x, p1.y)), scaler(Point(x, p2.y)));
        }
        for (int y = p1.y; y <= p2.y; y += step) {
            painter.drawLine(scaler(Point(p1.x, y)), scaler(Point(p2.x, y)));
        }
    }

    Model copiedModel;
    Model &modelToDraw = lastTrack.empty() ? commitedModel : (copiedModel = commitedModel);
    PFigure modified = recognize(lastTrack, modelToDraw);
    for (PFigure fig : modelToDraw) {
        if (fig == modified) {
            painter.setPen(Qt::magenta);
        } else if (fig == modelToDraw.selectedFigure) {
            painter.setPen(Qt::blue);
        } else {
            painter.setPen(Qt::black);
        }
        fig->visit(fpainter);
    }

    QPen pen(QColor(255, 0, 0, 16));
    pen.setWidth(3);
    painter.setPen(pen);
    drawTrack(painter, scaler, lastTrack);
    for (const Track &track : visibleTracks) {
        drawTrack(painter, scaler, track);
    }
}

void Ui::ModelWidget::mousePressEvent(QMouseEvent *event) {
    lastTrack = Track();
    if (event->modifiers().testFlag(Qt::ShiftModifier)) {
        mouseAction = MouseAction::ViewpointMove;
    } else {
        mouseAction = MouseAction::TrackActive;
        lastTrack.points.push_back(scaler(event->pos()));
    }
    repaint();
}
void Ui::ModelWidget::mouseMoveEvent(QMouseEvent *event) {
    if (mouseAction == MouseAction::TrackActive) {
        lastTrack.points.push_back(scaler(event->pos()));
        repaint();
    }
}
void Ui::ModelWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (mouseAction == MouseAction::None) {
        return;
    }
    if (mouseAction == MouseAction::ViewpointMove) {
        mouseAction = MouseAction::None;
        repaint();
        return;
    }
    assert(mouseAction == MouseAction::TrackActive);
    mouseAction = MouseAction::None;
    lastTrack.points.push_back(scaler(event->pos()));
    Model previousModel = commitedModel;
    PFigure modifiedFigure = recognize(lastTrack, commitedModel);
    if (_gridStep > 0 && modifiedFigure) {
        GridAlignLayouter layouter(_gridStep);
        layouter.updateLayout(commitedModel, modifiedFigure);
        commitedModel.recalculate();
    }

    visibleTracks.push_back(lastTrack);
    auto iterator = --visibleTracks.end();
    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, [this, iterator, timer]() {
        visibleTracks.erase(iterator);
        delete timer;
        repaint();
    });
    timer->setInterval(1500);
    timer->setSingleShot(true);
    timer->start();

    lastTrack = Track();
    if (modifiedFigure) {
        previousModels.push_back(previousModel);
        redoModels.clear();
        emit canUndoChanged();
        emit canRedoChanged();
    }
    repaint();
}
void Ui::ModelWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && mouseAction == MouseAction::TrackActive) {
        lastTrack = Track();
        mouseAction = MouseAction::None;
        repaint();
    }
    if (event->key() == Qt::Key_Delete) {
        if (commitedModel.selectedFigure) {
            previousModels.push_back(commitedModel);
            redoModels.clear();
            for (auto it = commitedModel.begin(); it != commitedModel.end(); it++) {
                if (*it == commitedModel.selectedFigure) {
                    commitedModel.removeFigure(it);
                    break;
                }
            }
            emit canUndoChanged();
            emit canRedoChanged();
            repaint();
        }
    }
}
