#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QWidget>
#include <QElapsedTimer>
#include <QMimeData>
#include <list>
#include "model.h"
#include "figurepainter.h"

namespace Ui {
class ModelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ModelWidget(QWidget *parent = 0);
    void setModel(Model model);
    void addModelExtraTrack(Track extraTrack);
    Model &getModel();

    bool canUndo();
    void undo();

    bool canRedo();
    void redo();

    int gridStep();
    void setGridStep(int newGridStep);

    double scaleFactor();
    void setScaleFactor(double newScaleFactor);

    bool showTrack();
    void setShowTrack(bool newShowTrack);

    bool showRecognitionResult();
    void setShowRecognitionResult(bool newShowRecognitionResult);

    bool storeTracks();
    void setStoreTracks(bool newStoreTracks);

    bool canGetSelectedMimeData();
    QMimeData *selectedMimeData();
    bool canPasteMimeData(const QMimeData *mimeData);
    void pasteMimeData(const QMimeData *mimeData);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
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
    std::vector<Track> extraTracks;
    QElapsedTimer trackTimer;

    MouseAction mouseAction;
    QPoint viewpointMoveStart;
    Scaler viewpointMoveOldScaler;
    std::list<Track> visibleTracks;
    int _gridStep;
    Scaler scaler;

    bool _showTrack;
    bool _showRecognitionResult;
    bool _storeTracks;

    void modifyModelAndCommit(std::function<void()> action);
    void customContextMenuRequested(const QPoint &pos);

signals:
    void canUndoChanged();
    void canRedoChanged();
    void scaleFactorChanged();
    void canGetSelectedMimeDataChanged();

public slots:
};
}

#endif // MODELWIDGET_H
