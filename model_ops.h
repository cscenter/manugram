#ifndef MODEL_OPS
#define MODEL_OPS

#include "model.h"
void makeVerticallySymmetric(std::shared_ptr<figures::Curve> curve);
void makeHorizontallySymmetric(std::shared_ptr<figures::Curve> curve);
void makeTopBottomTree(Model &model, figures::PBoundedFigure root);

#endif // MODEL_OPS

