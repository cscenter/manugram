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
        QRectF rect(scale(fig.getBoundingBox().leftDown),
                   scale(fig.getBoundingBox().rightUp)
                  );
        painter.drawEllipse(rect);
    }

    virtual void accept(figures::Rectangle &fig) {
        QRectF rect(scale(fig.getBoundingBox().leftDown),
                   scale(fig.getBoundingBox().rightUp)
                  );
        painter.drawRect(rect);
    }

    QPointF scale(const Point &p) {
        return QPointF(p.x, p.y);
    }
    Point unscale(const QPointF &p) {
        return Point(p.x(), p.y());
    }

private:
    QPainter &painter;

    void drawArrow(const Point &a, const Point &b) {
        const double BRANCH_ANGLE = 25 * PI / 180.0;
        const int ARROW_LENGTH = 12;
        Point dir = b - a;
        double ang = atan2(dir.y, dir.x);
        QPointF start = scale(a);
        for (int k = -1; k <= 1; k += 2) {
            double curAng = ang + BRANCH_ANGLE * k;
            QPointF end = start;
            end.setX(end.x() + cos(curAng) * ARROW_LENGTH);
            end.setY(end.y() + sin(curAng) * ARROW_LENGTH);
            painter.drawLine(start, end);
        }
    }
};

class FigureSvgPainter : public FigureVisitor {
public:
    FigureSvgPainter(std::ostream &out) : out(out) {}

    void printHeader() {
        out << R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)" "\n"
               R"(<svg version="1.1")" "\n"
               R"(    baseProfile="full"  xmlns="http://www.w3.org/2000/svg")" "\n"
               R"(    xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:ev="http://www.w3.org/2001/xml-events")" "\n"
               R"(    fill="none" stroke="black" stroke-width="1">)" "\n";
        out << R"(<marker id="markerArrow" markerWidth="12" markerHeight="24" refX="12" refY="12" orient="auto">)" "\n"
               R"(    <path d="M12,12 L1.1243065555602004410893681189482,6.9285808591116067657562581242272 M12,12 L1.1243065555602004410893681189482,17.071419140888393234243741875773"/>)" "\n"
               R"(</marker>)" "\n";
        out << R"(<marker id="markerReverseArrow" markerWidth="12" markerHeight="24" refX="0" refY="12" orient="auto">)" "\n"
               R"(    <path d="M0,12 L10.875693444439799558910631881052,6.9285808591116067657562581242272 M0,12 L10.875693444439799558910631881052,17.071419140888393234243741875773"/>)" "\n"
               R"(</marker>)" "\n";
    }
    void printFooter() {
        out << "</svg>\n";
    }

    virtual void accept(figures::Segment &segm) {
        Point a = segm.getA();
        Point b = segm.getB();
        out << "<line x1=\"" << a.x << "\" y1=\"" << a.y << "\" x2=\"" << b.x << "\" y2=\"" << b.y << "\"";
        if (segm.getArrowedA()) {
            out << " marker-start=\"url(#markerReverseArrow)\"";
        }
        if (segm.getArrowedB()) {
            out << " marker-end=\"url(#markerArrow)\"";
        }
        out << "/>\n";
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
