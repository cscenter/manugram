#include "figurepainter.h"
#include "textpainter.h"

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

void FigurePainter::accept(figures::Curve &fig) {
    for (size_t i = 0; i + 1 < fig.points.size(); i++) {
        painter.drawLine(scaler(fig.points[i]), scaler(fig.points[i + 1]));
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

std::vector<std::pair<Point, Point>> generateArrow(const Point &a, const Point &b) {
    std::vector<std::pair<Point, Point>> result;
    Point dir = b - a;
    dir = dir * (ARROW_LENGTH / dir.length());
    for (int k : { -1, 1 }) {
        Point branch = dir;
        branch.rotateBy(k * ARROW_BRANCH_ANGLE * PI / 180.0);
        result.push_back({ a, a + branch });
    }
    return result;
}

void FigurePainter::drawArrow(const Point &a, const Point &b) {
    for (auto segment : generateArrow(a, b)) {
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
    out << "\"/>";
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
