#include "model.h"
#include "modelwidget.h"
#include "figurepainter.h"
#include "recognition.h"
#include "layouting.h"
#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
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

double roundDownToMultiple(double x, double multiple) {
    return floor(x / multiple) * multiple;
}

void Ui::ModelWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);

    QFont font;
    font.setPointSizeF(10 * scaler.scaleFactor);
    painter.setFont(font);

    QPen pen(Qt::black);
    pen.setWidthF(scaler.scaleFactor);
    painter.setPen(pen);

    FigurePainter fpainter(painter, scaler);
    if (gridStep() > 0) {
        int step = gridStep();
        // Calculating visible area
        Point p1 = scaler(QPointF(0, 0));
        Point p2 = scaler(QPointF(width(), height()));
        if (p1.x > p2.x) { std::swap(p1.x, p2.x); }
        if (p1.y > p2.y) { std::swap(p1.y, p2.y); }

        // Finding starting point for the grid
        p1.x = roundDownToMultiple(p1.x, step);
        p1.y = roundDownToMultiple(p1.y, step);

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
            pen.setColor(Qt::magenta);
        } else if (fig == modelToDraw.selectedFigure) {
            pen.setColor(Qt::blue);
        } else {
            pen.setColor(Qt::black);
        }
        painter.setPen(pen);
        fig->visit(fpainter);
    }

    pen.setColor(QColor(255, 0, 0, 16));
    pen.setWidth(3 * scaler.scaleFactor);
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
        viewpointMoveStart = event->pos();
        viewpointMoveOldScaler = scaler;
        setCursor(Qt::ClosedHandCursor);
    } else {
        mouseAction = MouseAction::TrackActive;
        lastTrack.points.push_back(scaler(event->pos()));
    }
    repaint();
}
void Ui::ModelWidget::mouseMoveEvent(QMouseEvent *event) {
    if (mouseAction == MouseAction::ViewpointMove) {
        scaler = viewpointMoveOldScaler;
        scaler.zeroPoint = scaler.zeroPoint + scaler(viewpointMoveStart) - scaler(event->pos());
        repaint();
    }
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
        setCursor(Qt::ArrowCursor);
        scaler = viewpointMoveOldScaler;
        scaler.zeroPoint = scaler.zeroPoint + scaler(viewpointMoveStart) - scaler(event->pos());
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

void Ui::ModelWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    figures::PBoundedFigure figure = std::dynamic_pointer_cast<figures::BoundedFigure>(commitedModel.selectedFigure);
    if (figure && figure->getApproximateDistanceToBorder(scaler(event->pos())) < 10) {
        bool ok;
        QString newLabel = QInputDialog::getMultiLineText(this, "Figure label", "Specify new figure label",
                                                 QString::fromStdString(figure->label()),
                                                          &ok);
        if (ok) {
            previousModels.push_back(commitedModel);
            redoModels.clear();

            figure->setLabel(newLabel.toStdString());

            emit canUndoChanged();
            emit canRedoChanged();
            repaint();
        }
    }
}
