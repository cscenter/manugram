#include "model.h"
#include <string>
#include <exception>

std::istream& operator>>(std::istream& in, Model &model) {
  int count;
  in >> count;
  while (count --> 0) {
    std::string type;
    in >> type;
    if (type == "segment") {
      int x1, y1, x2, y2;
      in >> x1 >> y1 >> x2 >> y2;
      model.addFigure(std::make_shared<figures::Segment>(Point(x1, y1), Point(x2, y2)));
    } else {
      throw std::runtime_error("Invalid model format");
    }
  }
  return in;
}
