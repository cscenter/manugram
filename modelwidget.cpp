#include "model.h"
#include "modelwidget.h"
#include "figurepainter.h"
#include "recognition.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QTimer>
#include <iostream>

Ui::ModelWidget::ModelWidget(QWidget *parent) :
    QWidget(parent), trackIsCancelled(false) {
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
}

void drawTrack(QPainter &painter, FigurePainter &fpainter, const Track &track) {
    for (size_t i = 0; i + 1 < track.size(); i++) {
        painter.drawLine(fpainter.scale(track[i]), fpainter.scale(track[i + 1]));
    }
}

void Ui::ModelWidget::setModel(Model model) {
    commitedModel = std::move(model);
    previousModels.clear();
    emit canUndoChanged();
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
    commitedModel = std::move(previousModels.back());
    previousModels.pop_back();
    if (!canUndo()) {
        emit canUndoChanged();
    }
    repaint();
}


void Ui::ModelWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);
    painter.setPen(Qt::black);

    FigurePainter fpainter(painter);
    Model modelToDraw = commitedModel;
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
    drawTrack(painter, fpainter, lastTrack);
    for (const Track &track : visibleTracks) {
        drawTrack(painter, fpainter, track);
    }
}

void Ui::ModelWidget::mousePressEvent(QMouseEvent *event) {
    lastTrack = Track();
    trackIsCancelled = false;
    lastTrack.points.push_back(Point(event->pos().x(), event->pos().y()));
    repaint();
}
void Ui::ModelWidget::mouseMoveEvent(QMouseEvent *event) {
    lastTrack.points.push_back(Point(event->pos().x(), event->pos().y()));
    repaint();
}
void Ui::ModelWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (trackIsCancelled) {
        return;
    }
    lastTrack.points.push_back(Point(event->pos().x(), event->pos().y()));
    previousModels.push_back(commitedModel); // if nothing is changed, we will pop it back
    bool somethingChanged = !!recognize(lastTrack, commitedModel);

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
    if (somethingChanged) {
        emit canUndoChanged();
    } else {
        previousModels.pop_back();
    }
    repaint();
}
void Ui::ModelWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        lastTrack = Track();
        trackIsCancelled = true;
        repaint();
    }
}
