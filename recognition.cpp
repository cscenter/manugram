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

bool isDeletionTrack(const Track &track) {
    int cnt = 0;
    std::cout << "new\n";
    for (int i = 0; i < (int)track.size(); i++) {
        int prevPoint = i - 1;
        while (prevPoint >= 0 && (track[i] - track[prevPoint]).lengthSquared() < 25) {
            prevPoint--;
        }
        int nextPoint = i + 1;
        while (nextPoint < (int)track.size() && (track[i] - track[nextPoint]).lengthSquared() < 25) {
            nextPoint++;
        }
        if (prevPoint >= 0 && nextPoint < (int)track.size()) {
            Point vec1 = track[nextPoint] - track[i];
            Point vec2 = track[prevPoint] - track[i];
            double ang = acos(Point::dotProduct(vec1, vec2) / (vec1.length() * vec2.length()));
            if (ang <= PI * 30 / 180) {
                cnt++;
                i = nextPoint;
            }
        }
    }
    std::cout << "deletion cnt = " << cnt << "\n";
    return cnt >= 3;
}

PFigure recognizeGrabs(const Track &track, Model &model) {
    Point start = track[0];
    Point end = track[track.size() - 1];

    for (auto it = model.begin(); it != model.end(); it++) {
        PFigure figure = *it;
        if (figure->getApproximateDistanceToBorder(start) < 10) { // grabbed
            // recognize deletion
            if (isDeletionTrack(track)) {
                model.removeFigure(it);
                return figure;
            }

            // try connection
            auto figA = dynamic_pointer_cast<BoundedFigure>(figure);
            if (figA) {
                for (PFigure figure2 : model) {
                    auto figB = dynamic_pointer_cast<BoundedFigure>(figure2);
                    if (figB && figB->getApproximateDistanceToBorder(end) < 10) {
                        auto result = make_shared<SegmentConnection>(figA, figB);
                        model.addFigure(result);
                        return result;
                    }
                }
            }

            // now we try translation
            figure->translate(end - start);
            model.recalculate();
            return figure;
        }
    }
    return nullptr;
}

void squareBoundedFigure(PBoundedFigure figure) {
    BoundingBox box = figure->getBoundingBox();
    double siz1 = box.width();
    double siz2 = box.height();
    if (siz1 > siz2) {
        std::swap(siz1, siz2);
    }
    std::cout << "bounded size=" << siz1 << ";" << siz2 << "; k=" << (siz1 / siz2) << "\n";
    if (siz1 / siz2 < 0.8) { return; }

    double siz = (siz1 + siz2) / 2;
    box.rightUp = box.leftDown + Point(siz, siz);
    figure->setBoundingBox(box);
}

PFigure recognizeClicks(const Point &click, Model &model) {
    for (PFigure figure : model) {
        if (figure->getApproximateDistanceToBorder(click) > 10) {
            continue;
        }
        model.selectedFigure = figure;

        std::shared_ptr<Segment> segm = dynamic_pointer_cast<Segment>(figure);
        if (segm) {
            if ((click - segm->getA()).length() < 10) {
                segm->setArrowedA(!segm->getArrowedA());
            }
            if ((click - segm->getB()).length() < 10) {
                segm->setArrowedB(!segm->getArrowedB());
            }
            return segm;
        }
    }
    return nullptr;
}

PFigure recognize(const Track &track, Model &model) {
    if (track.empty()) { return nullptr; }

    // Very small tracks are clicks
    if (getClosedCircumference(track) < 10) {
        return recognizeClicks(track[0], model);
    }

    // Moving and connecting
    if (PFigure result = recognizeGrabs(track, model)) {
        return result;
    }

    // Drawing new figures
    std::vector<PFigure> candidates;
    if (!isClosed(track))  {
        candidates.push_back(make_shared<Segment>(track[0], track[track.size() - 1]));
    } else {
        candidates.push_back(make_shared<Ellipse>(getBoundingBox(track)));
        candidates.push_back(make_shared<Rectangle>(getBoundingBox(track)));
    }

    if (candidates.empty()) { return nullptr; }

    std::vector<double> fits;
    for (PFigure figure : candidates) {
        fits.push_back(fitsToTrack(track, figure));
    }
    size_t id = max_element(fits.begin(), fits.end()) - fits.begin();
    std::cout << "max_fit = " << fits[id] << "; id = " << id << "\n";
    if (fits[id] >= 0.75) { // we allow some of points to fall out of our track
        auto boundedFigure = dynamic_pointer_cast<BoundedFigure>(candidates[id]);
        if (boundedFigure) {
            squareBoundedFigure(boundedFigure);
        }
        model.addFigure(candidates[id]);
        return candidates[id];
    }
    return nullptr;
}
