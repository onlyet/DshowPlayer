#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DshowPlayer.h"

class DshowPlayer : public QMainWindow
{
    Q_OBJECT

public:
    DshowPlayer(QWidget *parent = Q_NULLPTR);

private:
    Ui::DshowPlayerClass ui;
};
