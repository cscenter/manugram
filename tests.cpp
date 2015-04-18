#include <QtTest/QtTest>
#include <QDebug>
#include <random>
#include <typeinfo>
#include "model.h"
#include "model_io.h"
#include <fstream>

using namespace figures;

using std::make_shared;
using std::dynamic_pointer_cast;

class ModelModifier {
public:
    ModelModifier(Model &model, std::default_random_engine::result_type seed)
        : model(model)
        , generator(seed)
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
    Model &model;
    std::default_random_engine generator;

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
        double x = round(coordGen(generator) * 10) / 10;
        double y = round(coordGen(generator) * 10) / 10;
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

class FiguresComparator : public FigureVisitor {
public:
    FiguresComparator(const Figure &other, const std::map<PFigure, PFigure> &othersMapping) : other(other), othersMapping(othersMapping), _result(false) {}
    bool result() const { return _result; }

    virtual void accept(Segment &segm1) override {
        _result = false;
        auto &segm2 = dynamic_cast<const Segment&>(other);
        if (segm1.label() != segm2.label()) { return; }
        if (segm1.getArrowedA() != segm2.getArrowedA()) { return; }
        if (segm1.getArrowedB() != segm2.getArrowedB()) { return; }
        _result =
                segm1.getA() == segm2.getA() &&
                segm1.getB() == segm2.getB();
    }

    virtual void accept(SegmentConnection &segm1) override {
        _result = false;
        auto &segm2 = dynamic_cast<const SegmentConnection&>(other);
        if (segm1.label() != segm2.label()) { return; }
        if (segm1.getArrowedA() != segm2.getArrowedA()) { return; }
        if (segm1.getArrowedB() != segm2.getArrowedB()) { return; }
        _result =
                segm1.getFigureA() == othersMapping.at(segm2.getFigureA()) &&
                segm1.getFigureB() == othersMapping.at(segm2.getFigureB());
    }

    virtual void accept(Curve &curve1) override {
        _result = false;
        auto &curve2 = dynamic_cast<const Curve&>(other);
        if (curve1.label() != curve2.label()) { return; }
        _result = curve1.points == curve2.points;
    }

    virtual void accept(figures::Ellipse &fig1) override {
        _result = false;
        auto &fig2 = dynamic_cast<const Ellipse&>(other);
        if (fig1.label() != fig2.label()) { return; }
        _result = fig1.getBoundingBox() == fig2.getBoundingBox();
    }

    virtual void accept(figures::Rectangle &fig1) override {
        _result = false;
        auto &fig2 = dynamic_cast<const Rectangle&>(other);
        if (fig1.label() != fig2.label()) { return; }
        _result = fig1.getBoundingBox() == fig2.getBoundingBox();
    }

private:
    const Figure &other;
    const std::map<PFigure, PFigure> &othersMapping;
    bool _result;
};

bool modelsAreEqual(const Model &a, const Model &b) {
    if (a.size() != b.size()) {
        return false;
    }

    std::map<PFigure, PFigure> others;

    auto ita = a.begin(), itb = b.begin();
    for (; ita != a.end() && itb != b.end(); ita++, itb++) {
        // one dereference from iterator, second dereference from shared_ptr
        Figure &a = **ita;
        Figure &b = **itb;
        if (typeid(a) != typeid(b)) {
            return false;
        }
        FiguresComparator cmp(b, others);
        a.visit(cmp);
        if (!cmp.result()) {
            return false;
        }
        assert(!others.count(*itb));
        others[*itb] = *ita;
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
            if (!file.open(QFile::ReadOnly | QFile::Text)) {
                break;
            }
            qDebug() << "Trying" << resourceName;

            QByteArray inData = file.readAll();
            std::stringstream inDataStream;
            inDataStream << inData.toStdString();

            Model model;
            try {
                inDataStream >> model;
            } catch (model_format_error &e) {
                qDebug() << e.what();
                throw;
            }

            std::stringstream outDataStream;
            outDataStream << model;
            std::string outStr = outDataStream.str();
            QByteArray outData(outStr.data(), outStr.length());
            QCOMPARE(outData, inData);
        }
        QVERIFY(testId > 1);
    }

    void testStressModelAndIO() {
        const int PASSES = 10;
        for (int pass = 0; pass < PASSES; pass++) {
            qDebug("pass %d/%d", pass + 1, PASSES);
            Model model;
            ModelModifier modifier(model, pass);
            for (int iteration = 0; iteration < 100; iteration++) {
                QCOMPARE(Figure::figuresAlive(), model.size());
                std::stringstream data;
                data << model;

                std::string saved1 = data.str();

                Model restored;
                data >> restored;

                std::stringstream data2;
                data2 << restored;
                QCOMPARE(Figure::figuresAlive(), 2 * model.size());

                Model restored2;
                data2 >> restored2;
                QCOMPARE(Figure::figuresAlive(), 3 * model.size());

                std::string saved2 = data.str();
                QCOMPARE(saved1, saved2);
                QVERIFY(modelsAreEqual(model, restored2));

                Model copyOfSource = model;
                QVERIFY(modelsAreEqual(model, copyOfSource));
                QCOMPARE(Figure::figuresAlive(), 4 * model.size());

                modifier.doRandom();
            }
        }
    }
};

QTEST_MAIN(Tests)
#include "tests.moc"
