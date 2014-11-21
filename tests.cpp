#include <QtTest/QtTest>
#include "model.h"

using namespace figures;

class TestFiguresDistances : public QObject {
    Q_OBJECT
private slots:
    void testDistanceToSegmentBorder() {
        const double EPS = 1e-8;
        Segment s(Point(1, 1), Point(4, 4));
        QVERIFY(fabs(s.getDistanceToBorder(Point(1, 1)) - 0) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(1, 0)) - 1) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(0, 0)) - sqrt(2)) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(4, 4)) - 0) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(5, 4)) - 1) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(5, 5)) - sqrt(2)) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(2, 2)) - 0) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(4, 2)) - sqrt(2)) < EPS);
    }
};

QTEST_MAIN(TestFiguresDistances)
#include "tests.moc"
