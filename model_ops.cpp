#include "model_ops.h"

void makeVerticallySymmetric(std::shared_ptr<figures::Curve> curve) {
    for (Point &p : curve->points) {
        std::swap(p.x, p.y);
    }
    makeHorizontallySymmetric(curve);
    for (Point &p : curve->points) {
        std::swap(p.x, p.y);
    }
}

bool getVerticalIntersection(Point a, Point b, double x, Point &result) {
    if (fabs(b.x - a.x) < 1e-8) {
        return false;
    }
    result = Point(x, a.y + (b.y - a.y) * (x - a.x) / (b.x - a.x));
    return true;
}

void makeHorizontallySymmetric(std::shared_ptr<figures::Curve> curve) {
    BoundingBox box = curve->getBoundingBox();
    auto &points = curve->points;
    double midX = box.center().x;
    if (points.at(0).x > midX) {
        for (Point &p : points) {
            p.x = -p.x;
        }
        makeHorizontallySymmetric(curve);
        for (Point &p : points) {
            p.x = -p.x;
        }
        return;
    }
    size_t cnt = 0;
    while (cnt < points.size() && points[cnt].x <= midX) {
        cnt++;
    }

    Point midPoint;
    bool hasMidPoint = getVerticalIntersection(points.at(cnt - 1), points.at(cnt), midX, midPoint);
    points.erase(points.begin() + cnt, points.end());
    bool midArrowBegin = curve->arrowBegin.at(cnt - 1);
    bool midArrowEnd = curve->arrowEnd.at(cnt - 1);

    curve->arrowBegin.erase(curve->arrowBegin.begin() + cnt - 1, curve->arrowBegin.end());
    curve->arrowEnd.erase(curve->arrowEnd.begin() + cnt - 1, curve->arrowEnd.end());

    curve->arrowBegin.push_back(midArrowBegin);
    if (hasMidPoint) {
        curve->arrowEnd.push_back(midArrowEnd);
        points.push_back(midPoint);
        curve->arrowBegin.push_back(midArrowEnd);
    }
    curve->arrowEnd.push_back(midArrowBegin);
    for (int i = cnt - 1; i >= 0; i--) {
        points.push_back(Point(2 * midX - points[i].x, points[i].y));
        if (i > 0) {
            curve->arrowBegin.push_back(curve->arrowEnd[i - 1]);
            curve->arrowEnd.push_back(curve->arrowBegin[i - 1]);
        }
    }
    curve->selfCheck();
}
