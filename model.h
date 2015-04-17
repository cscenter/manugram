#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <stdexcept>
#include <list>
#include <cmath>
#include <cassert>

const double PI = atan(1.0) * 4;
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

    void rotateBy(double angle) {
        double nx = x * cos(angle) - y * sin(angle);
        double ny = x * sin(angle) + y * cos(angle);
        x = nx;
        y = ny;
    }

    std::string str() const {
        std::stringstream res;
        res << "(" << x << ", " << y << ")";
        return res.str();
    }
    static double dotProduct(const Point &a, const Point &b) {
        return a.x * b.x + a.y * b.y;
    }
    static double crossProduct(const Point &a, const Point &b) {
        return a.x * b.y - a.y * b.x;
    }
};

/*
 * OX axis goes from left to right
 * OY axis goes from top to bottom (as on the screen)
 */
struct BoundingBox {
    Point leftUp, rightDown;
    BoundingBox() : leftUp(INFINITY, INFINITY), rightDown(-INFINITY, -INFINITY) {}
    BoundingBox(std::initializer_list<Point> points) : BoundingBox() {
        for (Point p : points) {
            addPoint(p);
        }
    }

    void addPoint(Point p) {
        leftUp.x = std::min(leftUp.x, p.x);
        leftUp.y = std::min(leftUp.y, p.y);
        rightDown.x = std::max(rightDown.x, p.x);
        rightDown.y = std::max(rightDown.y, p.y);
    }

    Point rightUp()  const { return Point(rightDown.x, leftUp.y); }
    Point leftDown() const { return Point(leftUp.x, rightDown.y); }
    Point center()   const { return (leftUp + rightDown) * 0.5; }
    double width ()  const { return rightDown.x - leftUp.x; }
    double height()  const { return rightDown.y - leftUp.y; }
};

class Figure;
class FigureVisitor;

typedef std::shared_ptr<Figure> PFigure;

class Figure {
public:
    virtual ~Figure() {}
    virtual BoundingBox getBoundingBox() const = 0;
    virtual void translate(const Point &diff) = 0;
    virtual std::string str() const = 0;
    virtual void visit(FigureVisitor &) = 0;
    virtual bool isInsideOrOnBorder(const Point &p) = 0;
    virtual double getApproximateDistanceToBorder(const Point &p) = 0;
    virtual void recalculate() {}
    virtual bool dependsOn(const PFigure &) { return false; }
    std::string label() const {
        return _label;
    }
    void setLabel(const std::string &newLabel) {
        _label = newLabel;
    }
protected:
    std::string _label;
};
PFigure clone(PFigure figure, const std::map<PFigure, PFigure> &othersMapping);

namespace figures {
class Segment;
class SegmentConnection;
class Curve;
class BoundedFigure;
class Ellipse;
class Rectangle;
typedef std::shared_ptr<BoundedFigure> PBoundedFigure;
}

class visitor_implementation_not_found : std::logic_error {
public:
    visitor_implementation_not_found() : std::logic_error("Visitor implementation not found") {}
    virtual const char *what() const throw() { return std::logic_error::what(); }
};

class FigureVisitor {
public:
    virtual ~FigureVisitor() {}
    virtual void accept(figures::Segment &) = 0;
    virtual void accept(figures::SegmentConnection &) = 0;
    virtual void accept(figures::Curve &) = 0;
    virtual void accept(figures::Ellipse &) = 0;
    virtual void accept(figures::Rectangle &) = 0;
};

namespace figures {
class Segment : public Figure {
public:
    Segment(const Point &_a, const Point &_b) : a(_a), b(_b), arrowedA(false), arrowedB(false) {}
    void visit(FigureVisitor &v) override { v.accept(*this); }

    Point getA() const { return a; }
    Point getB() const { return b; }
    bool getArrowedA() const { return arrowedA; }
    bool getArrowedB() const { return arrowedB; }
    void setArrowedA(bool val) { arrowedA = val; }
    void setArrowedB(bool val) { arrowedB = val; }

    BoundingBox getBoundingBox() const override {
        return { a, b };
    }
    void translate(const Point &diff) override {
        a += diff;
        b += diff;
    }
    std::string str() const override {
        std::stringstream res;
        res << "segment(" << a.str()
            << (arrowedA ? "<" : "") <<  "--" << (arrowedB ? ">" : "")
            << b.str() << ")";
        return res.str();
    }

    bool isInsideOrOnBorder(const Point &p) override;
    double getApproximateDistanceToBorder(const Point &p) override;

protected:
    Segment() : arrowedA(false), arrowedB(false) {}

    Point a, b;
    bool arrowedA, arrowedB;
    double getDistanceToLine(const Point &p);
};
class Curve : public Figure {
public:
    Curve(const std::vector<Point> _points) : points(_points) {}
    virtual BoundingBox getBoundingBox() const override;
    virtual void translate(const Point &diff) override;
    virtual std::string str() const override;
    virtual void visit(FigureVisitor &v) override { v.accept(*this); }
    virtual bool isInsideOrOnBorder(const Point &p) override;
    virtual double getApproximateDistanceToBorder(const Point &p) override;
    std::vector<Point> points;
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
        box.leftUp += diff;
        box.rightDown += diff;
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
    SegmentConnection(const SegmentConnection &other) = delete;
    SegmentConnection &operator=(const SegmentConnection &other) = delete;
    PBoundedFigure getFigureA() { return figA; }
    PBoundedFigure getFigureB() { return figB; }
    void visit(FigureVisitor &v) override { v.accept(*this); }
    void recalculate() override;
    virtual bool dependsOn(const PFigure &other) {
        return other == figA || other == figB;
    }

protected:
    PBoundedFigure figA, figB;
};

class Ellipse : public BoundedFigure {
public:
    Ellipse(BoundingBox box) : BoundedFigure(box) {}
    void visit(FigureVisitor &v) override { v.accept(*this); }
    std::string str() const override {
        std::stringstream res;
        res << "ellipse(" << getBoundingBox().leftUp.str() << "--" << getBoundingBox().rightDown.str() << ")";
        return res.str();
    }
    bool isInsideOrOnBorder(const Point &p) override;
    double getApproximateDistanceToBorder(const Point &) override;
};
class Rectangle : public BoundedFigure {
public:
    Rectangle(BoundingBox box) : BoundedFigure(box) {}
    void visit(FigureVisitor &v) override { v.accept(*this); }
    std::string str() const override {
        std::stringstream res;
        res << "rectangle(" << getBoundingBox().leftUp.str() << "--" << getBoundingBox().rightDown.str() << ")";
        return res.str();
    }
    bool isInsideOrOnBorder(const Point &p) override;
    double getApproximateDistanceToBorder(const Point &p) override;
};
} // namespace figures

class Model {
public:
    Model() {}
    Model(const Model &other) {
        std::map<PFigure, PFigure> mapping;
        for (PFigure figure : other._figures) {
            mapping.insert(std::make_pair(figure, *addFigure(clone(figure, mapping))));
        }
        if (other.selectedFigure) {
            selectedFigure = mapping.at(other.selectedFigure);
        }
    }
    Model(Model &&other) : _figures(std::move(other._figures)), selectedFigure(std::move(other.selectedFigure)) {}
    Model &operator=(Model other) {
        swap(other);
        return *this;
    }

    void swap(Model &other) {
        _figures.swap(other._figures);
        std::swap(selectedFigure, other.selectedFigure);
    }

    typedef std::list<PFigure>::iterator iterator;
    typedef std::list<PFigure>::const_iterator const_iterator;

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
    const_iterator begin() const {
        return _figures.begin();
    }
    const_iterator end() const {
        return _figures.end();
    }
    void removeFigure(iterator it) {
        PFigure old = *it;
        _figures.erase(it);
        for (auto it2 = _figures.begin(); it2 != _figures.end(); it2++) {
            if ((*it2)->dependsOn(old)) {
                removeFigure(it2);
                it2 = _figures.begin();
            }
        }
        if (old == selectedFigure) {
            selectedFigure.reset();
        }
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
public:
    PFigure selectedFigure;
};

struct Track {
    std::vector<Point> points;

    bool empty() const { return points.empty(); }
    size_t size() const { return points.size(); }
    Point &operator[](size_t id)       { return points[id]; }
    const Point &operator[](size_t id) const { return points[id]; }
};

#endif // MODEL_H
