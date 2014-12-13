#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QWidget>
#include <list>
#include "model.h"

namespace Ui {
class ModelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ModelWidget(QWidget *parent = 0);
    void setModel(Model model);
    Model &getModel();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    Model commitedModel;

private:
    Track lastTrack;
    bool trackIsCancelled;
    std::list<Track> visibleTracks;

signals:

public slots:

};
}

#endif // MODELWIDGET_H
