#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QWidget>
#include "model.h"

namespace Ui {
class ModelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ModelWidget(QWidget *parent = 0);
    Model model;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Track lastTrack;

signals:

public slots:

};
}

#endif // MODELWIDGET_H
