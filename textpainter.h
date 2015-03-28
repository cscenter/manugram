#ifndef TEXTPAINTER_H
#define TEXTPAINTER_H

#include "model.h"
struct TextPosition {
    Point leftUp;
    double width, height;
    double rotation;

    TextPosition() : leftUp(), width(0), height(0), rotation(0) {}
};

TextPosition getTextPosition(Figure &figure);

#endif // TEXTPAINTER_H
