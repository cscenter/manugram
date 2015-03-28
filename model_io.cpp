#include "model_io.h"
#include "figurepainter.h"
#include "textpainter.h"
#include <QImage>

std::istream &operator>>(std::istream &in, Model &model) {
    int count;
    if (!(in >> count)) {
        throw model_format_error("unable to read number of figures");
    }
    std::vector<PFigure> figures;
    size_t selected_id = 0;
    while (count -- > 0) {
        std::string type;
        if (!(in >> type)) {
            throw model_format_error("unable to read fngure type");
        }
        if (type == "segment" || type == "segment_connection") {
            std::shared_ptr<figures::Segment> segm;
            if (type == "segment_connection") {
                size_t aId, bId;
                if (!(in >> aId >> bId)) {
                    throw model_format_error("unable to read connection information");
                }
                aId--, bId--;
                if (aId >= figures.size() || bId >= figures.size()) {
                    throw model_format_error("invalid figures in connection");
                }
                auto figA = std::dynamic_pointer_cast<figures::BoundedFigure>(figures.at(aId));
                auto figB = std::dynamic_pointer_cast<figures::BoundedFigure>(figures.at(bId));
                if (!figA || !figB) {
                    throw model_format_error("invalid reference in connection");
                }
                segm = std::make_shared<figures::SegmentConnection>(figA, figB);
            } else {
                double x1, y1, x2, y2;
                if (!(in >> x1 >> y1 >> x2 >> y2)) {
                    throw model_format_error("unable to read segment");
                }
                segm = std::make_shared<figures::Segment>(Point(x1, y1), Point(x2, y2));
            }
            bool arrowA, arrowB;
            if (!(in >> arrowA >> arrowB)) {
                throw model_format_error("unable to read segment");
            }
            segm->setArrowedA(arrowA);
            segm->setArrowedB(arrowB);
            figures.push_back(segm);
        } else if (type == "rectangle") {
            double x1, y1, x2, y2;
            if (!(in >> x1 >> y1 >> x2 >> y2)) {
                throw model_format_error("unable to read rectangle");
            }
            figures.push_back(std::make_shared<figures::Rectangle>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
        } else if (type == "ellipse") {
            double x1, y1, x2, y2;
            if (!(in >> x1 >> y1 >> x2 >> y2)) {
                throw model_format_error("unable to read ellipse");
            }
            figures.push_back(std::make_shared<figures::Ellipse>(BoundingBox({Point(x1, y1), Point(x2, y2)})));
        } else if (type == "label=") {
            if (figures.empty()) {
                throw model_format_error("Misplaced 'label='");
            }
            int len;
            if (!(in >> len) || (len <= 0)) {
                throw model_format_error("Unable to read label length");
            }
            char separator;
            if (!in.read(&separator, 1) || separator != ' ') {
                throw model_format_error("Unable to read label separator");
            }
            std::vector<char> label(len);
            if (!in.read(&label[0], len)) {
                throw model_format_error("Unable to read label data");
            }
            figures.back()->setLabel(std::string(label.begin(), label.end()));
        } else if (type == "selected=") {
            if (selected_id != 0) {
                throw model_format_error("two figures are selected");
            }
            if (!(in >> selected_id)) {
                throw model_format_error("unable to read selected figure id");
            }
        } else {
            throw model_format_error("unknown type: '" + type + "'");
        }
    }
    for (PFigure figure : figures) {
        model.addFigure(figure);
    }
    if (selected_id != 0) {
        model.selectedFigure = figures.at(selected_id - 1);
    }
    return in;
}

class FigurePrinter : public FigureVisitor {
public:
    FigurePrinter(std::ostream &out, const std::map<PFigure, size_t> &ids) : out(out), ids(ids) {}

    virtual void accept(figures::Segment &segm) {
        out << "segment ";
        printPoint(segm.getA());
        out << " ";
        printPoint(segm.getB());
        out << " " << segm.getArrowedA() << " " << segm.getArrowedB() << "\n";
        printLabel(segm);
    }
    virtual void accept(figures::SegmentConnection &segm) {
        out << "segment_connection ";
        out << ids.at(segm.getFigureA()) << " ";
        out << ids.at(segm.getFigureB()) << " ";
        out << " " << segm.getArrowedA() << " " << segm.getArrowedB() << "\n";
        printLabel(segm);
    }

    virtual void accept(figures::Ellipse &fig) {
        out << "ellipse ";
        printBoundingBox(fig.getBoundingBox());
        out << "\n";
        printLabel(fig);
    }

    virtual void accept(figures::Rectangle &fig) {
        out << "rectangle ";
        printBoundingBox(fig.getBoundingBox());
        out << "\n";
        printLabel(fig);
    }

private:
    std::ostream &out;
    const std::map<PFigure, size_t> &ids;
    void printPoint(const Point &p) {
        out << p.x << " " << p.y;
    }
    void printBoundingBox(const BoundingBox &box) {
        printPoint(box.leftUp);
        out << " ";
        printPoint(box.rightDown);
    }
    void printLabel(const Figure &figure) {
        if (figure.label().empty()) { return; }
        out << "  label= " << figure.label().size() << " " << figure.label() << "\n";
    }
};

std::ostream &operator<<(std::ostream &out, Model &model) {
    size_t operations = 0;
    operations += model.size();
    operations += !!model.selectedFigure;
    for (PFigure figure : model) {
        operations += !figure->label().empty();
    }
    out << operations << '\n';

    std::map<PFigure, size_t> ids;
    for (PFigure figure : model) {
        size_t id = ids.size() + 1;
        ids[figure] = id;
    }

    FigurePrinter printer(out, ids);
    for (PFigure figure : model) {
        figure->visit(printer);
    }

    if (model.selectedFigure) {
        out << "selected= " << ids.at(model.selectedFigure) << "\n";
    }
    return out;
}

void exportModelToSvg(Model &m, std::ostream &out) {
    FigureSvgPainter painter(out);
    painter.printHeader();
    for (PFigure figure : m) {
        figure->visit(painter);
    }
    painter.printFooter();
}

void exportModelToImageFile(Model &model, const QString &filename) {
    BoundingBox imageBox;
    for (const PFigure &fig : model) {
        BoundingBox box = fig->getBoundingBox();
        imageBox.addPoint(box.leftUp);
        imageBox.addPoint(box.rightDown);
        if (!fig->label().empty() && std::dynamic_pointer_cast<figures::BoundedFigure>(fig)) {
            TextPosition text = getTextPosition(*fig);
            Point corners[] = { text.leftUp, text.leftUp + Point(text.width, text.height) };
            for (Point corner : corners) {
                imageBox.addPoint(corner);
            }
        }
    }
    if (imageBox.leftUp.x > imageBox.rightDown.x) {
        imageBox = { Point(0, 0) };
    }
    {
        double size = std::max(imageBox.width(), imageBox.height());
        double gap = size * 0.05;
        imageBox.leftUp.x -= gap;
        imageBox.leftUp.y -= gap;
        imageBox.rightDown.x += gap;
        imageBox.rightDown.y += gap;
    }

    QImage img(imageBox.width(), imageBox.height(), QImage::Format_ARGB32);
    QPainter painter(&img);
    QFont font;
    font.setPointSizeF(10);
    painter.setFont(font);
    painter.fillRect(QRect(QPoint(), img.size()), Qt::white);
    painter.setPen(Qt::black);
    FigurePainter fpainter(painter, imageBox.leftUp);
    for (PFigure fig : model) {
        fig->visit(fpainter);
    }
    painter.end();
    if (!img.save(filename)) {
        throw std::runtime_error("Unable to save to PNG file");
    }
}
