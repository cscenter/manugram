#include "model.h"
#include <memory>

void recognize(const Track &track, Model &model) {
    if (track.empty()) return;
    model.addFigure(std::make_shared<figures::Segment>(track[0], track[track.size() - 1]));
}
