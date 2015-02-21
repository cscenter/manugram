#ifndef LAYOUTING_H
#define LAYOUTING_H

#include "model.h"

class Layouter {
public:
    virtual void updateLayout(Model &model, PFigure changed) = 0;
};

class GridAlignLayouter : public Layouter {
public:
    GridAlignLayouter(int _gridStep);
    virtual void updateLayout(Model &model, PFigure changed) override;

private:
    int gridStep;
    void alignPoint(Point &p);
};

#endif // RECOGNITION_H
