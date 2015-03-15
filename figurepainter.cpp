#include "figurepainter.h"

void FigurePainter::accept(figures::Segment &segm) {
    painter.drawLine(scaler(segm.getA()), scaler(segm.getB()));
    if (segm.getArrowedA()) {
        drawArrow(segm.getA(), segm.getB());
    }
    if (segm.getArrowedB()) {
        drawArrow(segm.getB(), segm.getA());
    }
    drawLabel(segm);
}
void FigurePainter::accept(figures::SegmentConnection &segm) {
    accept(static_cast<figures::Segment &>(segm));
}

void FigurePainter::accept(figures::Ellipse &fig) {
    QRectF rect(scaler(fig.getBoundingBox().leftUp),
                scaler(fig.getBoundingBox().rightDown)
               );
    painter.drawEllipse(rect);
    drawLabel(fig);
}

void FigurePainter::accept(figures::Rectangle &fig) {
    QRectF rect(scaler(fig.getBoundingBox().leftUp),
                scaler(fig.getBoundingBox().rightDown)
               );
    painter.drawRect(rect);
    drawLabel(fig);
}

void FigurePainter::drawArrow(const Point &a, const Point &b) {
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

void FigurePainter::drawLabel(const figures::Segment &figure) {
    const std::string &label = figure.label();
    if (label.empty()) { return; }

    QString text = QString::fromStdString(label);
    QPointF a = scaler(figure.getA());
    QPointF b = scaler(figure.getB());
    if (a.x() > b.x()) {
        std::swap(a, b);
    }
    QPointF direction = b - a;

    painter.save();
    double angle = 0;
    if (direction.manhattanLength() > 10) {
        angle = atan2(direction.y(), direction.x()) * 180 / PI;
    }
    const double MAX_ANGLE = 30;
    const int BIG_SIZE = 1e6; // used in QFontMetrics call when there are no limits
    int length = (int)QVector2D(direction).length();

    if (fabs(angle) <= MAX_ANGLE) {
        painter.translate(a);
        painter.rotate(angle);
        painter.drawText(
                    QRect(QPoint(-BIG_SIZE / 2, -BIG_SIZE), QPoint(BIG_SIZE / 2 + length, 0)),
                    Qt::AlignBottom | Qt::AlignHCenter,
                    text
                    );
    } else {
        painter.translate((a + b) / 2);
        painter.drawText(
                    QRect(QPoint(10, -BIG_SIZE / 2), QPoint(10 + BIG_SIZE, BIG_SIZE / 2)),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    text
                    );
    }

    painter.restore();
}

void FigurePainter::drawLabel(const figures::BoundedFigure &figure) {
    const std::string &label = figure.label();
    if (label.empty()) { return; }

    const double REQUIRED_GAP = 0.8;
    const int BIG_SIZE = 1e6; // used in QFontMetrics call when there are no limits

    QString text = QString::fromStdString(label);
    BoundingBox box = figure.getBoundingBox();
    QPointF leftUp = scaler(box.leftUp);
    QPointF rightDown = scaler(box.rightDown);

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
    rect.translate(scaler(box.leftDown()));
    painter.drawText(rect, Qt::AlignLeft, text);
}

void FigureSvgPainter::printHeader() {
    QFile resource(":/svg-data/header.svg");
    resource.open(QIODevice::ReadOnly);
    out << resource.readAll().toStdString();
}
void FigureSvgPainter::printFooter() {
    QFile resource(":/svg-data/footer.svg");
    resource.open(QIODevice::ReadOnly);
    out << resource.readAll().toStdString();
}

void FigureSvgPainter::accept(figures::Segment &segm) {
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
void FigureSvgPainter::accept(figures::SegmentConnection &segm) {
    accept((figures::Segment &)segm);
}

void FigureSvgPainter::accept(figures::Ellipse &fig) {
    BoundingBox box = fig.getBoundingBox();
    out << "<ellipse cx=\"" << box.center().x << "\" cy=\"" << box.center().y << "\" rx=\"" << box.width() / 2 << "\" ry=\"" << box.height() / 2 << "\" />\n";
}

void FigureSvgPainter::accept(figures::Rectangle &fig) {
    BoundingBox box = fig.getBoundingBox();
    out << "<rect x=\"" << box.leftUp.x << "\" y=\"" << box.leftUp.y << "\" width=\"" << box.width() << "\" height=\"" << box.height() << "\" />\n";
}
