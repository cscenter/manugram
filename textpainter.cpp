#include "figurepainter.h"
#include "textpainter.h"
#include <QString>
#include <QPointF>
#include <QFontMetrics>
#include <QDebug>

class TextPositionVisitor : public FigureVisitor {
public:
    TextPosition textPosition() const { return result; }

    virtual void accept(figures::Segment &figure) override {
        const std::string &label = figure.label();

        Scaler scaler;
        QString text = QString::fromStdString(label);
        QPointF a = scaler(figure.getA());
        QPointF b = scaler(figure.getB());
        if (a.x() > b.x()) {
            std::swap(a, b);
        }
        QPointF direction = b - a;

        double angle = 0;
        if (direction.manhattanLength() > 10) {
            angle = atan2(direction.y(), direction.x()) * 180 / PI;
        }
        const double MAX_ANGLE = 30;
        const int BIG_SIZE = 1e6; // used in QFontMetrics call when there are no limits
        int length = (int)QVector2D(direction).length();

        QFont font;
        font.setPointSizeF(10);
        QFontMetrics metrics(font);
        QRect baseRect(QRect(QPoint(0, 0), QPoint(BIG_SIZE, BIG_SIZE)));
        QRect rect = metrics.boundingRect(baseRect, Qt::AlignTop | Qt::AlignLeft, text);

        result.width = rect.width();
        result.height = rect.height();
        if (fabs(angle) <= MAX_ANGLE) {
            Point offset((length - rect.width()) / 2, -rect.height());
            offset.rotateBy(angle * PI / 180);
            result.leftUp = scaler(a) + offset;
            result.rotation = angle;
        } else {
            result.leftUp = scaler((a + b) / 2 + QPointF(10, -rect.height() / 2));
        }
        return;
    }
    void accept(figures::BoundedFigure &figure) {
        const std::string &label = figure.label();

        Scaler scaler;
        const double REQUIRED_GAP = 0.8;
        const int BIG_SIZE = 1e6; // used in QFontMetrics call when there are no limits

        QString text = QString::fromStdString(label);
        BoundingBox box = figure.getBoundingBox();
        QPointF leftUp = scaler(box.leftUp);
        QPointF rightDown = scaler(box.rightDown);

        QFont font;
        font.setPointSizeF(10);
        QFontMetrics metrics(font);
        QPointF baseRectSize = rightDown - leftUp;
        QRect baseRect(QPoint(), QPoint((int)baseRectSize.x(), (int)baseRectSize.y()));
        QRectF rect = metrics.boundingRect(baseRect, Qt::AlignCenter, text);
        if (rect.width() <= REQUIRED_GAP * baseRect.width() && rect.height() <= REQUIRED_GAP * baseRect.height()) {
            rect.translate(leftUp);
        } else {
            baseRect.setSize(QSize(BIG_SIZE, BIG_SIZE));
            rect = metrics.boundingRect(baseRect, Qt::AlignHCenter, text);
            rect.translate(QPointF(baseRectSize.x() / 2 - BIG_SIZE / 2, 0));
            rect.translate(scaler(box.leftDown()));
        }
        result.leftUp = scaler(rect.topLeft());
        result.width = rect.width();
        result.height = rect.height();
    }
    virtual void accept(figures::Curve &fig) override {
        figures::Segment segm(fig.points.at(0), fig.points.at(1));
        segm.setLabel(fig.label());
        accept(segm);
    }

    virtual void accept(figures::SegmentConnection &segm) override {
        accept((figures::Segment&)segm);
    }
    virtual void accept(figures::Ellipse &fig) override {
        accept((figures::BoundedFigure&)fig);
    }
    virtual void accept(figures::Rectangle &fig) override {
        accept((figures::BoundedFigure&)fig);
    }
private:
    TextPosition result;
};

TextPosition getTextPosition(Figure &figure) {
    TextPositionVisitor visitor;
    figure.visit(visitor);
    return visitor.textPosition();
}
