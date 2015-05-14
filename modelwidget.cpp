#include "model.h"
#include "model_io.h"
#include "modelwidget.h"
#include "figurepainter.h"
#include "recognition.h"
#include "layouting.h"
#include "model_ops.h"
#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QWheelEvent>
#include <QGesture>
#include <QDebug>
#include <QTimer>
#include <QMenu>

Ui::ModelWidget::ModelWidget(QWidget *parent) :
    QWidget(parent), mouseAction(MouseAction::None), _gridStep(0), _showTrack(true), _showRecognitionResult(true), _storeTracks(false) {
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    grabGesture(Qt::PinchGesture);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &Ui::ModelWidget::customContextMenuRequested);
}

void drawTrack(QPainter &painter, Scaler &scaler, const Track &track) {
    QPen oldPen = painter.pen();
    std::vector<double> speeds = calculateRelativeSpeeds(track);
    for (size_t i = 0; i + 1 < track.size(); i++) {
        double k = speeds[i] * 510;
        QColor color;
        if (k <= 255) color = QColor(255, k, 0, 127);
        else color = QColor(255 - (k - 255), 255, 0, 127);
        QPen pen = oldPen;
        pen.setColor(color);
        painter.setPen(pen);
        painter.drawLine(scaler(track[i]), scaler(track[i + 1]));
    }
    std::vector<int> stops = getSpeedBreakpoints(track);
    for (int stop : stops) {
        QPen pen = oldPen;
        pen.setColor(QColor(0, 255, 255));
        painter.setPen(pen);
        painter.drawEllipse(scaler(track[stop]), scaler.scaleFactor * 10, scaler.scaleFactor * 10);
    }
    painter.setPen(oldPen);
}

int Ui::ModelWidget::gridStep() {
    return _gridStep;
}

void Ui::ModelWidget::setGridStep(int newGridStep) {
    if (newGridStep < 0) {
        throw std::runtime_error("Grid step should be >= 0");
    }
    _gridStep = newGridStep;
    update();
}

double Ui::ModelWidget::scaleFactor() {
    return scaler.scaleFactor;
}

void Ui::ModelWidget::setScaleFactor(double newScaleFactor) {
    if (!(newScaleFactor >= 0.01)) { // >= instead of < for NaNs
        throw std::runtime_error("Scale factor should be >= 0.01");
    }
    scaler.scaleFactor = newScaleFactor;
    emit scaleFactorChanged();
    update();
}

bool Ui::ModelWidget::showTrack() {
    return _showTrack;
}
void Ui::ModelWidget::setShowTrack(bool newShowTrack) {
    _showTrack = newShowTrack;
}

bool Ui::ModelWidget::showRecognitionResult() {
    return _showRecognitionResult;
}
void Ui::ModelWidget::setShowRecognitionResult(bool newShowRecognitionResult) {
    _showRecognitionResult = newShowRecognitionResult;
}

bool Ui::ModelWidget::storeTracks() {
    return _storeTracks;
}
void Ui::ModelWidget::setStoreTracks(bool newStoreTracks) {
    _storeTracks = newStoreTracks;
}


void Ui::ModelWidget::setModel(Model model) {
    commitedModel = std::move(model);
    previousModels.clear();
    redoModels.clear();
    emit canUndoChanged();
    emit canRedoChanged();
    extraTracks = std::vector<Track>();
}
Model &Ui::ModelWidget::getModel() {
    return commitedModel;
}

void Ui::ModelWidget::addModelExtraTrack(Track track) {
    extraTracks.push_back(std::move(track));
}

bool Ui::ModelWidget::canUndo() {
    return !previousModels.empty();
}

void Ui::ModelWidget::undo() {
    if (!canUndo()) {
        throw std::runtime_error("Cannot undo");
    }
    redoModels.push_front(std::move(commitedModel));
    commitedModel = std::move(previousModels.back());
    previousModels.pop_back();
    if (!canUndo()) {
        emit canUndoChanged();
    }
    emit canRedoChanged();
    update();
}

bool Ui::ModelWidget::canRedo() {
    return !redoModels.empty();
}

void Ui::ModelWidget::redo() {
    if (!canRedo()) {
        throw std::runtime_error("Cannot redo");
    }
    previousModels.push_back(std::move(commitedModel));
    commitedModel = std::move(redoModels.front());
    redoModels.pop_front();
    if (!canRedo()) {
        canRedoChanged();
    }
    canUndoChanged();
    update();
}

void Ui::ModelWidget::modifyModelAndCommit(std::function<void()> action) {
    previousModels.push_back(commitedModel);
    redoModels.clear();
    action();
    emit canUndoChanged();
    emit canRedoChanged();
    update();
}


double roundDownToMultiple(double x, double multiple) {
    return floor(x / multiple) * multiple;
}

void Ui::ModelWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(), size()), Qt::white);

    QFont font;
    font.setPointSizeF(10 * scaler.scaleFactor);
    painter.setFont(font);

    QPen pen(Qt::black);
    pen.setWidthF(scaler.scaleFactor);
    painter.setPen(pen);

    FigurePainter fpainter(painter, scaler);
    if (gridStep() > 0) {
        int step = gridStep();
        // Calculating visible area
        Point p1 = scaler(QPointF(0, 0));
        Point p2 = scaler(QPointF(width(), height()));
        if (p1.x > p2.x) { std::swap(p1.x, p2.x); }
        if (p1.y > p2.y) { std::swap(p1.y, p2.y); }

        // Finding starting point for the grid
        p1.x = roundDownToMultiple(p1.x, step);
        p1.y = roundDownToMultiple(p1.y, step);

        // Drawing
        QPen pen(QColor(192, 192, 192, 255));
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        for (int x = p1.x; x <= p2.x; x += step) {
            painter.drawLine(scaler(Point(x, p1.y)), scaler(Point(x, p2.y)));
        }
        for (int y = p1.y; y <= p2.y; y += step) {
            painter.drawLine(scaler(Point(p1.x, y)), scaler(Point(p2.x, y)));
        }
    }

    Model copiedModel;
    Model &modelToDraw = (lastTrack.empty() || !showRecognitionResult()) ? commitedModel : (copiedModel = commitedModel);
    PFigure modified = showRecognitionResult() ? recognize(lastTrack, modelToDraw) : nullptr;
    for (PFigure fig : modelToDraw) {
        if (fig == modified) {
            pen.setColor(Qt::magenta);
        } else if (fig == modelToDraw.selectedFigure) {
            pen.setColor(Qt::blue);
        } else {
            pen.setColor(Qt::black);
        }
        painter.setPen(pen);
        fig->visit(fpainter);
    }

    pen.setColor(QColor(255, 0, 0, 16));
    pen.setWidth(3 * scaler.scaleFactor);
    painter.setPen(pen);
    if (showTrack()) {
        drawTrack(painter, scaler, lastTrack);
        for (const Track &track : visibleTracks) {
            drawTrack(painter, scaler, track);
        }
    }
    for (const Track &track : extraTracks) {
        drawTrack(painter, scaler, track);
    }
}

void Ui::ModelWidget::customContextMenuRequested(const QPoint &pos) {
    PFigure figure = findClickedFigure(commitedModel, scaler(pos));
    if (figure) {
        std::shared_ptr<figures::Curve> curve = std::dynamic_pointer_cast<figures::Curve>(figure);
        if (curve) {
            QMenu contextMenu;
            QAction verticalSymmetry("Make vertically symmetric", this);
            connect(&verticalSymmetry, &QAction::triggered, [this, curve]() {
                modifyModelAndCommit([curve]() {
                    makeVerticallySymmetric(curve);
                });
            });
            contextMenu.addAction(&verticalSymmetry);

            QAction horizontalSymmetry("Make horizontally symmetric", this);
            connect(&horizontalSymmetry, &QAction::triggered, [this, curve]() {
                modifyModelAndCommit([curve]() {
                    makeHorizontallySymmetric(curve);
                });
            });
            contextMenu.addAction(&horizontalSymmetry);

            contextMenu.exec(mapToGlobal(pos));
        }

        auto bounded = std::dynamic_pointer_cast<figures::BoundedFigure>(figure);
        if (bounded) {
            QMenu contextMenu;
            QAction topBottomTree("Arrange children in a tree", this);
            connect(&topBottomTree, &QAction::triggered, [this, bounded]() {
                modifyModelAndCommit([this, bounded]() {
                    makeTopBottomTree(commitedModel, bounded);
                });
            });
            contextMenu.addAction(&topBottomTree);

            contextMenu.exec(mapToGlobal(pos));
        }
    }
}

void Ui::ModelWidget::mousePressEvent(QMouseEvent *event) {
    lastTrack = Track();

    if (event->modifiers().testFlag(Qt::ShiftModifier) || event->buttons().testFlag(Qt::MiddleButton)) {
        mouseAction = MouseAction::ViewpointMove;
        viewpointMoveStart = event->pos();
        viewpointMoveOldScaler = scaler;
        setCursor(Qt::ClosedHandCursor);
    } else if (event->buttons().testFlag(Qt::LeftButton)) {
        mouseAction = MouseAction::TrackActive;
        trackTimer.start();
        lastTrack.points.push_back(TrackPoint(scaler(event->pos()), trackTimer.elapsed()));
    }
    update();
}
void Ui::ModelWidget::mouseMoveEvent(QMouseEvent *event) {
    if (mouseAction == MouseAction::ViewpointMove) {
        scaler = viewpointMoveOldScaler;
        scaler.zeroPoint = scaler.zeroPoint + scaler(viewpointMoveStart) - scaler(event->pos());
        update();
    } else if (mouseAction == MouseAction::TrackActive) {
        lastTrack.points.push_back(TrackPoint(scaler(event->pos()), trackTimer.elapsed()));
        if (showTrack() || showRecognitionResult()) {
            update();
        }
    } else {
        event->ignore();
    }
}
void Ui::ModelWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (mouseAction == MouseAction::None) {
        event->ignore();
        return;
    }
    if (mouseAction == MouseAction::ViewpointMove) {
        mouseAction = MouseAction::None;
        setCursor(Qt::ArrowCursor);
        scaler = viewpointMoveOldScaler;
        scaler.zeroPoint = scaler.zeroPoint + scaler(viewpointMoveStart) - scaler(event->pos());
        update();
        return;
    }
    assert(mouseAction == MouseAction::TrackActive);
    mouseAction = MouseAction::None;
    lastTrack.points.push_back(TrackPoint(scaler(event->pos()), trackTimer.elapsed()));
    if (storeTracks()) {
        QFile file(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".track");
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            QMessageBox::critical(this, "Error while saving track", "Unable to open file for writing");
        } else {
            std::stringstream stream;
            stream << lastTrack;
            std::string data = stream.str();
            if (file.write(data.data(), data.length()) != data.length()) {
                QMessageBox::critical(this, "Error while saving track", "Unable to write to opened file");
            }
        }
    }
    Model previousModel = commitedModel;
    PFigure modifiedFigure = recognize(lastTrack, commitedModel);
    if (_gridStep > 0 && modifiedFigure) {
        GridAlignLayouter layouter(_gridStep);
        layouter.updateLayout(commitedModel, modifiedFigure);
        commitedModel.recalculate();
    }

    visibleTracks.push_back(lastTrack);
    auto iterator = --visibleTracks.end();
    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, [this, iterator, timer]() {
        visibleTracks.erase(iterator);
        delete timer;
        update();
    });
    timer->setInterval(1500);
    timer->setSingleShot(true);
    timer->start();

    lastTrack = Track();
    if (modifiedFigure) {
        previousModels.push_back(previousModel);
        redoModels.clear();
        emit canUndoChanged();
        emit canRedoChanged();
    }
    update();
}
void Ui::ModelWidget::keyReleaseEvent(QKeyEvent *event) {
    event->ignore();
    if (event->key() == Qt::Key_Escape && mouseAction == MouseAction::TrackActive) {
        event->accept();
        lastTrack = Track();
        mouseAction = MouseAction::None;
        update();
    }
    if (event->key() == Qt::Key_Delete) {
        if (commitedModel.selectedFigure) {
            event->accept();
            modifyModelAndCommit([this]() {
                for (auto it = commitedModel.begin(); it != commitedModel.end(); it++) {
                    if (*it == commitedModel.selectedFigure) {
                        commitedModel.removeFigure(it);
                        break;
                    }
                }
            });
        }
    }
}

void Ui::ModelWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    event->ignore();
    auto &figure = commitedModel.selectedFigure;
    if (!figure) { return; }

    Point eventPos = scaler(event->pos());
    bool hit = false;
    hit |= figure->isInsideOrOnBorder(eventPos);
    hit |= figure->getApproximateDistanceToBorder(eventPos) <= figureSelectGap();
    if (!hit) { return; }

    event->accept();
    bool ok;
    QString newLabel = QInputDialog::getMultiLineText(this, "Figure label", "Specify new figure label",
                                             QString::fromStdString(figure->label()),
                                                      &ok);
    if (ok) {
        modifyModelAndCommit([figure, &newLabel]() {
            figure->setLabel(newLabel.toStdString());
        });
    }
}

void Ui::ModelWidget::wheelEvent(QWheelEvent *event) {
    event->ignore();
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        event->accept();
        double scrolled = event->angleDelta().y() / 8;
        double factor = 1.0 + 0.2 * (scrolled / 15); // 20% per each 15 degrees (standard step)
        factor = std::max(factor, 0.1);
        scaler.scaleWithFixedPoint(scaler(event->pos()), factor);
        update();
    }
}

bool Ui::ModelWidget::event(QEvent *event) {
    if (event->type() == QEvent::Gesture) {
        QGestureEvent *gevent = static_cast<QGestureEvent*>(event);
        QPinchGesture *gesture = static_cast<QPinchGesture*>(gevent->gesture(Qt::PinchGesture));
        if (gesture) {
            gevent->accept(gesture);

            lastTrack = Track();
            mouseAction = MouseAction::None;

            scaler.zeroPoint = scaler.zeroPoint + scaler(gesture->lastCenterPoint()) - scaler(gesture->centerPoint());
            scaler.scaleWithFixedPoint(scaler(gesture->centerPoint()), gesture->scaleFactor());
            emit scaleFactorChanged();
            update();

            return true;
        }
    }
    return QWidget::event(event);
}
