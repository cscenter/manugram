#ifndef _MODEL_H
#define _MODEL_H

#include <boost/format.hpp>
#include <utility>
#include <string>
#include <vector>
#include <memory>
#include <list>

struct Point {
  double x, y;

  Point() : x(0), y(0) {}
  Point(double _x, double _y) : x(_x), y(_y) {}
  Point operator+(const Point &b) const { return {x + b.x, y + b.y }; }
  Point& operator+=(const Point &b) { x += b.x; y += b.y; return *this; }

  std::string str() const {
    static boost::format fmter("(%1%, %2%)");
    return (fmter % x % y).str();
  }
};

struct BoundingBox {
  Point leftDown, rightUp;
};

class Figure {
public:
  virtual ~Figure() {};
  virtual BoundingBox getBoundingBox() const = 0;
  virtual void translate(const Point &diff) = 0;
  virtual std::string str() const = 0;
};
typedef std::shared_ptr<Figure> PFigure;

namespace figures {
  class Segment : public Figure {
  public:
    Segment(const Point &_a, const Point &_b) : a(_a), b(_b) {}
    ~Segment() override {}

    BoundingBox getBoundingBox() const override {
      return { 
        Point(std::min(a.x, b.x), std::min(a.y, b.y)),
        Point(std::max(a.x, b.x), std::max(a.y, b.y))
      };
    }
    void translate(const Point &diff) override {
      a = a + diff;
      b = b + diff;
    }
    std::string str() const override {
      static boost::format fmter("segment(%1%--%2%)");
      return (fmter % a.str() % b.str()).str();
    }
  private:
    Point a, b;
  };

  class BoundedFigure : public Figure {
  public:
    BoundingBox getBoundingBox() const override {
      return box;
    }
    void translate(const Point &diff) override {
      box.leftDown += diff;
      box.rightUp += diff;
    }
  private:
    BoundingBox box;
  };

  class Ellipse : public BoundedFigure {
  };
  class Rectangle : public BoundedFigure {
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

private:
  std::list<PFigure> _figures;
};

#endif
