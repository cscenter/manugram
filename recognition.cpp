#include "model.h"
#include "recognition.h"
#include <memory>
#include <algorithm>
#include <vector>
#include <cmath>

using std::min;
using std::max;
using std::make_shared;
using std::dynamic_pointer_cast;

int FIGURE_SELECT_GAP = -1;
int MIN_CLOSED_FIGURE_GAP = -1;
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
        MIN_CLOSED_FIGURE_GAP = 10;
        break;
    case RecognitionPreset::Touch:
        FIGURE_SELECT_GAP = 30;
        MIN_CLOSED_FIGURE_GAP = 50;
        break;
    };
}

int figureSelectGap() {
    return FIGURE_SELECT_GAP;
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

double getClosedFigureGap(const BoundingBox &box) {
    return std::max(
                (double)MIN_CLOSED_FIGURE_GAP,
                std::max(box.width(), box.height()) * 0.1
                );
}

double getClosedFigureGap(const Track &track) {
    return getClosedFigureGap(getBoundingBox(track));
}
double getClosedFigureGap(const std::vector<Point> &points) {
    BoundingBox box;
    for (Point p : points) {
        box.addPoint(p);
    }
    return getClosedFigureGap(box);
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

bool cutToClosedFromEnd(Track &track) {
    auto &points = track.points;
    auto it = points.begin();
    const double CLOSED_FIGURE_GAP = getClosedFigureGap(track);
    if ((points[0] - points.back()).length() > CLOSED_FIGURE_GAP * 3) {
        return false;
    }
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

bool cutToClosed(Track &track) {
    if (cutToClosedFromEnd(track)) return true;

    reverse(track.points.begin(), track.points.end());
    bool result = cutToClosedFromEnd(track);
    reverse(track.points.begin(), track.points.end());
    return result;
}

double fitsToTrack(const Track &track, const PFigure &figure) {
    BoundingBox box = getBoundingBox(track);
    double maxDistanceX = std::max((double)TRACK_FIT_GAP, box.width() * 0.1);
    double maxDistanceY = std::max((double)TRACK_FIT_GAP, box.height() * 0.1);
    double failDistanceX = maxDistanceX * 2;
    double failDistanceY = maxDistanceY * 2;

    int goodCount = 0;
    for (Point p : track.points) {
        Point p2 = figure->getApproximateNearestPointOnBorder(p);
        double dx = fabs(p.x - p2.x);
        double dy = fabs(p.y - p2.y);
        if (dx > failDistanceX || dy > failDistanceY) { return 0; }
        goodCount += dx <= maxDistanceX && dy <= maxDistanceY;
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

PFigure recognizeConnections(const Track &track, Model &model) {
    Point start = track[0];
    Point end = track[track.size() - 1];

    for (auto it = model.begin(); it != model.end(); it++) {
        PFigure figure = *it;
        auto figA = dynamic_pointer_cast<BoundedFigure>(figure);
        if (!figA) { continue; }
        if (!figure->isInsideOrOnBorder(start)) { continue; }
        for (PFigure figure2 : model) {
            if (figure == figure2) {
                continue;
            }
            auto figB = dynamic_pointer_cast<BoundedFigure>(figure2);
            if (figB && figB->isInsideOrOnBorder(end)) {
                auto result = make_shared<SegmentConnection>(figA, figB);
                model.addFigure(result);
                return result;
            }
        }
    }
    return nullptr;
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
            // but only if figure was selected previously (#65)
            if (figure == model.selectedFigure) {
                figure->translate(end - start);
                model.recalculate();
                return figure;
            }
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

struct SelectionFit {
    bool isSegment;
    double distance;
    PFigure figure;

    SelectionFit() : isSegment(false), distance(HUGE_VAL), figure(nullptr) {}
    bool operator<(const SelectionFit &other) const {
        if (isSegment != other.isSegment) { return isSegment > other.isSegment; }
        return distance < other.distance;
    }
};
PFigure recognizeClicks(const Point &click, Model &model) {
    SelectionFit bestFit;
    for (PFigure figure : model) {
        SelectionFit currentFit;
        currentFit.isSegment = !!dynamic_pointer_cast<Segment>(figure);
        currentFit.distance = figure->getApproximateDistanceToBorder(click);
        currentFit.figure = figure;
        if (currentFit.distance >= FIGURE_SELECT_GAP) { continue; }
        bestFit = min(bestFit, currentFit);
    }
    if (bestFit.figure) {
        model.selectedFigure = bestFit.figure;
    } else {
        bool found = false;
        for (PFigure figure : model) {
            if (figure->isInsideOrOnBorder(click)) {
                model.selectedFigure = figure;
                found = true;
            }
        }
        if (!found) {
            model.selectedFigure = nullptr;
        }
    }

    std::shared_ptr<Segment> segm = dynamic_pointer_cast<Segment>(model.selectedFigure);
    if (segm) {
        if ((click - segm->getA()).length() <= FIGURE_SELECT_GAP) {
            segm->setArrowedA(!segm->getArrowedA());
        }
        if ((click - segm->getB()).length() <= FIGURE_SELECT_GAP) {
            segm->setArrowedB(!segm->getArrowedB());
        }
        return segm;
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

std::vector<Point> smoothCurve(std::vector<Point> current) {
    assert(!current.empty());
    const double CLOSED_FIGURE_GAP = getClosedFigureGap(current);

    Point start = current[0];
    Point end = current.back();
    if ((start - end).length() <= CLOSED_FIGURE_GAP) {
        end = current.back() = start;
    }
    Segment segment(start, end);

    std::pair<double, size_t> maxDistance(0, 0);
    for (size_t i = 0; i < current.size(); i++) {
        double d = segment.getApproximateDistanceToBorder(current[i]);
        maxDistance = max(maxDistance, std::make_pair(d, i));
    }

    std::vector<Point> result;
    if (maxDistance.first > TRACK_FIT_GAP) {
        auto part1 = smoothCurve(std::vector<Point>(current.begin(), current.begin() + maxDistance.second + 1));
        auto part2 = smoothCurve(std::vector<Point>(current.begin() + maxDistance.second, current.end()));
        result.insert(result.end(), part1.begin(), part1.end());
        result.insert(result.end(), part2.begin() + 1, part2.end());
    } else {
        result.push_back(start);
        result.push_back(end);
    }
    return result;
}

PFigure recognize(const Track &_track, Model &model) {
    if (FIGURE_SELECT_GAP < 0 || MIN_CLOSED_FIGURE_GAP < 0) {
        throw std::logic_error("Recognition preset is not selected");
    }

    // this is to preserve the API (const Track&) and make future optimizations possible
    // i.e. avoid copying if it's not necessary
    Track track = _track;
    if (track.empty()) { return nullptr; }

    const double CLOSED_FIGURE_GAP = getClosedFigureGap(track);
    // Very small tracks are clicks
    if (getClosedCircumference(track) <= CLOSED_FIGURE_GAP) {
        return recognizeClicks(track[0], model);
    }

    // Moving and connecting
    if (PFigure result = recognizeGrabs(track, model)) {
        return result;
    }

    // Connecting interior-->interior
    if (PFigure result = recognizeConnections(track, model)) {
        return result;
    }

    // Drawing new figures
    std::vector<PFigure> candidates;
    Track uncuttedTrack = track;
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

    std::vector<Point> points(uncuttedTrack.points.begin(), uncuttedTrack.points.end());
    auto result = make_shared<Curve>(smoothCurve(points));
    model.addFigure(result);
    return result;
}

std::vector<double> calculateRelativeSpeeds(const Track &track) {
    std::vector<double> res;
    for (size_t i = 0; i + 1 < track.size(); i++) {
        double len = (track[i + 1] - track[i]).length();
        double speed = len / (track[i + 1].time - track[i].time);
        if (std::isinf(speed) || std::isnan(speed)) { speed = INFINITY; }
        res.push_back(speed);
    }
    if (res.empty()) { return res; }

    auto percentiles = res;
    sort(percentiles.begin(), percentiles.end());
    double p10 = percentiles[percentiles.size() / 10];
    double p90 = percentiles[percentiles.size() * 9 / 10];
    for (auto &x : res) {
        x = (x - p10) / (p90 - p10);
        if (std::isinf(x) || std::isnan(x)) { x = 0.5; }
        x = std::max(x, 0.0);
        x = std::min(x, 1.0);
    }
    return res;
}
