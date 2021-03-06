#include <memory>
#include "layouting.h"

using namespace figures;
using std::dynamic_pointer_cast;

GridAlignLayouter::GridAlignLayouter(int _gridStep) : gridStep(_gridStep) {}

void GridAlignLayouter::alignPoint(Point &p) {
    p.x = round(p.x / gridStep) * gridStep;
    p.y = round(p.y / gridStep) * gridStep;
}

void GridAlignLayouter::updateLayout(Model &, PFigure changed) {
    if (!changed) {
        return;
    }
    if (auto boundedFigure = dynamic_pointer_cast<BoundedFigure>(changed)) {
        BoundingBox box = boundedFigure->getBoundingBox();
        alignPoint(box.leftUp);
        alignPoint(box.rightDown);
        boundedFigure->setBoundingBox(box);
    }
    if (auto curve = dynamic_pointer_cast<Curve>(changed)) {
        for (Point &p : curve->points) {
            alignPoint(p);
        }
    }
    if (typeid(*changed) == typeid(figures::Segment)) {
        auto segment = dynamic_pointer_cast<Segment>(changed);
        Point a = segment->getA();
        Point b = segment->getB();
        alignPoint(a);
        alignPoint(b);
        segment->setA(a);
        segment->setB(b);
    }
}
