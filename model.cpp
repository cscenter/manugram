#include "model.h"
#include <string>
#include <algorithm>
#include <map>

double figures::Segment::getApproximateDistanceToBorder(const Point &p) {
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

double figures::Rectangle::getApproximateDistanceToBorder(const Point &p) {
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

double figures::Ellipse::getApproximateDistanceToBorder(const Point &_p) {
    Point p = _p - box.center();
    if (p.length() < 1e-7) { return std::min(box.width(), box.height()) / 2; }
    double a = box.width() / 2;
    double b = box.height() / 2;
    Point near = p;
    near.x /= a; near.y /= b;
    double d = near.length();
    near.x /= d; near.y /= d;
    near.x *= a; near.y *= b;
    return (p - near).length();
}

std::vector<Point> getMiddleBorderPoints(BoundingBox box) {
    std::vector<Point> res;
    res.push_back(Point(box.center().x, box.leftDown.y));
    res.push_back(Point(box.center().x, box.rightUp.y));
    res.push_back(Point(box.leftDown.x, box.center().y));
    res.push_back(Point(box.rightUp.x, box.center().y));
    return res;
}

void figures::SegmentConnection::recalculate() {
    using std::vector;
    using std::get;
    vector<Point> as = getMiddleBorderPoints(figA->getBoundingBox());
    vector<Point> bs = getMiddleBorderPoints(figB->getBoundingBox());
    double minDistSquared = HUGE_VAL;
    for (Point a : as) {
        for (Point b : bs) {
            double curDistSquared = (a - b).lengthSquared();
            if (minDistSquared > curDistSquared) {
                minDistSquared = curDistSquared;
                this->a = a;
                this->b = b;
            }
        }
    }
}

std::istream &operator>>(std::istream &in, Model &model) {
    int count;
    if (!(in >> count)) {
        throw model_format_error("unable to read number of figures");
    }
    std::vector<PFigure> figures;
    while (count -- > 0) {
        std::string type;
        if (!(in >> type)) {
            throw model_format_error("unable to read fngure type");
        }
        if (type == "segment" || type == "segment_connection") {
            std::shared_ptr<figures::Segment> segm;
            if (type == "segment_connection") {
                size_t aId, bId;
                if (!(in >> aId >> bId)) {
                    throw model_format_error("unable to read connection information");
                }
                aId--, bId--;
                if (aId >= figures.size() || bId >= figures.size()) {
                    throw model_format_error("invalid figures in connection");
                }
                auto figA = std::dynamic_pointer_cast<figures::BoundedFigure>(figures.at(aId));
                auto figB = std::dynamic_pointer_cast<figures::BoundedFigure>(figures.at(bId));
                if (!figA || !figB) {
                    throw model_format_error("invalid reference in connection");
                }
                segm = std::make_shared<figures::SegmentConnection>(figA, figB);
            } else {
                double x1, y1, x2, y2;
                if (!(in >> x1 >> y1 >> x2 >> y2)) {
                    throw model_format_error("unable to read segment");
                }
                segm = std::make_shared<figures::Segment>(Point(x1, y1), Point(x2, y2));
            }
            bool arrowA, arrowB;
            if (!(in >> arrowA >> arrowB)) {
                throw model_format_error("unable to read segment");
            }
            segm->setArrowedA(arrowA);
            segm->setArrowedB(arrowB);
            figures.push_back(segm);
        } else if (type == "rectangle") {
            double x1, y1, x2, y2;
            if (!(in >> x1 >> y1 >> x2 >> y2)) {
                throw model_format_error("unable to read rectangle");
            }
            figures.push_back(std::make_shared<figures::Rectangle>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
        } else if (type == "ellipse") {
            double x1, y1, x2, y2;
            if (!(in >> x1 >> y1 >> x2 >> y2)) {
                throw model_format_error("unable to read ellipse");
            }
            figures.push_back(std::make_shared<figures::Ellipse>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
        } else {
            throw model_format_error("unknown type: '" + type + "'");
        }
    }
    for (PFigure figure : figures) {
        model.addFigure(figure);
    }
    return in;
}

class FigurePrinter : public FigureVisitor {
public:
    FigurePrinter(std::ostream &out, const std::map<PFigure, size_t> &ids) : out(out), ids(ids) {}

    virtual void accept(figures::Segment &segm) {
        out << "segment ";
        printPoint(segm.getA());
        out << " ";
        printPoint(segm.getB());
        out << " " << segm.getArrowedA() << " " << segm.getArrowedB() << "\n";
    }
    virtual void accept(figures::SegmentConnection &segm) {
        out << "segment_connection ";
        out << ids.at(segm.getFigureA()) << " ";
        out << ids.at(segm.getFigureB()) << " ";
        out << " " << segm.getArrowedA() << " " << segm.getArrowedB() << "\n";
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
    const std::map<PFigure, size_t> &ids;
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

    std::map<PFigure, size_t> ids;
    for (PFigure figure : model) {
        int id = ids.size() + 1;
        ids[figure] = id;
    }

    FigurePrinter printer(out, ids);
    for (PFigure figure : model) {
        figure->visit(printer);
    }
    return out;
}

class CloningVisitor : public FigureVisitor {
public:
    CloningVisitor(const std::map<PFigure, PFigure> &mapping) : mapping(mapping) {}
    PFigure getResult() { return result; }

    virtual void accept(figures::Segment &fig) override {
        result = std::make_shared<figures::Segment>(fig);
    }
    virtual void accept(figures::SegmentConnection &fig) {
        auto res = std::make_shared<figures::SegmentConnection>(fig.getFigureA(), fig.getFigureB());
        res->setArrowedA(fig.getArrowedA());
        res->setArrowedB(fig.getArrowedB());
        result = res;
    }

    virtual void accept(figures::Ellipse &fig) {
        result = std::make_shared<figures::Ellipse>(fig);
    }

    virtual void accept(figures::Rectangle &fig) {
        result = std::make_shared<figures::Rectangle>(fig);
    }

private:
    const std::map<PFigure, PFigure> &mapping;
    PFigure result;
};

PFigure clone(PFigure figure, const std::map<PFigure, PFigure> &othersMapping) {
    CloningVisitor visitor(othersMapping);
    figure->visit(visitor);
    return visitor.getResult();
}
