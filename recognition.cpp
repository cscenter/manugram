#include "model.h"
#include "recognition.h"
#include <memory>
#include <algorithm>
#include <vector>

using std::min;
using std::max;
using std::make_shared;
using std::dynamic_pointer_cast;

int FIGURE_SELECT_GAP = -1;
int CLOSED_FIGURE_GAP = -1;
const int TRACK_FIT_GAP = 10;
const int DELETION_MOVE_MINIMAL_LENGTH = 5;
const int DELETION_MOVE_MAX_ANGLE = 30;
const int DELETION_MOVE_MIN_COUNT = 3;
const double SQUARING_MIN_RATIO = 0.8;
const double MIN_FIT_POINTS_AMOUNT = 0.75;

void setRecognitionPreset(RecognitionPreset preset) {
    switch (preset) {
    case RecognitionPreset::Mouse:
        FIGURE_SELECT_GAP = 10;
        CLOSED_FIGURE_GAP = 10;
        break;
    case RecognitionPreset::Touch:
        FIGURE_SELECT_GAP = 30;
        CLOSED_FIGURE_GAP = 50;
        break;
    };
}

BoundingBox getBoundingBox(const Track &track) {
    BoundingBox res;
    res.leftUp = res.rightDown = track[0];
    for (Point p : track.points) {
        res.leftUp.x = min(res.leftUp.x, p.x);
        res.leftUp.y = min(res.leftUp.y, p.y);
        res.rightDown.x = max(res.rightDown.x, p.x);
        res.rightDown.y = max(res.rightDown.y, p.y);
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

bool cutToClosed(Track &track) {
    auto &points = track.points;
    auto it = points.begin();
    while (it != points.end() && (points[0] - *it).length() <= CLOSED_FIGURE_GAP) {
        it++;
    }
    while (it != points.end() && (points[0] - *it).length() > CLOSED_FIGURE_GAP) {
        it++;
    }
    if (it == points.end()) {
        return false;
    }
    while (it != points.end() && (points[0] - *it).length() <= CLOSED_FIGURE_GAP) {
        it++;
    }
    points.erase(it, points.end());
    return true;
}

double fitsToTrack(const Track &track, const PFigure &figure) {
    BoundingBox box = getBoundingBox(track);
    double maxDistance = TRACK_FIT_GAP;
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
        while (prevPoint >= 0 && (track[i] - track[prevPoint]).length() < DELETION_MOVE_MINIMAL_LENGTH) {
            prevPoint--;
        }
        int nextPoint = i + 1;
        while (nextPoint < (int)track.size() && (track[i] - track[nextPoint]).length() < DELETION_MOVE_MINIMAL_LENGTH) {
            nextPoint++;
        }
        if (prevPoint >= 0 && nextPoint < (int)track.size()) {
            Point vec1 = track[nextPoint] - track[i];
            Point vec2 = track[prevPoint] - track[i];
            double ang = acos(Point::dotProduct(vec1, vec2) / (vec1.length() * vec2.length()));
            if (ang <= PI * DELETION_MOVE_MAX_ANGLE / 180) {
                cnt++;
                i = nextPoint;
            }
        }
    }
    std::cout << "deletion cnt = " << cnt << "\n";
    return cnt >= DELETION_MOVE_MIN_COUNT;
}

PFigure recognizeGrabs(const Track &track, Model &model) {
    Point start = track[0];
    Point end = track[track.size() - 1];

    for (auto it = model.begin(); it != model.end(); it++) {
        PFigure figure = *it;
        if (figure->getApproximateDistanceToBorder(start) <= FIGURE_SELECT_GAP) { // grabbed
            // recognize deletion
            if (isDeletionTrack(track)) {
                model.removeFigure(it);
                return figure;
            }

            // try connection
            auto figA = dynamic_pointer_cast<BoundedFigure>(figure);
            if (figA) {
                for (PFigure figure2 : model) {
                    if (figure == figure2) {
                        continue;
                    }
                    auto figB = dynamic_pointer_cast<BoundedFigure>(figure2);
                    if (figB && figB->getApproximateDistanceToBorder(end) <= FIGURE_SELECT_GAP) {
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
    if (siz1 / siz2 < SQUARING_MIN_RATIO) { return; }

    double siz = (siz1 + siz2) / 2;
    box.rightDown = box.leftUp + Point(siz, siz);
    figure->setBoundingBox(box);
}

PFigure recognizeClicks(const Point &click, Model &model) {
    for (PFigure figure : model) {
        if (figure->getApproximateDistanceToBorder(click) > FIGURE_SELECT_GAP) {
            continue;
        }
        model.selectedFigure = figure;

        std::shared_ptr<Segment> segm = dynamic_pointer_cast<Segment>(figure);
        if (segm) {
            if ((click - segm->getA()).length() <= FIGURE_SELECT_GAP) {
                segm->setArrowedA(!segm->getArrowedA());
            }
            if ((click - segm->getB()).length() <= FIGURE_SELECT_GAP) {
                segm->setArrowedB(!segm->getArrowedB());
            }
            return segm;
        }
    }
    return nullptr;
}

/*
 * Consider a continuos polyline. This function returns its center of mass.
 * Please note that this differs from center of mass of all points of track,
 * we calculate weighted sum of segments' centers (weight == length of segment)
 */
Point getWeightedCenter(const Track &track) {
    Point result;
    double summaryLength = 0;
    for (size_t i = 0; i < track.size(); i++) {
        Point a = track[i];
        Point b = track[(i + 1) % track.size()];
        Point middle = (a + b) * 0.5;
        double length = (b - a).length();
        result += middle * length;
        summaryLength += length;
    }
    result = result * (1.0 / summaryLength);
    return result;
}

/*
 * Calculates best rectangle which will fit the track
 * For example, it considers that there may be some
 * outliners (they happen near corners when drawing on touch devices)
 * First, it calculate approximate center of the rectangle and then
 * it calculates average 'width' and 'height' of the rectangle.
 * Points which differ from the center no more than width/4 are
 * considered 'horizontal segments' and vice-versa for vertical.
 */
BoundingBox getBestFitRectangle(const Track &track) {
    BoundingBox total = getBoundingBox(track);
    double maxDx = total.width() / 4;
    double maxDy = total.height() / 4;
    Point center = getWeightedCenter(track);

    double sumDx = 0, sumDy = 0;
    int countDx = 0, countDy = 0;
    for (Point p : track.points) {
        double dx = fabs(p.x - center.x);
        double dy = fabs(p.y - center.y);
        if (dx < maxDx && dy >= maxDy) {
            sumDy += dy;
            countDy++;
        }
        if (dy < maxDy && dx >= maxDx) {
            sumDx += dx;
            countDx++;
        }
    }
    Point diff(
        countDx ? sumDx / countDx : maxDx,
        countDy ? sumDy / countDy : maxDy
    );
    return BoundingBox({ center - diff, center + diff });
}

PFigure recognize(const Track &_track, Model &model) {
    if (FIGURE_SELECT_GAP < 0 || CLOSED_FIGURE_GAP < 0) {
        throw std::logic_error("Recognition preset is not selected");
    }

    // this is to preserve the API (const Track&) and make future optimizations possible
    // i.e. avoid copying if it's not necessary
    Track track = _track;
    if (track.empty()) { return nullptr; }

    // Very small tracks are clicks
    if (getClosedCircumference(track) <= CLOSED_FIGURE_GAP) {
        return recognizeClicks(track[0], model);
    }

    // Moving and connecting
    if (PFigure result = recognizeGrabs(track, model)) {
        return result;
    }

    // Drawing new figures
    std::vector<PFigure> candidates;
    if (!cutToClosed(track))  {
        candidates.push_back(make_shared<Segment>(track[0], track[track.size() - 1]));
    } else {
        candidates.push_back(make_shared<Ellipse>(getBoundingBox(track)));
        candidates.push_back(make_shared<Rectangle>(getBestFitRectangle(track)));
    }

    if (candidates.empty()) { return nullptr; }

    std::vector<double> fits;
    for (PFigure figure : candidates) {
        fits.push_back(fitsToTrack(track, figure));
    }
    size_t id = max_element(fits.begin(), fits.end()) - fits.begin();
    std::cout << "max_fit = " << fits[id] << "; id = " << id << "\n";
    if (fits[id] >= MIN_FIT_POINTS_AMOUNT) { // we allow some of points to fall out of our track
        auto boundedFigure = dynamic_pointer_cast<BoundedFigure>(candidates[id]);
        if (boundedFigure) {
            squareBoundedFigure(boundedFigure);
        }
        model.addFigure(candidates[id]);
        return candidates[id];
    }
    return nullptr;
}
