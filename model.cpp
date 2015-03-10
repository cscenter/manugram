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

/*
 * Consider a segment from figure's center to point b
 * Intersection point of this segment with figure is returned,
 * If there is no such one or figure is degenerate, center is returned instead
 */
class CutSegmentFromCenterVisitor : FigureVisitor {
    static constexpr double EPS = 1e-8;
    Point b;
    Point _result;
    CutSegmentFromCenterVisitor(const Point &_b) : b(_b) {}
public:
    Point result() { return _result; }
    virtual void accept(figures::Ellipse &figure) override {
        BoundingBox ellipse = figure.getBoundingBox();
        _result = ellipse.center();
        if (ellipse.width() < EPS || ellipse.height() < EPS) {
            return;
        }
        Point direction = b - ellipse.center();
        double aspectRatio = ellipse.width() / ellipse.height();
        direction.x /= aspectRatio;
        double r = ellipse.height() / 2;
        if (direction.length() < r - EPS) {
            return;
        }
        direction = direction * (r / direction.length());
        direction.x *= aspectRatio;
        _result = ellipse.center() + direction;
    }

    virtual void accept(figures::Rectangle &figure) override {
        BoundingBox rect = figure.getBoundingBox();
        _result = rect.center();
        if (rect.width() < EPS || rect.height() < EPS) {
            return;
        }
        Point direction = b - rect.center();
        double aspectRatio = rect.width() / rect.height();
        direction.x /= aspectRatio;
        double r = rect.height() / 2;
        double compressRatio = std::min(r / fabs(direction.x), r / fabs(direction.y));
        if (compressRatio > 1 - EPS) {
            return;
        }
        direction = direction * compressRatio;
        direction.x *= aspectRatio;
        _result = rect.center() + direction;
    }
    virtual void accept(figures::Segment &) override { throw visitor_implementation_not_found(); }
    virtual void accept(figures::SegmentConnection &) override { throw visitor_implementation_not_found(); }
    static Point apply(PFigure figure, Point b) {
        CutSegmentFromCenterVisitor visitor(b);
        figure->visit(visitor);
        return visitor.result();
    }
};

void figures::SegmentConnection::recalculate() {
    Point centerA = figA->getBoundingBox().center();
    Point centerB = figB->getBoundingBox().center();
    this->a = CutSegmentFromCenterVisitor::apply(figA, centerB);
    this->b = CutSegmentFromCenterVisitor::apply(figB, centerA);
}

class CloningVisitor : public FigureVisitor {
public:
    CloningVisitor(const std::map<PFigure, PFigure> &mapping) : mapping(mapping) {}
    PFigure getResult() { return result; }

    virtual void accept(figures::Segment &fig) override {
        result = std::make_shared<figures::Segment>(fig);
    }
    virtual void accept(figures::SegmentConnection &fig) {
        auto newA = std::dynamic_pointer_cast<figures::BoundedFigure>(mapping.at(fig.getFigureA()));
        auto newB = std::dynamic_pointer_cast<figures::BoundedFigure>(mapping.at(fig.getFigureB()));
        auto res = std::make_shared<figures::SegmentConnection>(newA, newB);
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
