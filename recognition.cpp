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
const int DELETION_MOVE_MAX_ANGLE = 30;
const int DELETION_MOVE_MIN_COUNT = 3;
const int DELETION_MAX_TIME = 1000;
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

// (amount of point fitted, -(average error))
std::pair<double, double> fitsToTrack(const Track &track, const PFigure &figure) {
    BoundingBox box = getBoundingBox(track);
    double maxDistanceX = std::max((double)TRACK_FIT_GAP, box.width() * 0.1);
    double maxDistanceY = std::max((double)TRACK_FIT_GAP, box.height() * 0.1);
    double failDistanceX = maxDistanceX * 2;
    double failDistanceY = maxDistanceY * 2;

    int goodCount = 0;
    double averageError = 0;
    for (Point p : track.points) {
        Point p2 = figure->getApproximateNearestPointOnBorder(p);
        double dx = fabs(p.x - p2.x);
        double dy = fabs(p.y - p2.y);
        if (dx > failDistanceX || dy > failDistanceY) { return std::make_pair(0, 0); }
        goodCount += dx <= maxDistanceX && dy <= maxDistanceY;

        dx /= maxDistanceX;
        dy /= maxDistanceY;
        double error = dx * dx + dy * dy;
        averageError += error;
    }
    averageError /= track.size();
    averageError = sqrt(averageError);

    return std::make_pair(goodCount * 1.0 / track.size(), -averageError);
}

using namespace figures;

double getAngle(Point v1, Point v2) {
    Point v(Point::dotProduct(v1, v2), Point::crossProduct(v1, v2));
    if (v == Point()) return NAN;
    return atan2(v.y, v.x);
}

bool isDeletionTrack(const Track &track) {
    std::vector<int> stops = getSpeedBreakpoints(track);
    if (track.points.back().time > DELETION_MAX_TIME) {
        return false;
    }
    int cnt = 0;
    for (int i = 1; i + 1 < (int)track.size(); i++) {
        Point v1 = track[i - 1] - track[i];
        Point v2 = track[i + 1] - track[i];
        double ang = fabs(getAngle(v1, v2));
        if (std::isnan(ang)) continue;
        ang = std::min(ang, 2 * PI - ang);
        if (ang < DELETION_MOVE_MAX_ANGLE * PI / 180) {
            cnt++;
        }
    }
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
    if (siz1 / siz2 < SQUARING_MIN_RATIO) { return; }

    double siz = (siz1 + siz2) / 2;
    box.rightDown = box.leftUp + Point(siz, siz);
    figure->setBoundingBox(box);
}

struct SelectionFit {
    bool isArrowable;
    double distance;
    PFigure figure;

    SelectionFit() : isArrowable(false), distance(HUGE_VAL), figure(nullptr) {}
    bool operator<(const SelectionFit &other) const {
        if (isArrowable != other.isArrowable) { return isArrowable > other.isArrowable; }
        return distance < other.distance;
    }
};
PFigure recognizeClicks(const Point &click, Model &model) {
    SelectionFit bestFit;
    for (PFigure figure : model) {
        SelectionFit currentFit;
        currentFit.isArrowable = !!dynamic_pointer_cast<Segment>(figure) || !!dynamic_pointer_cast<Curve>(figure);
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

    std::shared_ptr<Curve> curve = dynamic_pointer_cast<Curve>(model.selectedFigure);
    if (curve) {
        std::pair<double, size_t> nearestSegment(INFINITY, 0);
        for (size_t i = 0; i < curve->arrowBegin.size(); i++) {
            Segment s(curve->points[i], curve->points[i + 1]);
            double currentDistance = s.getApproximateDistanceToBorder(click);
            nearestSegment = std::min(nearestSegment, std::make_pair(currentDistance, i));
        }
        if (nearestSegment.first <= FIGURE_SELECT_GAP) {
            size_t i = nearestSegment.second;
            Point a = curve->points[i], b = curve->points[i + 1];
            if ((click - a).length() <= FIGURE_SELECT_GAP) {
                curve->arrowBegin[i] = !curve->arrowBegin[i];
            }
            if ((click - b).length() <= FIGURE_SELECT_GAP) {
                curve->arrowEnd[i] = !curve->arrowEnd[i];
            }
        }
        return curve;
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

bool hasStopNear(const Track &track, const std::vector<int> &stops, const Point &goal) {
    const double MAX_STOP_DISTANCE = getClosedFigureGap(track) * 2;
    for (int id : stops) {
        Point p = track[id];
        if ((p - goal).length() <= MAX_STOP_DISTANCE) {
            return true;
        }
    }
    return false;
}

std::vector<int> getSpeedBreakpoints(const Track &track, const double SPEED_STOP_THRESHOLD);

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
        std::vector<int> stops = getSpeedBreakpoints(track);
        if (stops.size() >= 2) {
            Point start = track[0], end = track[track.size() - 1];
            bool ok = true;
            for (Point p : { start, end }) {
                ok &= hasStopNear(track, stops, p);
            }
            if (ok) {
                candidates.push_back(make_shared<Segment>(track[0], track[track.size() - 1]));
            }
        }
    } else {
        candidates.push_back(make_shared<Ellipse>(getBoundingBox(track)));

        std::vector<int> stops = getSpeedBreakpoints(track);
        // check that there were stops in corners
        BoundingBox rect = getBestFitRectangle(track);
        bool ok = stops.size() >= 4;
        for (Point corner : { rect.leftUp, rect.rightDown, rect.leftDown(), rect.rightUp() }) {
            ok &= hasStopNear(track, stops, corner);
        }
        if (ok) {
            candidates.push_back(make_shared<Rectangle>(rect));
        }
    }

    if (candidates.empty()) { return nullptr; }

    std::vector<std::pair<double, double> > fits;
    for (PFigure figure : candidates) {
        fits.push_back(fitsToTrack(track, figure));
    }
    size_t id = max_element(fits.begin(), fits.end()) - fits.begin();
    if (fits[id].first >= MIN_FIT_POINTS_AMOUNT) { // we allow some of points to fall out of our track
        auto boundedFigure = dynamic_pointer_cast<BoundedFigure>(candidates[id]);
        if (boundedFigure) {
            squareBoundedFigure(boundedFigure);
        }
        model.addFigure(candidates[id]);
        return candidates[id];
    }

    // custom speed threshold for stops breakpoints
    std::vector<int> stops = getSpeedBreakpoints(uncuttedTrack, 0.02);
    stops.insert(stops.begin(), 0);
    stops.push_back(uncuttedTrack.size() - 1);
    stops.erase(unique(stops.begin(), stops.end()), stops.end());

    std::vector<Point> points;
    std::vector<int> curveStops;
    for (size_t i = 0; i + 1 < stops.size(); i++) {
        int a = stops[i], b = stops[i + 1];
        std::vector<Point> currentSegment;
        for (int i2 = a; i2 <= b; i2++) {
            currentSegment.push_back(uncuttedTrack[i2]);
        }
        currentSegment = smoothCurve(currentSegment);
        currentSegment.erase(unique(currentSegment.begin(), currentSegment.end()), currentSegment.end());

        // there is no need to add first point on all iterations except the very first
        points.insert(points.end(), currentSegment.begin() + (i > 0), currentSegment.end());
        curveStops.push_back(points.size() - currentSegment.size());
    }
    auto result = make_shared<Curve>(points);
    for (int stop : curveStops) {
        result->isStop.at(stop) = true;
    }
    model.addFigure(result);
    return result;
}

PFigure findClickedFigure(const Model &model, const Point &click) {
    double nearest = INFINITY;
    PFigure answer;
    for (PFigure figure : model) {
        double distance = figure->getApproximateDistanceToBorder(click);
        if (distance <= FIGURE_SELECT_GAP && distance < nearest) {
            nearest = distance;
            answer = figure;
        }
    }
    return answer;
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
    double p90 = percentiles[percentiles.size() * 9 / 10];
    for (auto &x : res) {
        x = x / p90;
        if (std::isinf(x) || std::isnan(x)) { x = 0.5; }
        x = std::max(x, 0.0);
        x = std::min(x, 1.0);
    }
    return res;
}

std::vector<int> getSpeedBreakpoints(const Track &track, const double SPEED_STOP_THRESHOLD) {
    std::vector<double> speeds = calculateRelativeSpeeds(track);
    if (speeds.empty()) { return std::vector<int>(); }

    const double STOP_AREA = 15;

    std::vector<int> stops;
    for (size_t i = 0; i < speeds.size(); i++) {
        if (speeds[i] > SPEED_STOP_THRESHOLD) continue;

        Point current = track[i];
        int left = i, right = i;
        bool ok = true;
        while (left  >= 0                  && (track[left ] - current).length() <= STOP_AREA) { ok = ok && speeds[left]  >= speeds[i]; left--;  }
        while (right <  (int)speeds.size() && (track[right] - current).length() <= STOP_AREA) { ok = ok && speeds[right] >= speeds[i]; right++; }
        if (ok) {
            stops.push_back(i);
            while (i < track.size() && (track[i] - current).length() <= STOP_AREA) {
                i++;
            }
            i--;
        }
    }
    return stops;
}

std::vector<int> getSpeedBreakpoints(const Track &track) {
    return getSpeedBreakpoints(track, 0.07); // default for figures
}
