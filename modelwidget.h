#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QWidget>
#include <list>
#include "model.h"
#include "figurepainter.h"

namespace Ui {
class ModelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ModelWidget(QWidget *parent = 0);
    void setModel(Model model);
    Model &getModel();

    bool canUndo();
    void undo();

    bool canRedo();
    void redo();

    int gridStep();
    void setGridStep(int newGridStep);

    double scaleFactor();
    void setScaleFactor(double newScaleFactor);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    Model commitedModel;
    std::list<Model> previousModels;
    std::list<Model> redoModels;

private:
    enum MouseAction {
        None,
        TrackActive,
        ViewpointMove
    };

    Track lastTrack;
    MouseAction mouseAction;
    QPoint viewpointMoveStart;
    Scaler viewpointMoveOldScaler;
    std::list<Track> visibleTracks;
    int _gridStep;
    Scaler scaler;

signals:
    void canUndoChanged();
    void canRedoChanged();
    void scaleFactorChanged();

public slots:
};
}

#endif // MODELWIDGET_H
