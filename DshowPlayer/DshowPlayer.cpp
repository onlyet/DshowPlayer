#include "DshowPlayer.h"
#include "DshowControler.h"
#include <QThread>
#include <QDebug>
//#include <Q>

DshowPlayer::DshowPlayer(QWidget *parent)
    : QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	ui.setupUi(this);

	QThread *thread = new QThread;
	DshowControler *dc = new DshowControler();
	dc->moveToThread(thread);
	connect(thread, &QThread::started, dc, &DshowControler::startPlay);
	connect(dc, &DshowControler::finished, thread, &QThread::quit);
	connect(dc, &DshowControler::finished, dc, &DshowControler::deleteLater);
	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(dc, &DshowControler::updateImage, this, &DshowPlayer::updateImage);

	thread->start();
	//emit startPlay();
}

void DshowPlayer::updateImage(QPixmap img)
{
	//qDebug() << "DshowPlayer size: " << this->size();
	//qDebug() << "frame size: " << ui.frame->size();
	//qDebug() << "imgLbl size: " << ui.imgLbl->size();
	ui.imgLbl->setPixmap(img);
}

