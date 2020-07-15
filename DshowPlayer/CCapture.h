#pragma once

#include <QObject>
#include <QPixmap>

#include <dshow.h>
#include <qedit.h>
#include <Ks.h>
#include <KsProxy.h>

class CCapture : public QObject
{
	Q_OBJECT

public:
	CCapture(QObject *parent);
	~CCapture();
};


#define STATIC_PROPSETID_VIDCAP_EXTENSION_UNIT \
	0xA29E7641, 0xDE04, 0x47e3, 0x8b, 0x2b, 0xf4, 0x34, 0x1a, 0xff, 0x00, 0x3b
DEFINE_GUIDSTRUCT("A29E7641-DE04-47E3-8B2B-F4341AFF003B", PROPSETID_VIDCAP_EXTENSION_UNIT);
#define PROPSETID_VIDCAP_EXTENSION_UNIT DEFINE_GUIDNAMED(PROPSETID_VIDCAP_EXTENSION_UNIT)

static const GUID MEDIASUBTYPE_H264_EX = { 0x34363248, 0x0000, 0x0010,{ 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID MEDIASUBTYPE_H265 = { 0x35363248, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

extern "C"
{
	struct AVCodecContext;
	struct AVFrame;
    struct SwsContext;
}

class CaptureVideo : public QObject, public ISampleGrabberCB
{
	Q_OBJECT

public:
	CaptureVideo(QObject *parent = nullptr);
	~CaptureVideo(void);

	STDMETHODIMP_(ULONG) AddRef() override { return 2; }
	STDMETHODIMP_(ULONG) Release() override { return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv) override {
		if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown) {
			*ppv = (void *) static_cast<ISampleGrabberCB*> (this);
			return NOERROR;
		}
		return E_NOINTERFACE;
	}
	STDMETHODIMP SampleCB(double SampleTime, IMediaSample * pSample) override;
	STDMETHODIMP BufferCB(double dblSampleTime, BYTE * pBuffer, long lBufferSize) override;


	HRESULT OpenDevice(IMoniker *pMoniker);
	void Close();					// close all interface

	HRESULT ExtensionCtrl(int type, ULONG flag, byte* data, ULONG length);
	IMoniker *GetMoniker();
	HRESULT StartPreview();
	HRESULT StopPreview();
	void StartRecord();
	void StopRecord();
	HRESULT RequestKeyFrame(BYTE byStream);
	HRESULT OpenStream(BYTE byStream);
	HRESULT CloseStream(BYTE byStream);
	HRESULT SetIDR(BYTE byStream, WORD wIDR);
	HRESULT SetBitrate(BYTE byStream, BYTE byRCMode, BYTE byMinQP, BYTE byMaxQP, UINT nBitrate);
	HRESULT SetProfile(BYTE byStream, BYTE byProfile, BYTE byFrameRateMode);
	HRESULT SetVideoSize(BYTE byStream, WORD wWidth, WORD wHeight);
	HRESULT SetFrameRate(BYTE byStream, BYTE byFrameRate);
	HRESULT SetEncodeFormate(BYTE bySteam, BYTE byEncode);

	bool initDecoder();

	bool yuv2Rgb(uchar *out, int outWidth, int outHeigh);

private:
	HRESULT InitializeEnv();				// initialize environment
	HRESULT BindCaptureFilter();
	void TearDownGraph();
	HRESULT BuildPreviewGraph();
	HRESULT BuildPreviewGraphEx();
	HRESULT InitSampleGrabber();

signals:
	void updateImage(QPixmap img);
	void finished();

public slots:
	void process();

private:
	IGraphBuilder *m_pGraphBuilder;	// filter graph manager
	ICaptureGraphBuilder2 *m_pCaptureGraphBuilder;
	IMediaControl *m_pMediaControl;	// 控制filter graph中的数据流
	IBaseFilter *m_pVCap;
	IKsControl *m_pKsCtrl;
	IMediaEventEx *m_pME;
	IMoniker *m_pMoniker;	// 设备别名
	IBaseFilter* m_pGrabberFilter;
	ISampleGrabber *m_pSampleGrabber;

	IPin *m_pPinOutCapture;
	IPin *m_pPinInGrabber;
	//IPin *m_pPinOutGrabber;

#ifdef __ENABLE_RECORD__
	CFile m_fileRecorder;
	BOOL m_bFileOpen;
	BOOL m_bFirstKeyFrame;
#endif // __ENABLE_RECORD__

	bool m_bRecord;


	AVCodecContext *m_vDecodeCtx = nullptr;
	AVFrame	*m_yuvFrame = nullptr;
	AVFrame *m_rgbFrame = nullptr;
	uint8_t * m_rgbFrameBuf = nullptr;
	SwsContext *m_swsCtx = nullptr;


	int m_dstWinWidth;	// 目标窗口宽度
	int m_dstWinHeight;	// 目标窗口高度

public:
	bool m_bPreview;
};