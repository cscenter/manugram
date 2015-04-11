#ifndef FIGURESPAINTER_H
#define FIGURESPAINTER_H

#include "model.h"
#include <QPainter>
#include <QFile>
#include <QVector2D>
#include <cmath>

struct Scaler {
    Scaler(
        const Point &zeroPoint = Point(),
        const double scaleFactor = 1.0
    )
        : zeroPoint(zeroPoint)
        , scaleFactor(scaleFactor)
    {}

    QPointF operator()(const Point &p) {
        return QPointF((p.x - zeroPoint.x) * scaleFactor, (p.y - zeroPoint.y) * scaleFactor);
    }
    Point operator()(const QPointF &p) {
        return Point(p.x() / scaleFactor + zeroPoint.x, p.y() / scaleFactor + zeroPoint.y);
    }

    void scaleWithFixedPoint(const Point &fixed, double factor) {
        zeroPoint.x = (fixed.x * (factor - 1) + zeroPoint.x) / factor;
        zeroPoint.y = (fixed.y * (factor - 1) + zeroPoint.y) / factor;
        scaleFactor *= factor;
    }

    Point zeroPoint;
    double scaleFactor;
};

class FigurePainter : public FigureVisitor {
public:
    FigurePainter(
        QPainter &painter,
        const Scaler &scaler = Scaler()
    )
        : painter(painter)
        , scaler(scaler)
    {}
    virtual ~FigurePainter() {}

    virtual void accept(figures::Segment &segm) override;
    virtual void accept(figures::SegmentConnection &segm) override;
    virtual void accept(figures::Curve &fig) override;
    virtual void accept(figures::Ellipse &fig) override;
    virtual void accept(figures::Rectangle &fig) override;

private:
    QPainter &painter;
    Scaler scaler;

    void drawArrow(const Point &end, const Point &start);
    void drawLabel(Figure &figure);
};

class FigureSvgPainter : public FigureVisitor {
public:
    FigureSvgPainter(std::ostream &out) : out(out) {}

    void printHeader();
    void printFooter();

    virtual void accept(figures::Segment &segm) override;
    virtual void accept(figures::SegmentConnection &segm) override;
    virtual void accept(figures::Curve &fig) override;
    virtual void accept(figures::Ellipse &fig) override;
    virtual void accept(figures::Rectangle &fig) override;

private:
    std::ostream &out;
    void drawLabel(Figure &figure);
};

#endif // FIGURESPAINTER_H
