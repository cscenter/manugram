#include <QtTest/QtTest>
#include <QDebug>
#include "model.h"
#include "model_io.h"

using namespace figures;

class Tests : public QObject {
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
    void testInsideSegment() {
        Segment s(Point(11, 4), Point(14, 1));
        QVERIFY( s.isInsideOrOnBorder(Point(11, 4)));
        QVERIFY(!s.isInsideOrOnBorder(Point(11, 4.001)));
        QVERIFY(!s.isInsideOrOnBorder(Point(10.999, 4)));
        QVERIFY( s.isInsideOrOnBorder(Point(14, 1)));
        QVERIFY(!s.isInsideOrOnBorder(Point(13.999, 1)));
        QVERIFY(!s.isInsideOrOnBorder(Point(14, 0.999)));
        QVERIFY(!s.isInsideOrOnBorder(Point(13.999, 0.999)));
        QVERIFY( s.isInsideOrOnBorder(Point(12, 3)));
        QVERIFY( s.isInsideOrOnBorder(Point(13, 2)));
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
    void testInsideEllipse() {
        Ellipse e({ Point(11, 1), Point(17, 5) });
        QVERIFY(!e.isInsideOrOnBorder(Point(11, 1)));
        QVERIFY(!e.isInsideOrOnBorder(Point(11.5, 1.5)));
        QVERIFY(!e.isInsideOrOnBorder(Point(17, 5)));
        QVERIFY(!e.isInsideOrOnBorder(Point(11, 5)));
        QVERIFY(e.isInsideOrOnBorder(Point(14, 5)));
        QVERIFY(e.isInsideOrOnBorder(Point(14, 1)));
        QVERIFY(e.isInsideOrOnBorder(Point(14, 3)));
        QVERIFY(e.isInsideOrOnBorder(Point(15, 3.2)));
        QVERIFY( e.isInsideOrOnBorder(Point(16.1, 4.4)));
        QVERIFY(!e.isInsideOrOnBorder(Point(16.2, 4.4)));
        QVERIFY(!e.isInsideOrOnBorder(Point(16.1, 4.5)));
    }
    void testDistanceToEllipseBorder() {
        const double EPS = 1e-8;
        Ellipse e({ Point(11, 1), Point(17, 5) });
        QVERIFY(e.getApproximateDistanceToBorder(Point(11, 1)) <= sqrt(9 + 4) + EPS);
        QVERIFY(e.getApproximateDistanceToBorder(Point(11, 3)) <= EPS);
        QVERIFY(e.getApproximateDistanceToBorder(Point(17, 3)) <= EPS);
        QVERIFY(e.getApproximateDistanceToBorder(Point(14, 1)) <= EPS);
        QVERIFY(e.getApproximateDistanceToBorder(Point(14, 5)) <= EPS);
        QVERIFY(e.getApproximateDistanceToBorder(Point(10, 3)) >= 1 - EPS);
        QVERIFY(e.getApproximateDistanceToBorder(Point(14, 0)) >= 1 - EPS);
        QVERIFY(fabs(e.getApproximateDistanceToBorder(Point(14, 3)) - 2) < EPS);

        Ellipse e2({ Point(11, 1), Point(15, 7) });
        QVERIFY(fabs(e2.getApproximateDistanceToBorder(Point(13, 4)) - 2) < EPS);
    }

    void testSimpleSaveLoad() {
        int testId = 1;
        for (;; testId++) {
            char resourceName[64];
            snprintf(resourceName, sizeof resourceName, ":/tests/%02d.model", testId);
            QFile file(resourceName);
            if (!file.open(QFile::ReadOnly)) {
                break;
            }
            qDebug() << "Trying" << resourceName;

            QByteArray inData = file.readAll();
            std::stringstream inDataStream;
            inDataStream << inData.toStdString();
            Model model;
            inDataStream >> model;

            std::stringstream outDataStream;
            outDataStream << model;
            std::string outStr = outDataStream.str();
            QByteArray outData(outStr.data(), outStr.length());
            QCOMPARE(outData, inData);
        }
        QVERIFY(testId > 1);
    }
};

QTEST_MAIN(Tests)
#include "tests.moc"
