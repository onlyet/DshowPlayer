#pragma once

#include <QObject>
#include <QStringList>
#include <QPixmap>
#include <QVariant>

class QWidget;
class IMoniker;
class CaptureVideo;

extern "C"
{
	struct AVCodecContext;
	struct AVFrame;
	struct SwsContext;
}

class DshowControler : public QObject
{
	Q_OBJECT

public:
	DshowControler(QVariantMap params);
	~DshowControler();

	bool setParams();

private:
	void ReleaseMoniker();
	void EnumDevices();
	void OpenDevice();

signals:
	void updateDevNameList(QStringList);
	void finished();
	void updateImage(QPixmap img);

public slots:
	void startPlay();
	void process();

private:
	IMoniker *m_rgpmVideo[10];
	CaptureVideo *m_pCaptureVideo = nullptr;
	QStringList m_devNameList;

	int m_nCurDev = -1;	// 当前设备index

	//AVFrame *m_rgbFrame = nullptr;
	//uint8_t *m_rgbFrameBuf = nullptr;
	//SwsContext *m_swsCtx = nullptr;

	int m_srcWidth = 1920;
	int m_dstHeight = 1080;

	int m_dstWinWidth;	// 目标窗口宽度
	int m_dstWinHeight;	// 目标窗口高度

    bool m_paused = false;
    bool m_stopped = false;
    int m_duration = 0;

    QWidget *m_wid = nullptr;
};
