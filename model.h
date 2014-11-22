#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <list>
#include <cmath>
#include <cassert>

struct Point {
    double x, y;

    Point() : x(0), y(0) {}
    Point(double _x, double _y) : x(_x), y(_y) {}
    Point operator+(const Point &b) const { return {x + b.x, y + b.y }; }
    Point operator-(const Point &b) const { return {x - b.x, y - b.y }; }
    Point operator*(const double &b) const { return {x * b, y * b}; }
    Point &operator+=(const Point &b) { x += b.x; y += b.y; return *this; }
    double lengthSquared() const { return x * x + y * y; }
    double length() const { return sqrt(lengthSquared()); }

    std::string str() const {
        std::stringstream res;
        res << "(" << x << ", " << y << ")";
        return res.str();
    }
    static double dotProduct(const Point &a, const Point &b) {
        return a.x * b.x + a.y * b.y;
    }
};

struct BoundingBox {
    Point leftDown, rightUp;
    Point rightDown() const { return Point(rightUp.x, leftDown.y); }
    Point leftUp() const    { return Point(leftDown.x, rightUp.y); }
    Point center()  const { return (leftDown + rightUp) * 0.5; }
    double width () const { return rightUp.x - leftDown.x; }
    double height() const { return rightUp.y - leftDown.y; }
};

class FigureVisitor;

class Figure {
public:
    virtual ~Figure() {}
    virtual BoundingBox getBoundingBox() const = 0;
    virtual void translate(const Point &diff) = 0;
    virtual std::string str() const = 0;
    virtual void visit(FigureVisitor &) = 0;
    virtual double getApproximateDistanceToBorder(const Point &p) = 0;
    virtual void recalculate() {}
};
typedef std::shared_ptr<Figure> PFigure;

namespace figures {
class Segment;
class SegmentConnection;
class BoundedFigure;
class Ellipse;
class Rectangle;
typedef std::shared_ptr<BoundedFigure> PBoundedFigure;
}

class FigureVisitor {
public:
    virtual ~FigureVisitor() {}
    virtual void accept(figures::Segment &) = 0;
    virtual void accept(figures::SegmentConnection &) = 0;
    virtual void accept(figures::Ellipse &) = 0;
    virtual void accept(figures::Rectangle &) = 0;
};

namespace figures {
class Segment : public Figure {
public:
    Segment(const Point &_a, const Point &_b) : a(_a), b(_b) {}
    void visit(FigureVisitor &v) override { v.accept(*this); }

    Point getA() const { return a; }
    Point getB() const { return b; }

    BoundingBox getBoundingBox() const override {
        return {
            Point(std::min(a.x, b.x), std::min(a.y, b.y)),
            Point(std::max(a.x, b.x), std::max(a.y, b.y))
        };
    }
    void translate(const Point &diff) override {
        a += diff;
        b += diff;
    }
    std::string str() const override {
        std::stringstream res;
        res << "segment(" << a.str() << "--" << b.str() << ")";
        return res.str();
    }

    double getApproximateDistanceToBorder(const Point &p) override;

protected:
    Segment() {}

    Point a, b;
    double getDistanceToLine(const Point &p);
};

class BoundedFigure : public Figure {
public:
    BoundedFigure(BoundingBox box) : box(box) {}
    BoundingBox getBoundingBox() const override {
        return box;
    }
    void setBoundingBox(BoundingBox &_box) {
        box = _box;
    }
    void translate(const Point &diff) override {
        box.leftDown += diff;
        box.rightUp += diff;
    }
protected:
    BoundingBox box;
};

class SegmentConnection : public Segment {
public:
    SegmentConnection(const PBoundedFigure &_figA, const PBoundedFigure &_figB)
            : figA(_figA), figB(_figB) {
        recalculate();
    }
    void visit(FigureVisitor &v) override { v.accept(*this); }
    void recalculate() override {
        a = figA->getBoundingBox().center();
        b = figB->getBoundingBox().center();
    }
protected:
    std::shared_ptr<BoundedFigure> figA, figB;
};

class Ellipse : public BoundedFigure {
public:
    Ellipse(BoundingBox box) : BoundedFigure(box) {}
    void visit(FigureVisitor &v) override { v.accept(*this); }
    std::string str() const override {
        std::stringstream res;
        res << "ellipse(" << getBoundingBox().leftDown.str() << "--" << getBoundingBox().rightUp.str() << ")";
        return res.str();
    }
    double getApproximateDistanceToBorder(const Point &) override;
};
class Rectangle : public BoundedFigure {
public:
    Rectangle(BoundingBox box) : BoundedFigure(box) {}
    void visit(FigureVisitor &v) override { v.accept(*this); }
    std::string str() const override {
        std::stringstream res;
        res << "rectangle(" << getBoundingBox().leftDown.str() << "--" << getBoundingBox().rightUp.str() << ")";
        return res.str();
    }
    double getApproximateDistanceToBorder(const Point &p) override;
};
} // namespace figures

class Model {
public:
    typedef std::list<PFigure>::iterator iterator;

    iterator addFigure(PFigure a) {
        _figures.push_back(std::move(a));
        return --_figures.end();
    }
    iterator begin() {
        return _figures.begin();
    }
    iterator end() {
        return _figures.end();
    }
    void removeFigure(iterator it) {
        _figures.erase(it);
    }
    size_t size() const {
        return _figures.size();
    }
    void recalculate() {
        for (PFigure fig : *this) {
            fig->recalculate();
        }
    }

private:
    std::list<PFigure> _figures;
};

class model_format_error : std::runtime_error {
public:
    model_format_error(const std::string &message) : std::runtime_error("Invalid model format: " + message) {}
    virtual const char *what() const throw() { return std::runtime_error::what(); }
};

std::istream &operator>>(std::istream &in , Model &model);
std::ostream &operator<<(std::ostream &in , Model &model);

struct Track {
    std::vector<Point> points;

    bool empty() const { return points.empty(); }
    size_t size() const { return points.size(); }
    Point &operator[](size_t id)       { return points[id]; }
    const Point &operator[](size_t id) const { return points[id]; }
};

#endif // MODEL_H
