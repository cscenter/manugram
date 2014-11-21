#include <QtTest/QtTest>
#include "model.h"

using namespace figures;

class TestFiguresDistances : public QObject {
    Q_OBJECT
private slots:
    void testDistanceToSegmentBorder() {
        const double EPS = 1e-8;
        Segment s(Point(11, 1), Point(14, 4));
        QVERIFY(fabs(s.getDistanceToBorder(Point(11, 1)) - 0) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(11, 0)) - 1) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(10, 0)) - sqrt(2)) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(14, 4)) - 0) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(15, 4)) - 1) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(15, 5)) - sqrt(2)) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(12, 2)) - 0) < EPS);
        QVERIFY(fabs(s.getDistanceToBorder(Point(14, 2)) - sqrt(2)) < EPS);
    }
};

QTEST_MAIN(TestFiguresDistances)
#include "tests.moc"
