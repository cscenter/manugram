#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QWidget>
#include "model.h"

namespace Ui {
class ModelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModelWidget(QWidget *parent = 0);
    Model model;

protected:
    void paintEvent(QPaintEvent *event) override;

signals:

public slots:

};
}

#endif // MODELWIDGET_H
