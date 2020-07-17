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


	/*
	 * 由于使用的native窗口（d3d窗口），所以设置以下两个标志
	*/
	// 避免闪屏
    ui.imgLbl->setUpdatesEnabled(false);
	// 避免程序无响应（鼠标指针转圈）
	ui.imgLbl->setAttribute(Qt::WA_NativeWindow, true);

	QThread *thread = new QThread;
    QVariantMap params;
    params.insert("wid", reinterpret_cast<int>(ui.imgLbl));
	DshowControler *dc = new DshowControler(params);
	dc->moveToThread(thread);
	connect(thread, &QThread::started, dc, &DshowControler::process);
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

