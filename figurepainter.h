#ifndef FIGURESPAINTER_H
#define FIGURESPAINTER_H

#include "model.h"
#include <QPainter>
#include <QFile>
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

    virtual void accept(figures::Segment &segm) {
        painter.drawLine(scaler(segm.getA()), scaler(segm.getB()));
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
        QRectF rect(scaler(fig.getBoundingBox().leftDown),
                    scaler(fig.getBoundingBox().rightUp)
                   );
        painter.drawEllipse(rect);
        drawLabel(fig);
    }

    virtual void accept(figures::Rectangle &fig) {
        QRectF rect(scaler(fig.getBoundingBox().leftDown),
                    scaler(fig.getBoundingBox().rightUp)
                   );
        painter.drawRect(rect);
        drawLabel(fig);
    }

private:
    QPainter &painter;
    Scaler scaler;

    void drawArrow(const Point &a, const Point &b) {
        const double BRANCH_ANGLE = 25 * PI / 180.0;
        const int ARROW_LENGTH = 12 * scaler.scaleFactor;
        Point dir = b - a;
        double ang = atan2(dir.y, dir.x);
        QPointF start = scaler(a);
        for (int k = -1; k <= 1; k += 2) {
            double curAng = ang + BRANCH_ANGLE * k;
            QPointF end = start;
            end.setX(end.x() + cos(curAng) * ARROW_LENGTH);
            end.setY(end.y() + sin(curAng) * ARROW_LENGTH);
            painter.drawLine(start, end);
        }
    }

    void drawLabel(const figures::BoundedFigure &figure) {
        const std::string &label = figure.label();
        if (label.empty()) { return; }

        const double REQUIRED_GAP = 0.8;
        const int BIG_SIZE = 1e6; // used in QFontMetrics call when there are no limits

        QString text = QString::fromStdString(label);
        BoundingBox box = figure.getBoundingBox();
        // Here we have some mix-up with what's 'up' and what's 'down' (#48)
        QPointF leftUp = scaler(box.leftDown);
        QPointF rightDown = scaler(box.rightUp);

        QFontMetrics metrics = painter.fontMetrics();
        QPointF baseRectSize = rightDown - leftUp;
        QRect baseRect(QPoint(), QPoint((int)baseRectSize.x(), (int)baseRectSize.y()));
        QRectF rect = metrics.boundingRect(baseRect, Qt::AlignCenter, text);
        if (rect.width() <= REQUIRED_GAP * baseRect.width() && rect.height() <= REQUIRED_GAP * baseRect.height()) {
            rect.translate(leftUp);
            painter.drawText(rect, Qt::AlignCenter, text);
            return;
        }

        baseRect.setSize(QSize(BIG_SIZE, BIG_SIZE));
        rect = metrics.boundingRect(baseRect, Qt::AlignHCenter, text);
        rect.translate(QPointF(baseRectSize.x() / 2 - BIG_SIZE / 2, 0));
        rect.translate(scaler(box.leftUp())); // another mix-up (#48)
        painter.drawText(rect, Qt::AlignLeft, text);
    }
};

class FigureSvgPainter : public FigureVisitor {
public:
    FigureSvgPainter(std::ostream &out) : out(out) {}

    void printHeader() {
        QFile resource(":/svg-data/header.svg");
        resource.open(QIODevice::ReadOnly);
        out << resource.readAll().toStdString();
    }
    void printFooter() {
        QFile resource(":/svg-data/footer.svg");
        resource.open(QIODevice::ReadOnly);
        out << resource.readAll().toStdString();
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
        accept((figures::Segment &)segm);
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
