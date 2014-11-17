#include "model.h"
#include <string>

double figures::Segment::getDistanceToBorder(const Point &p) {
    double res = std::min((p - a).length(), (p - b).length());
    // Please note that here we do not care about floatint-point error,
    // as if 'start' falls near 'a' or 'b', we've already calculated it
    if (Point::dotProduct(p - a, b - a) >= 0
            && Point::dotProduct(p - b, a - b) >= 0) {
        res = std::min(res, getDistanceToLine(p));
    }
    return res;
}

double figures::Segment::getDistanceToLine(const Point &p) {
    // Coefficients of A*x + B*y + C = 0
    double A = b.y - a.y;
    double B = a.x - b.x;
    double C = -A * a.x - B * a.y;

    // normalization of the equation
    double D = sqrt(A * A + B * B);
    if (fabs(D) < 1e-8) { return HUGE_VAL; } // degenerate case

    A /= D; B /= D; C /= D;
    return fabs(A * p.x + B * p.y + C);
}

double figures::Rectangle::getDistanceToBorder(const Point &p) {
    bool inX = box.leftDown.x <= p.x && p.x <= box.rightUp.x;
    bool inY = box.leftDown.y <= p.y && p.y <= box.rightUp.y;
    double res = HUGE_VAL;
    if (inX) { // Point lies in a vertical bar bounded by Left and Right
        res = std::min(res, fabs(p.y - box.leftDown.y));
        res = std::min(res, fabs(p.y - box.rightUp.y));
    }
    if (inY) { // Point lies in a horizontal bar
        res = std::min(res, fabs(p.x - box.leftDown.x));
        res = std::min(res, fabs(p.x - box.rightUp.x));
    }
    if (!inX && !inY) {
        res = std::min(res, (p - box.leftDown).length());
        res = std::min(res, (p - box.rightUp).length());
        res = std::min(res, (p - box.leftUp()).length());
        res = std::min(res, (p - box.rightDown()).length());
    }
    return res;
}

std::istream &operator>>(std::istream &in, Model &model) {
    int count;
    if (!(in >> count)) {
        throw model_format_error("unable to read number of figures");
    }
    while (count -- > 0) {
        std::string type;
        if (!(in >> type)) {
            throw model_format_error("unable to read fngure type");
        }
        if (type == "segment") {
            int x1, y1, x2, y2;
            if (!(in >> x1 >> y1 >> x2 >> y2)) {
                throw model_format_error("unable to read segment");
            }
            model.addFigure(std::make_shared<figures::Segment>(Point(x1, y1), Point(x2, y2)));
        } else if (type == "rectangle") {
            int x1, y1, x2, y2;
            if (!(in >> x1 >> y1 >> x2 >> y2)) {
                throw model_format_error("unable to read rectangle");
            }
            model.addFigure(std::make_shared<figures::Rectangle>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
        } else if (type == "ellipse") {
            int x1, y1, x2, y2;
            if (!(in >> x1 >> y1 >> x2 >> y2)) {
                throw model_format_error("unable to read ellipse");
            }
            model.addFigure(std::make_shared<figures::Ellipse>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
        } else {
            throw model_format_error("unknown type: '" + type + "'");
        }
    }
    return in;
}

class FigurePrinter : public FigureVisitor {
public:
    FigurePrinter(std::ostream &out) : out(out) {}
    virtual ~FigurePrinter() {}

    virtual void accept(figures::Segment &segm) {
        out << "segment ";
        printPoint(segm.getA());
        out << " ";
        printPoint(segm.getB());
        out << "\n";
    }

    virtual void accept(figures::Ellipse &fig) {
        out << "ellipse ";
        printBoundingBox(fig.getBoundingBox());
        out << "\n";
    }

    virtual void accept(figures::Rectangle &fig) {
        out << "rectangle ";
        printBoundingBox(fig.getBoundingBox());
        out << "\n";
    }

private:
    std::ostream &out;
    void printPoint(const Point &p) {
        out << p.x << " " << p.y;
    }
    void printBoundingBox(const BoundingBox &box) {
        printPoint(box.leftDown);
        out << " ";
        printPoint(box.rightUp);
    }
};

std::ostream &operator<<(std::ostream &out, Model &model) {
    out << model.size() << '\n';
    FigurePrinter printer(out);
    for (PFigure figure : model) {
        figure->visit(printer);
    }
    return out;
}
