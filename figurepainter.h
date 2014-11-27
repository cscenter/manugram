#ifndef FIGURESPAINTER_H
#define FIGURESPAINTER_H

#include "model.h"
#include <QPainter>
#include <cmath>

class FigurePainter : public FigureVisitor {
public:
    FigurePainter(QPainter &painter) : painter(painter) {}
    virtual ~FigurePainter() {}

    virtual void accept(figures::Segment &segm) {
        painter.drawLine(scale(segm.getA()), scale(segm.getB()));
        if (segm.getArrowedA()) {
            drawArrow(segm.getA(), segm.getB());
        }
        if (segm.getArrowedB()) {
            drawArrow(segm.getB(), segm.getA());
        }
    }
    virtual void accept(figures::SegmentConnection &segm) {
        accept(static_cast<figures::Segment &>(segm));
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

    QPoint scale(const Point &p) {
        return QPoint(p.x, p.y);
    }

private:
    QPainter &painter;

    void drawArrow(const Point &a, const Point &b) {
        const double PI = atan(1.0) * 4;
        const double BRANCH_ANGLE = 25 * PI / 180.0;
        const int ARROW_LENGTH = 12;
        Point dir = b - a;
        double ang = atan2(dir.y, dir.x);
        QPoint start = scale(a);
        for (int k = -1; k <= 1; k += 2) {
            double curAng = ang + BRANCH_ANGLE * k;
            QPoint end = start;
            end.setX(end.x() + cos(curAng) * ARROW_LENGTH);
            end.setY(end.y() + sin(curAng) * ARROW_LENGTH);
            painter.drawLine(start, end);
        }
    }
};

#endif // FIGURESPAINTER_H
