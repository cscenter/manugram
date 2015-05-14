#include "model_ops.h"
#include <set>
#include <deque>
#include <vector>

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
    assert(cnt >= 1);
    assert(cnt < points.size());

    Point midPoint;
    bool hasMidPoint = getVerticalIntersection(points.at(cnt - 1), points.at(cnt), midX, midPoint);
    points.erase(points.begin() + cnt, points.end());
    curve->isStop.erase(curve->isStop.begin() + cnt, curve->isStop.end());

    bool midArrowBegin = curve->arrowBegin.at(cnt - 1);
    bool midArrowEnd = curve->arrowEnd.at(cnt - 1);

    curve->arrowBegin.erase(curve->arrowBegin.begin() + cnt - 1, curve->arrowBegin.end());
    curve->arrowEnd.erase(curve->arrowEnd.begin() + cnt - 1, curve->arrowEnd.end());

    curve->arrowBegin.push_back(midArrowBegin);
    if (hasMidPoint) {
        curve->arrowEnd.push_back(midArrowEnd);
        points.push_back(midPoint);
        curve->isStop.push_back(false);
        curve->arrowBegin.push_back(midArrowEnd);
    }
    curve->arrowEnd.push_back(midArrowBegin);
    for (int i = cnt - 1; i >= 0; i--) {
        points.push_back(Point(2 * midX - points[i].x, points[i].y));
        curve->isStop.push_back(curve->isStop[i]);
        if (i > 0) {
            curve->arrowBegin.push_back(curve->arrowEnd[i - 1]);
            curve->arrowEnd.push_back(curve->arrowBegin[i - 1]);
        }
    }
    curve->selfCheck();
}

void makeTopBottomTree(Model &model, figures::PBoundedFigure root) {
    typedef figures::PBoundedFigure Node;
    std::map<Node, std::vector<Node>> edges;
    // building graph
    for (PFigure figure : model) {
        auto connection = std::dynamic_pointer_cast<figures::SegmentConnection>(figure);
        if (connection) {
            bool dirAB = connection->getArrowedB();
            bool dirBA = connection->getArrowedA();
            if (!dirAB && !dirBA) {
                dirAB = dirBA = true;
            }
            if (dirAB) {
                edges[connection->getFigureA()].push_back(connection->getFigureB());
            }
            if (dirBA) {
                edges[connection->getFigureB()].push_back(connection->getFigureA());
            }
        }
    }

    const double NODES_GAP_K = 0.5; // 0.5 of node's height

    std::map<Node, std::vector<Node>> children;
    std::vector<Node> order;

    // breadth-first-search, building tree itself
    {
        std::set<Node> visited;
        std::deque<Node> q;

        q.push_back(root);
        visited.insert(root);
        while (!q.empty()) {
            Node v = q.front();
            q.pop_front();
            order.push_back(v);
            for (Node child : edges[v]) {
                if (visited.count(child)) { continue; }
                visited.insert(child);
                children[v].push_back(child);
                q.push_back(child);
            }
        }
    }

    std::map<Node, BoundingBox> boxes;
    // traversing nodes from the bottom to the top
    for (int i = order.size() - 1; i >= 0; i--) {
        Node v = order[i];
        BoundingBox &box = boxes[v] = v->getBoundingBox();
        const double NODES_GAP = box.height() * NODES_GAP_K;

        if (!children[v].empty()) {
            double sumChildrenWidth = 0;
            double maxChildHeight = 0;
            for (Node child : children[v]) {
                sumChildrenWidth += boxes[child].width();
                maxChildHeight = std::max(maxChildHeight, boxes[child].height());
            }
            if (children[v].size() >= 2) {
                sumChildrenWidth += (children[v].size()  - 1) * NODES_GAP;
            }
            box.rightDown.x = std::max(box.rightDown.x, box.leftUp.x + sumChildrenWidth);
            box.rightDown.y += NODES_GAP + maxChildHeight;
        }
    }

    // returns offset by which node should be translated to be 'root node' with corresponding sumBox
    const auto getNodeOffset = [](const BoundingBox &vBox, const BoundingBox &sumBox) {
        Point offset;
        offset.x = sumBox.center().x - vBox.center().x;
        offset.y = sumBox.leftUp.y - vBox.leftUp.y;
        return offset;
    };

    // traversing tree from the top to the bottom and arranging nodes
    // invariant: when visiting node X, its bounding box is correct

    { // first, we need to keep the root in its original place
        Node v = order[0];
        BoundingBox vBox = v->getBoundingBox();
        BoundingBox &sumBox = boxes[v];
        sumBox.translate(Point() - getNodeOffset(vBox, sumBox));
    }
    for (size_t i = 0; i < order.size(); i++) {
        Node v = order[i];
        BoundingBox vBox = v->getBoundingBox();
        const double NODES_GAP = vBox.height() * NODES_GAP_K;

        BoundingBox sumBox = boxes[v];
        v->translate(getNodeOffset(vBox, sumBox));
        Point currentCorner = sumBox.leftUp;
        currentCorner.y += vBox.height() + NODES_GAP;
        for (Node child : children[v]) {
            BoundingBox &childBox = boxes[child];
            childBox.translate(currentCorner - childBox.leftUp);
            currentCorner.x += childBox.width() + NODES_GAP;
        }
    }
    model.recalculate();
}
