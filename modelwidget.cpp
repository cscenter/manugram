#include "model.h"
#include "modelwidget.h"
#include <QPainter>

Ui::ModelWidget::ModelWidget(QWidget *parent) :
    QWidget(parent)
{
}

void Ui::ModelWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);
    painter.setPen(Qt::black);
    for (PFigure fig : model) {
        figures::Segment *segm = dynamic_cast<figures::Segment*>(fig.get());
        if (segm) {
            painter.drawLine(QPoint(segm->getA().x, segm->getA().y), QPoint(segm->getB().x, segm->getB().y));
        }
    }
}
