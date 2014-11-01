#include "model.h"
#include <string>
#include <exception>

std::istream& operator>>(std::istream& in, Model &model) {
  int count;
  if (!(in >> count)) {
    throw std::runtime_error("Invalid model format: unable to read number of figures");
  }
  while (count --> 0) {
    std::string type;
    in >> type;
    if (type == "segment") {
      int x1, y1, x2, y2;
      in >> x1 >> y1 >> x2 >> y2;
      model.addFigure(std::make_shared<figures::Segment>(Point(x1, y1), Point(x2, y2)));
    } else if (type == "rectangle") {
      int x1, y1, x2, y2;
      in >> x1 >> y1 >> x2 >> y2;
      model.addFigure(std::make_shared<figures::Rectangle>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
    } else if (type == "ellipse") {
      int x1, y1, x2, y2;
      in >> x1 >> y1 >> x2 >> y2;
      model.addFigure(std::make_shared<figures::Ellipse>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
    } else {
      throw std::runtime_error("Invalid model format: unknown type: " + type);
    }
  }
  return in;
}
