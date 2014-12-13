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
    QWidget(parent) {
}

void drawTrack(QPainter &painter, FigurePainter &fpainter, const Track &track) {
    for (size_t i = 0; i + 1 < track.size(); i++) {
        painter.drawLine(fpainter.scale(track[i]), fpainter.scale(track[i + 1]));
    }
}

void Ui::ModelWidget::setModel(Model model) {
    commitedModel = std::move(model);
}
Model& Ui::ModelWidget::getModel() {
    return commitedModel;
}

void Ui::ModelWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);
    painter.setPen(Qt::black);

    FigurePainter fpainter(painter);
    Model modelToDraw = commitedModel;
    recognize(lastTrack, modelToDraw);
    for (PFigure fig : modelToDraw) {
        fig->visit(fpainter);
    }

    QPen pen(QColor(255, 0, 0, 64));
    pen.setWidth(3);
    painter.setPen(pen);
    drawTrack(painter, fpainter, lastTrack);
    for (const Track &track : visibleTracks) {
        drawTrack(painter, fpainter, track);
    }
}

void Ui::ModelWidget::mousePressEvent(QMouseEvent *event) {
    lastTrack = Track();
    lastTrack.points.push_back(Point(event->pos().x(), event->pos().y()));
    repaint();
}
void Ui::ModelWidget::mouseMoveEvent(QMouseEvent *event) {
    lastTrack.points.push_back(Point(event->pos().x(), event->pos().y()));
    repaint();
}
void Ui::ModelWidget::mouseReleaseEvent(QMouseEvent *event) {
    lastTrack.points.push_back(Point(event->pos().x(), event->pos().y()));
    recognize(lastTrack, commitedModel);

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
    repaint();
}
