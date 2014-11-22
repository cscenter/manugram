#include "model.h"
#include <memory>
#include <algorithm>
#include <vector>

using std::min;
using std::max;
using std::make_shared;
using std::dynamic_pointer_cast;

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

double fitsToTrack(const Track &track, const PFigure &figure) {
    // Point should fall nearer than 10 pixels
    BoundingBox box = getBoundingBox(track);
    double maxDistance = 10;
    maxDistance = std::min(maxDistance, std::min(box.width(), box.height()) * 0.2);

    int goodCount = 0;
    for (Point p : track.points) {
        goodCount += figure->getApproximateDistanceToBorder(p) <= maxDistance;
    }

    return goodCount * 1.0 / track.size();
}

using namespace figures;

bool recognizeGrabs(const Track &track, Model &model) {
    Point start = track[0];
    Point end = track[track.size() - 1];

    for (PFigure figure : model) {
        if (figure->getApproximateDistanceToBorder(start) < 10) { // grabbed
            // try connection first
            auto figA = dynamic_pointer_cast<BoundedFigure>(figure);
            if (figA) {
                for (PFigure figure2 : model) {
                    auto figB = dynamic_pointer_cast<BoundedFigure>(figure2);
                    if (figB && figB->getApproximateDistanceToBorder(end) < 10) {
                        model.addFigure(make_shared<SegmentConnection>(figA, figB));
                        return true;
                    }
                }
            }

            // now we try translation
            figure->translate(end - start);
            model.recalculate();
            return true;
        }
    }
    return false;
}

void recognize(const Track &track, Model &model) {
    if (track.empty()) { return; }

    // Moving and connecting
    if (recognizeGrabs(track, model)) {
        return;
    }

    // Drawing new figures

    // Ignore very small tracks
    if (getClosedCircumference(track) < 10) { return; }

    std::vector<PFigure> candidates;
    if (!isClosed(track))  {
        candidates.push_back(make_shared<Segment>(track[0], track[track.size() - 1]));
    } else {
        candidates.push_back(make_shared<Ellipse>(getBoundingBox(track)));
        candidates.push_back(make_shared<Rectangle>(getBoundingBox(track)));
    }

    if (candidates.empty()) { return; }

    std::vector<double> fits;
    for (PFigure figure : candidates) {
        fits.push_back(fitsToTrack(track, figure));
    }
    int id = max_element(fits.begin(), fits.end()) - fits.begin();
    std::cout << "max_fit = " << fits[id] << "; id = " << id << "\n";
    if (fits[id] >= 0.85) { // we allow 5% of points to fall out of our track
        model.addFigure(candidates[id]);
    }
}
