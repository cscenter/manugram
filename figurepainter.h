#ifndef FIGURESPAINTER_H
#define FIGURESPAINTER_H

#include "model.h"
#include <QPainter>

class FigurePainter : public FigureVisitor {
public:
    FigurePainter(QPainter &painter) : painter(painter) {}
    virtual ~FigurePainter() {}

    virtual void accept(figures::Segment &segm) {
        painter.drawLine(scale(segm.getA()), scale(segm.getB()));
    }

    virtual void accept(figures::Ellipse &fig) {
        QRect rect(scale(fig.getBoundingBox().leftDown),
                   scale(fig.getBoundingBox().rightUp)
                  );
        painter.drawEllipse(rect);
    }

    virtual void accept(figures::Rectangle &fig) {
        QRect rect(scale(fig.getBoundingBox().leftDown),
                   scale(fig.getBoundingBox().rightUp)
                  );
        painter.drawRect(rect);
    }

    QPoint scale(Point p) {
        return QPoint(p.x, p.y);
    }

private:
    QPainter &painter;
};

#endif // FIGURESPAINTER_H
