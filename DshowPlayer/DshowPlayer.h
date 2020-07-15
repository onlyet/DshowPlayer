#pragma once

#include <QtWidgets/QMainWindow>
#include <QPixmap>
#include "ui_DshowPlayer.h"

class DshowPlayer : public QWidget
{
    Q_OBJECT

public:
    DshowPlayer(QWidget *parent = Q_NULLPTR);

signals:
	void startPlay();

public slots:
	void updateImage(QPixmap img);

private:
    Ui::DshowPlayer ui;
};
