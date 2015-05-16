#include "figurepainter.h"
#include "textpainter.h"

std::vector<std::pair<Point, Point>> generateArrow(const Point &end, const Point &start);

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

Point getControlPoint(Point a, Point b, Point c) {
    if (a == b) {
        return c;
    }
    Point side1 = a - b, side2 = c - b;
    double needLength = side2.length() * 0.25;
    side1 = side1 * (1.0 / side1.length());
    side2 = side2 * (1.0 / side2.length());
    Point perp = (side1 + side2) * 0.5;
    perp = perp * (1.0 / perp.length());
    if (Point::crossProduct(perp, side2) >= 0) {
        perp.rotateBy(PI / 2);
    } else {
        perp.rotateBy(-PI / 2);
    }
    return b + perp * needLength;
}

void FigurePainter::accept(figures::Curve &fig) {
    fig.selfCheck();
    for (size_t i = 0; i + 1 < fig.points.size(); i++) {
        Point a = fig.points[i], b = fig.points[i + 1];
        if (a == b) {
            continue;
        }
        Point controlA = b, controlB = a;
        if (i > 0 && !fig.isStop[i]) {
            controlA = getControlPoint(fig.points[i - 1], a, b);
        }
        if (i + 2 < fig.points.size() && !fig.isStop[i + 1]) {
            controlB = getControlPoint(fig.points[i + 2], b, a);
        }

        QPainterPath path;
        path.moveTo(scaler(a));
        path.cubicTo(scaler(controlA), scaler(controlB), scaler(b));
        painter.drawPath(path);
        //painter.drawLine(scaler(a), scaler(b));
        if (fig.arrowBegin[i]) {
            for (auto segm : generateArrow(a, controlA)) {
                painter.drawLine(scaler(segm.first), scaler(segm.second));
            }
        }
        if (fig.arrowEnd[i]) {
            for (auto segm : generateArrow(b, controlB)) {
                painter.drawLine(scaler(segm.first), scaler(segm.second));
            }
        }
    }
    drawLabel(fig);
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

const double ARROW_BRANCH_ANGLE = 25;
const int ARROW_LENGTH = 12;

std::vector<std::pair<Point, Point>> generateArrow(const Point &end, const Point &start) {
    std::vector<std::pair<Point, Point>> result;
    Point dir = start - end;
    if (dir == Point()) {
        return result;
    }
    dir = dir * (ARROW_LENGTH / dir.length());
    for (int k : { -1, 1 }) {
        Point branch = dir;
        branch.rotateBy(k * ARROW_BRANCH_ANGLE * PI / 180.0);
        result.push_back({ end, end + branch });
    }
    return result;
}

void FigurePainter::drawArrow(const Point &end, const Point &start) {
    for (auto segment : generateArrow(end, start)) {
        painter.drawLine(scaler(segment.first), scaler(segment.second));
    }
}

void FigurePainter::drawLabel(Figure &figure) {
    const std::string &label = figure.label();
    if (label.empty()) { return; }

    TextPosition position = getTextPosition(figure);
    QRectF rect(QPointF(), QSizeF(position.width * scaler.scaleFactor, position.height * scaler.scaleFactor));
    painter.save();
    painter.translate(scaler(position.leftUp));
    painter.rotate(position.rotation);
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, QString::fromStdString(label));
    painter.restore();
}

void FigureSvgPainter::printHeader() {
    QFile resource(":/svg-data/header.svg");
    resource.open(QIODevice::ReadOnly);
    QString header = resource.readAll();
    header.replace("{{markerSize}}", QString::number(2 * ARROW_LENGTH));
    header.replace("{{markerCenter}}", QString::number(ARROW_LENGTH));

    for (int id = 0; id < 2; id++) {
        QString path = "";
        Point start(id == 0 ? 0 : 2 * ARROW_LENGTH, ARROW_LENGTH);
        Point end(ARROW_LENGTH, ARROW_LENGTH);

        for (auto segment : generateArrow(end, start)) {
            Point a = segment.first, b = segment.second;
            path += QString("M%1, %2 ").arg(a.x).arg(a.y);
            path += QString("L%1, %2 ").arg(b.x).arg(b.y);
        }
        header.replace(
                    id == 0 ? "{{markerDirectPath}}" : "{{markerReversePath}}",
                    path
                    );
    }
    out << header.toStdString();

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
    drawLabel(segm);
}
void FigureSvgPainter::accept(figures::SegmentConnection &segm) {
    accept((figures::Segment &)segm);
}

void FigureSvgPainter::accept(figures::Curve &fig) {
    out << "<polyline points=\"";
    for (Point p : fig.points) {
        out << " " << p.x << "," << p.y;
    }
    out << "\"/>\n";
    for (size_t i = 0; i < fig.arrowBegin.size(); i++) {
        std::vector<std::pair<Point, Point>> arrows[] = {
            generateArrow(fig.points[i], fig.points[i + 1]),
            generateArrow(fig.points[i + 1], fig.points[i]),
        };
        if (!fig.arrowBegin[i]) { arrows[0].clear(); }
        if (!fig.arrowEnd[i]) { arrows[1].clear(); }
        for (auto arrow : arrows) if (!arrow.empty()) {
            for (auto segm : arrow) {
                out << "  <path d=\"M" << segm.first.x << "," << segm.first.y << " L" << segm.second.x << "," << segm.second.y << "\"/>\n";
            }
        }
    }
    drawLabel(fig);
}

void FigureSvgPainter::accept(figures::Ellipse &fig) {
    BoundingBox box = fig.getBoundingBox();
    out << "<ellipse cx=\"" << box.center().x << "\" cy=\"" << box.center().y << "\" rx=\"" << box.width() / 2 << "\" ry=\"" << box.height() / 2 << "\" />\n";
    drawLabel(fig);
}

void FigureSvgPainter::accept(figures::Rectangle &fig) {
    BoundingBox box = fig.getBoundingBox();
    out << "<rect x=\"" << box.leftUp.x << "\" y=\"" << box.leftUp.y << "\" width=\"" << box.width() << "\" height=\"" << box.height() << "\" />\n";
    drawLabel(fig);
}

std::vector<std::string> getLines(const std::string &data) {
    std::vector<std::string> result;
    std::stringstream s;
    s << data;
    std::string line;
    while (getline(s, line)) {
        result.push_back(line);
    }
    return result;
}

void FigureSvgPainter::drawLabel(Figure &figure) {
    const std::string &label = figure.label();
    if (label.empty()) { return; }

    std::vector<std::string> lines = getLines(label);

    TextPosition position = getTextPosition(figure);
    Point offset(0, position.height);
    offset.rotateBy(position.rotation * PI / 180);

    Point leftDown = position.leftUp + offset;

    out << "<text x=\"" << leftDown.x << "\" y=\"" << leftDown.y << "\"";
    if (fabs(position.rotation) > 1e-6) {
        out << " transform=\"rotate(" << position.rotation << " " << leftDown.x << " " << leftDown.y << ")\"";
    }
    out << ">";
    if (lines.size() == 1) {
        out << lines[0];
    } else {
        out << "\n";
        // We ignore rotation here, because it's already performed in <text>
        Point step(0, position.height / lines.size());
        Point current = leftDown - step * lines.size();
        for (auto line : lines)  {
            current += step;
            out << "<tspan x=\"" << current.x << "\" y=\"" << current.y << "\">" << line << "</tspan>\n";
        }
    }
    out << "</text>";
}
