#ifndef RECOGNITION_H
#define RECOGNITION_H

#include "model.h"

enum class RecognitionPreset {
    Mouse,
    Touch
};

void setRecognitionPreset(RecognitionPreset preset);
int figureSelectGap();

// Returns pointer to the figure modified
PFigure recognize(const Track &track, Model &model);

std::vector<double> calculateRelativeSpeeds(const Track &track);

#endif // RECOGNITION_H
