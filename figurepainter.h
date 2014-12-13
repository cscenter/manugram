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

class FigureSvgPainter : public FigureVisitor {
public:
    FigureSvgPainter(std::ostream &out) : out(out) {}

    virtual void accept(figures::Segment &segm) {
        Point a = segm.getA();
        Point b = segm.getB();
        out << "<line x1=\"" << a.x << "\" y1=\"" << a.y << "\" x2=\"" << b.x << "\" y2=\"" << b.y << "\" style=\"";
        if (segm.getArrowedA()) {
            out << "marked-start: url(#markerArrow);";
        }
        if (segm.getArrowedB()) {
            out << "marked-end: url(#markerArrow);";
        }
        out << "\"/>\n";
    }
    virtual void accept(figures::SegmentConnection &segm) {
        accept((figures::Segment&)segm);
    }

    virtual void accept(figures::Ellipse &fig) {
        BoundingBox box = fig.getBoundingBox();
        out << "<ellipse cx=\"" << box.center().x << "\" cy=\"" << box.center().y << "\" rx=\"" << box.width() / 2 << "\" ry=\"" << box.height() / 2 << "\" />\n";
    }

    virtual void accept(figures::Rectangle &fig) {
        BoundingBox box = fig.getBoundingBox();
        out << "<rect x=\"" << box.leftDown.x << "\" y=\"" << box.leftDown.y << "\" width=\"" << box.width() << "\" height=\"" << box.height() << "\" />\n";
    }

private:
    std::ostream &out;
};

#endif // FIGURESPAINTER_H
