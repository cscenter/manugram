#include <QtTest/QtTest>
#include "model.h"

using namespace figures;

class TestFiguresDistances : public QObject {
    Q_OBJECT
private slots:
    void testDistanceToSegmentBorder() {
        const double EPS = 1e-8;
        Segment s(Point(11, 1), Point(14, 4));
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(11, 1)) - 0) < EPS);
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(11, 0)) - 1) < EPS);
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(10, 0)) - sqrt(2)) < EPS);
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(14, 4)) - 0) < EPS);
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(15, 4)) - 1) < EPS);
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(15, 5)) - sqrt(2)) < EPS);
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(12, 2)) - 0) < EPS);
        QVERIFY(fabs(s.getApproximateDistanceToBorder(Point(14, 2)) - sqrt(2)) < EPS);
    }
    void testDistanceToRectangleBorder() {
        const double EPS = 1e-8;
        Rectangle r({ Point(11, 1), Point(14, 4) });
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(11, 1)) - 0) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(11, 2)) - 0) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(11, 4)) - 0) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(13, 4)) - 0) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(15, 4)) - 1) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(15, 2)) - 1) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(12, 0)) - 1) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(12, 2)) - 1) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(12.5, 2)) - 1) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(13.5, 2)) - 0.5) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(12.5, 3)) - 1) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(10, 3)) - 1) < EPS);

        // Test distance to corners
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(15, 5)) - sqrt(2)) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(16, 5)) - sqrt(5)) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(10, -1)) - sqrt(5)) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(10, 6)) - sqrt(5)) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(16, 5)) - sqrt(5)) < EPS);
        QVERIFY(fabs(r.getApproximateDistanceToBorder(Point(16, 0)) - sqrt(5)) < EPS);
    }
};

QTEST_MAIN(TestFiguresDistances)
#include "tests.moc"
