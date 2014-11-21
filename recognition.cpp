#include "model.h"
#include <memory>
#include <algorithm>

using std::min;
using std::max;
using std::make_shared;

BoundingBox getBoundingBox(const Track &track) {
    BoundingBox res;
    res.leftDown = res.rightUp = track[0];
    for (Point p : track.points) {
        res.leftDown.x = min(res.leftDown.x, p.x);
        res.leftDown.y = min(res.leftDown.y, p.y);
        res.rightUp.x = max(res.rightUp.x, p.x);
        res.rightUp.y = max(res.rightUp.y, p.y);
    }
    return res;
}

double getCircumference(const Track &track) {
    double res = 0;
    for (size_t i = 0; i + 1 < track.size(); i++) {
        res += (track[i + 1] - track[i]).length();
    }
    return res;
}
double getClosedCircumference(const Track &track) {
    double res = getCircumference(track);
    res += (track[0] - track[track.size() - 1]).length();
    return res;
}

bool isClosed(const Track &track) {
    return (track[0] - track[track.size() - 1]).length() <= 10;
}

bool fitsToTrack(const Track &track, const PFigure &figure) {
    // Point should fall nearer than 10 pixels
    double maxDistance = 10;

    int goodCount = 0;
    for (Point p : track.points) {
        goodCount += figure->getApproximateDistanceToBorder(p) <= maxDistance;
    }

    // At least 90% of points fall nearer than 'maxDistance'
    return goodCount * 100 / track.size() >= 90;
}

void recognize(const Track &track, Model &model) {
    if (track.empty()) { return; }

    // Moving
    for (PFigure figure : model) {
        if (figure->getApproximateDistanceToBorder(track[0]) < 10) { // grabbed
            figure->translate(track[track.size() - 1] - track[0]);
            return;
        }
    }

    // Drawing new figures

    // Ignore very small tracks
    if (getClosedCircumference(track) < 10) { return; }

    using namespace figures;
    PFigure figure;
    if (!isClosed(track))  {
        figure = make_shared<Segment>(track[0], track[track.size() - 1]);
    } else {
        figure = make_shared<Rectangle>(getBoundingBox(track));
    }
    if (figure && fitsToTrack(track, figure)) {
        model.addFigure(figure);
    }
}
