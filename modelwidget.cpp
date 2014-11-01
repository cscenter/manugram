#include "model.h"
#include "modelwidget.h"
#include "figurepainter.h"
#include <QPainter>

Ui::ModelWidget::ModelWidget(QWidget *parent) :
    QWidget(parent)
{
}

void Ui::ModelWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);
    painter.setPen(Qt::black);

    FigurePainter fpainter(painter);
    for (PFigure fig : model) {
        fig->visit(fpainter);
    }
}
