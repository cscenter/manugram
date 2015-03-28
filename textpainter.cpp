#include "figurepainter.h"
#include "textpainter.h"
#include <QString>
#include <QPointF>
#include <QFontMetrics>

class TextPositionVisitor : public FigureVisitor {
public:
    TextPosition textPosition() const { return result; }

    virtual void accept(figures::Segment &segm) override {
        assert(false);
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
