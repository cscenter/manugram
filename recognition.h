#ifndef RECOGNITION_H
#define RECOGNITION_H

#include "model.h"

enum class RecognitionPreset {
    Mouse,
    Touch
};

void setRecognitionPreset(RecognitionPreset preset);

// Returns pointer to the figure modified
PFigure recognize(const Track &track, Model &model);

#endif // RECOGNITION_H
