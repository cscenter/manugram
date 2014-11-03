#include "model.h"
#include "modelwidget.h"
#include "figurepainter.h"
#include "recognition.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <iostream>

Ui::ModelWidget::ModelWidget(QWidget *parent) :
    QWidget(parent) {
}

void Ui::ModelWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);
    painter.setPen(Qt::black);

    FigurePainter fpainter(painter);
    for (PFigure fig : model) {
        fig->visit(fpainter);
    }

    painter.setPen(Qt::red);
    for (size_t i = 0; i + 1 < lastTrack.size(); i++) {
        painter.drawLine(fpainter.scale(lastTrack[i]), fpainter.scale(lastTrack[i + 1]));
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
    recognize(lastTrack, model);
    lastTrack = Track();
    repaint();
}
