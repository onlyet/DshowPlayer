#include "DshowControler.h"
#include "CCapture.h"
#include "common.h"
#include <QDebug>
#include <QThread>
#include <QTime>
#include <QWidget>

extern "C"
{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavutil/imgutils.h"
}

//#define Enable_Hardcode

DshowControler::DshowControler(QVariantMap params)
	: QObject(nullptr)
    //, m_wid(params["wid"].toInt())
{
    m_wid = reinterpret_cast<QWidget*>(params["wid"].toInt());

	for (int i = 0; i < NUMELMS(m_rgpmVideo); i++)
	{
		m_rgpmVideo[i] = NULL;
	}

	m_dstWinWidth = 1920;
	m_dstWinHeight = 1080;


	QThread *cvThread = new QThread;
	m_pCaptureVideo = new CaptureVideo();
	m_pCaptureVideo->moveToThread(cvThread);
	//connect(cvThread, &QThread::started, m_pCaptureVideo, &CaptureVideo::process);
	connect(m_pCaptureVideo, &CaptureVideo::updateImage, this, &DshowControler::updateImage);
	connect(m_pCaptureVideo, &CaptureVideo::finished, m_pCaptureVideo, &CaptureVideo::deleteLater);
	connect(m_pCaptureVideo, &CaptureVideo::finished, cvThread, &QThread::quit);
	connect(cvThread, &QThread::finished, cvThread, &QThread::deleteLater);
	cvThread->start();
}

DshowControler::~DshowControler()
{
}

bool DshowControler::setParams()
{
	HRESULT hr;
	int nBitrate;
	WORD wIFrame;
	BYTE byRCMode, byMinQP, byMaxQP, byFramerate;

	if (NULL == m_pCaptureVideo)
	{
		return false;
	}

	hr = m_pCaptureVideo->RequestKeyFrame(MRCONFC_VSTREAM_MASTER);
	if (FAILED(hr))
	{
		qWarning() << "request key frame failed";
	}

	// 0是CBR
	byRCMode = 0;
	byMinQP = 10;
	byMaxQP = 40;
	nBitrate = 2048;
	hr = m_pCaptureVideo->SetBitrate(MRCONFC_VSTREAM_MASTER, byRCMode, byMinQP, byMaxQP, nBitrate);
	if (FAILED(hr))
	{
		qWarning() << "set bitrate failed";
	}

	byFramerate = 30;
	hr = m_pCaptureVideo->SetFrameRate(MRCONFC_VSTREAM_MASTER, byFramerate);
	if (FAILED(hr))
	{
		qWarning() << "set frame rate failed";
	}

	// I帧间距
	wIFrame = 90;
	hr = m_pCaptureVideo->SetIDR(MRCONFC_VSTREAM_MASTER, wIFrame);
	if (FAILED(hr))
	{
		qWarning() << "set iframe failed";
	}


	return true;
}

void DshowControler::ReleaseMoniker()
{
	for (int i = 0; i < NUMELMS(m_rgpmVideo); i++)
	{
		SAFE_RELEASE(m_rgpmVideo[i]);
	}
}

void DshowControler::EnumDevices()
{
	HRESULT hr;
	ULONG cFetched;
	// enumerate all video capture devices
	ICreateDevEnum *pCreateDevEnum = NULL;
	IEnumMoniker *pEm = NULL;
	IMoniker *pM = NULL;
	IMoniker *pMonikerVideo = NULL;
	IPropertyBag *pBag = NULL;
	int nIndex = 0;

	ReleaseMoniker();

	pMonikerVideo = m_pCaptureVideo->GetMoniker();

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if (hr != NOERROR)
	{
		qCritical() << "Error Creating Device Enumerator";
		goto exit;
	}

	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR)
	{
		qWarning() << "Sorry, you have no video capture hardware";
		goto exit;
	}

	pEm->Reset();

	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (hr == NOERROR)
			{
				QString name = QString::fromUtf16(reinterpret_cast<ushort*>(var.bstrVal));
				if (m_devNameList.contains(name))
				{
					int cnt = m_devNameList.count(name);
					m_devNameList.append(QString("%1#%2").arg(name).arg(QString::number(cnt)));
				}
				else
				{
					m_devNameList.append(name);
				}
				SysFreeString(var.bstrVal);

				if (NULL != pMonikerVideo && (S_OK == pMonikerVideo->IsEqual(pM)))
				{
					//m_cbDevs.SetCurSel(nIndex);
					//TODO: comboBox选中当前item
					m_nCurDev = nIndex;
				}

				//ASSERT(m_rgpmVideo[nIndex] == 0);
				assert(m_rgpmVideo[nIndex] == 0);
				m_rgpmVideo[nIndex] = pM;
				pM->AddRef();

				nIndex++;
			}
			SAFE_RELEASE(pBag);
		}
		SAFE_RELEASE(pM);
	}
	emit updateDevNameList(m_devNameList);

exit:
	SAFE_RELEASE(pEm);
	SAFE_RELEASE(pCreateDevEnum);
}

void DshowControler::OpenDevice()
{
	int nCurIndex;
	HRESULT hr = -1;

	//nCurIndex = m_cbDevs.GetCurSel();
	nCurIndex = 0;
	if (nCurIndex == m_nCurDev)
	{
		return;
	}

	//m_pCaptureVideo->initDecoder();

	m_nCurDev = nCurIndex;
	hr = m_pCaptureVideo->OpenDevice(m_rgpmVideo[m_nCurDev]);
	if (FAILED(hr))
	{
		qInfo() << "OpenDevice failed";
		return;
	}

	hr = m_pCaptureVideo->StartPreview();
	if (FAILED(hr))
	{
		qInfo() << "StartPreview failed";
	}
}

void DshowControler::startPlay()
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
	}

	EnumDevices();
	OpenDevice();
	// 为什么要filter graph运行后才能修改参数而不能提前设置？
	setParams();
}

void DshowControler::process()
{
    startPlay();

    QTime t;

    int w = 1920;
    int h = 1080;
	//int w = 720;
	//int h = 404;

    if (m_pCaptureVideo)
    {
#ifdef Enable_D3dRender
#ifdef Enable_Hardcode
        m_pCaptureVideo->initD3D_NV12((HWND)m_wid->winId(), w, h);
#else
        m_pCaptureVideo->initD3D_YUVJ420P((HWND)m_wid->winId(), w, h);
#endif // !Enable_Hardcode
#endif // Enable_D3dRender
    }

    while (!m_stopped)
    {
        if (!m_pCaptureVideo)
        {
            QThread::msleep(40);
            continue;
        }
        if (m_duration <= 0)
        {
            //m_duration = 1000 / m_pCaptureVideo->getFps();
            m_duration = 1000 / 30;
        }

        t.restart();
        if (!m_paused /*&& m_wid->isVisible()*/)
        {
            QImage img(w, h, QImage::Format_RGB32);
            if (m_pCaptureVideo->yuv2Rgb(img.bits(), w, h))
            {
                emit updateImage(QPixmap::fromImage(img));
            }
        }

        int deadline = m_duration - t.elapsed();
        if (deadline > 0)
        {
            QThread::msleep(deadline);
        }
    }
    qInfo() << "DshowControler finished";
}
