#include <QtTest/QtTest>
#include <QDebug>
#include <random>
#include <typeinfo>
#include "model.h"
#include "model_io.h"

using namespace figures;

using std::make_shared;
using std::dynamic_pointer_cast;

class ModelModifier {
public:
    ModelModifier(Model &model)
        : model(model)
        , coordGen(MIN_COORD, MAX_COORD)
        , curveLengthGen(2, 20)
        , labelLengthGen(1, 20)
        , labelLetterGen('a', 'z')
    {}

    void doRandom() {
        int operation = randint(1, 4);
        if (operation == 1) {
            removeFigure();
        } else {
            addFigure();
        }
    }

    void addFigure() {
        int type = randint(0, 4);
        Point a = genPoint();
        Point b = genPoint();

        PFigure figure;
        switch (type) {
        case 0: {
            auto segment = make_shared<Segment>(a, b);
            segment->setArrowedA(randint(0, 1));
            segment->setArrowedB(randint(0, 1));
            figure = segment;
        }   break;
        case 1:
            figure = make_shared<Ellipse>(BoundingBox({ a, b }));
            break;
        case 2:
            figure = make_shared<Rectangle>(BoundingBox({ a, b }));
            break;
        case 3: {
            int len = curveLengthGen(generator);
            std::vector<Point> points;
            for (int i = 0; i < len; i++) {
                points.push_back(genPoint());
            }
            figure = make_shared<Curve>(points);
        }   break;
        case 4: {
            auto figA = getRandomBoundedFigure();
            auto figB = getRandomBoundedFigure();
            if (!figA || !figB) {
                return;
            }
            auto segment = make_shared<SegmentConnection>(figA, figB);
            segment->setArrowedA(randint(0, 1));
            segment->setArrowedB(randint(0, 1));
            figure = segment;
        }
        }
        if (randint(0, 1) == 0) {
            figure->setLabel(genLabel());
        }
        model.addFigure(figure);
    }

    void removeFigure() {
        if (model.size() == 0) {
            return;
        }
        int id = randint(0, model.size() - 1);
        for (auto it = model.begin(); it != model.end(); it++, id--) {
            if (id == 0) {
                model.removeFigure(it);
                return;
            }
        }
    }

private:
    std::default_random_engine generator;
    Model &model;

    PBoundedFigure getRandomBoundedFigure() {
        size_t count = 0;
        for (auto fig : model) {
            auto result = dynamic_pointer_cast<BoundedFigure>(fig);
            if (result) { count++; }
        }
        if (!count) { return nullptr; }
        int id = randint(0, count - 1);
        for (auto fig : model) {
            auto result = dynamic_pointer_cast<BoundedFigure>(fig);
            if (result) {
                if (id == 0) {
                    return result;
                } else {
                    id--;
                }
            }
        }
        abort();
    }

    static const int MIN_COORD = -1e5;
    static const int MAX_COORD =  1e5;
    std::uniform_real_distribution<> coordGen;
    std::uniform_int_distribution<> curveLengthGen;
    std::uniform_int_distribution<> labelLengthGen, labelLetterGen;

    int randint(int l, int r) {
        return std::uniform_int_distribution<>(l, r)(generator);
    }

    Point genPoint() {
        double x = coordGen(generator);
        double y = coordGen(generator);
        return Point(x, y);
    }
    std::string genLabel() {
        int len = labelLengthGen(generator);
        std::string result = "";
        for (int i = 0; i < len; i++) {
            result += labelLetterGen(generator);
        }
        return result;
    }
};

bool modelsAreEqual(const Model &a, const Model &b) {
    if (a.size() != b.size()) {
        return false;
    }
    auto ita = a.begin(), itb = b.begin();
    for (; ita != a.end() && itb != b.end(); ita++, itb++) {
        // one dereference from iterator, second dereference from shared_ptr
        Figure &a = **ita;
        Figure &b = **itb;
        if (typeid(a) != typeid(b)) {
            return false;
        }
    }
    assert(ita == a.end() && itb == b.end());
    return true;
}

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

    void testStressSaveLoad() {
        const int PASSES = 10;
        for (int pass = 0; pass < PASSES; pass++) {
            qDebug("pass %d/%d", pass + 1, PASSES);
            Model model;
            ModelModifier modifier(model);
            for (int iteration = 0; iteration < 50; iteration++) {
                std::stringstream data;
                data << model;

                std::string saved1 = data.str();

                Model restored;
                data >> restored;

                std::stringstream data2;
                data2 << restored;

                Model restored2;
                data2 >> restored2;

                std::string saved2 = data.str();
                QCOMPARE(saved1, saved2);
                QVERIFY(modelsAreEqual(model, restored2));
                modifier.doRandom();
            }
        }
    }
};

QTEST_MAIN(Tests)
#include "tests.moc"
