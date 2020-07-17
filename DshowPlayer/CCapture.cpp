#include "CCapture.h"

#include "common.h"
#include <comdef.h> 

#include <Wmcodecdsp.h>

#include <QDebug>
#include <QTime>

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

#include <libyuv.h>

CCapture::CCapture(QObject *parent)
	: QObject(parent)
{
}

CCapture::~CCapture()
{
}

QString parseError(int errNum)
{
	char err[1024] = { 0 };
	av_strerror(errNum, err, 1024);
	return err;
}

/*
* 函数名称： Frame2JPG
* 功能描述： 将AVFrame(YUV420格式)保存为JPEG格式的图片
* 参    数： AVPacket packet av_read_frame读取的一包数据
* 参    数： AVFrame *pFrame 解码完的帧
* 参    数： stream_index 流下标，标记是视频流还是音频流
* 参    数： int width YUV420的宽
* 参    数： int height YUV420的高
* 返 回 值： int 0 代表成功，其他失败
*/
int Frame2JPG(/*AVPacket packet, */AVFrame *pFrame, unsigned int stream_index,
	int width, int height)
{
	// 输出文件路径  
	char out_file[MAX_PATH] = { 0 };
	//sprintf_s(out_file, sizeof(out_file), "%s%d.jpg", "f:\\", packet.pts);
	static int i = 0;
	sprintf_s(out_file, sizeof(out_file), "%s%d.jpg", "D:\\", i);
	++i;

	// 分配AVFormatContext对象  
	AVFormatContext *pFormatCtx = avformat_alloc_context();

	// 设置输出文件格式  
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
	// 创建并初始化一个和该url相关的AVIOContext  
	int ret;
	if ((ret = avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE)) < 0)
	{
		//gDebug().Write("Couldn't open output file.");
		return -1;
	}

	// 构建一个新stream  
	AVStream *pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL)
	{
		//gDebug().Write("Frame2JPG::avformat_new_stream error.");
		return -1;
	}

	// 设置该stream的信息  
	AVCodecContext *pCodecCtx = pAVStream->codec;

	pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	// Begin Output some information  
	// av_dump_format(pFormatCtx, 0, out_file, 1);
	// End Output some information  

	// 查找解码器  
	AVCodec *pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		//gDebug().Write("找不到图片编码器.");
		return -1;
	}
	// 设置pCodecCtx的解码器为pCodec  
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		//gDebug().Write("Could not open codec.");
		return -1;
	}

	//Write Header  
	ret = avformat_write_header(pFormatCtx, NULL);
	if (ret < 0)
	{
		//gDebug().Write("Frame2JPG::avformat_write_header.\n");
		return -1;
	}

	int y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode  
	// 给AVPacket分配足够大的空间  
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	int got_picture = 0;
	ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
	if (ret < 0)
	{
		//gDebug().Write("Encode Error.\n");
		return -1;
	}
	if (got_picture == 1)
	{
		//pkt.stream_index = pAVStream->index;  
		ret = av_write_frame(pFormatCtx, &pkt);
	}

	av_free_packet(&pkt);

	//Write Trailer  
	av_write_trailer(pFormatCtx);

	if (pAVStream)
	{
		avcodec_close(pAVStream->codec);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}

CaptureVideo::CaptureVideo(QObject *parent)
	: QObject(parent)
{
	//initialize member variable
	m_pCaptureGraphBuilder = NULL;
	m_pGraphBuilder = NULL;
	m_pMediaControl = NULL;
	m_pVCap = NULL;
	m_pKsCtrl = NULL;
	m_pME = NULL;
	m_pMoniker = NULL;
	m_pSampleGrabber = NULL;
	m_pGrabberFilter = NULL;

	m_pPinOutCapture = NULL;
	m_pPinInGrabber = NULL;
	//m_pPinOutGrabber = NULL;

	m_bRecord = false;
	m_bPreview = false;

#ifdef __ENABLE_RECORD__
	m_bFileOpen = FALSE;
	m_bFirstKeyFrame = FALSE;
#endif // __ENABLE_RECORD__

	//m_dstWinWidth = 1920;
	//m_dstWinHeight = 1080;

	initDecoder();

	m_yuvFrame = av_frame_alloc();
	//m_rgbFrame = av_frame_alloc();

	//m_swsCtx = sws_getContext(m_vDecodeCtx->width, m_vDecodeCtx->height,
	//	m_vDecodeCtx->pix_fmt, m_dst, m_vDecodeCtx->height,
	//	AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);


	//int m_vRgbFrameSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_dstWinWidth, m_dstWinHeight, 1);
	//m_rgbFrameBuf = (uint8_t *)av_malloc(m_vRgbFrameSize);
	//int ret = av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_rgbFrameBuf,
	//	AV_PIX_FMT_RGB32, m_dstWinWidth, m_dstWinHeight, 1);
}


CaptureVideo::~CaptureVideo(void)
{
	Close();
}

/************************************************************************/
/* 获取并初始化ICaptureGraphBuilder2等接口                                */
/************************************************************************/
HRESULT CaptureVideo::InitializeEnv()
{
	HRESULT hr;

	// Create the Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, (LPVOID*)&m_pGraphBuilder);
	if (FAILED(hr))
		return hr;

	// Create the Capture Graph Builder.
	// Make a graph builder object we can use for capture graph building
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
		IID_ICaptureGraphBuilder2, (LPVOID*)&m_pCaptureGraphBuilder);
	if (FAILED(hr))
		return hr;

	// Obtain interfaces for Media Control
	hr = m_pGraphBuilder->QueryInterface(IID_IMediaControl, (LPVOID*)&m_pMediaControl);
	if (FAILED(hr))
	{
		return hr;
	}

	// Initialize the Capture Graph Builder.
	hr = m_pCaptureGraphBuilder->SetFiltergraph(m_pGraphBuilder);

	return hr;
}

void CaptureVideo::Close()
{
	StopPreview();
	TearDownGraph();
	SAFE_RELEASE(m_pME);
	SAFE_RELEASE(m_pKsCtrl);
	SAFE_RELEASE(m_pMediaControl);
	SAFE_RELEASE(m_pVCap);
	SAFE_RELEASE(m_pMoniker);
	SAFE_RELEASE(m_pCaptureGraphBuilder);
	SAFE_RELEASE(m_pGraphBuilder);
	SAFE_RELEASE(m_pSampleGrabber);
	SAFE_RELEASE(m_pGrabberFilter);

	SAFE_RELEASE(m_pPinOutCapture);
	SAFE_RELEASE(m_pPinInGrabber);
	//SAFE_RELEASE(m_pPinOutGrabber);

	//TRACE("Capture video close.\n");
}

IMoniker* CaptureVideo::GetMoniker()
{
	return m_pMoniker;
}

HRESULT CaptureVideo::BindCaptureFilter()
{
	HRESULT hr = -1;
	IPropertyBag *pProBag = NULL;

	if (NULL == m_pMoniker)
	{
		return hr;
	}

	hr = m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (LPVOID*)&pProBag);
	if (FAILED(hr))
	{
		return hr;
	}

	// 该设备创建Capture filter
	hr = m_pMoniker->BindToObject(0, 0, IID_IBaseFilter, (LPVOID*)&m_pVCap);
	SAFE_RELEASE(pProBag);
	if (FAILED(hr))
	{
		return hr;
	}

	// 调用AddFilter把Capture filter添加到Filter Graph
	hr = m_pGraphBuilder->AddFilter(m_pVCap, L"Video Filter");
	return hr;
}

HRESULT CaptureVideo::OpenDevice(IMoniker *pMoniker)
{
	HRESULT hr = -1;

	if (NULL == pMoniker)
	{
		return hr;
	}

	Close();

	m_pMoniker = pMoniker;
	pMoniker->AddRef();

	// 1. 获取并初始化IcaptureGraphBuilder2接口
	hr = InitializeEnv();
	if (FAILED(hr))
	{
		goto exit;
	}

	// 2. 选择一个适当的视频捕捉设备，为该设备创建Capture filter
	hr = BindCaptureFilter();
	if (FAILED(hr))
	{
		goto exit;
	}
#if 1
	// 回调获取原始数据，如H264/MJPEG/YUV等
	hr = InitSampleGrabber();
	if (FAILED(hr))
	{
		goto exit;
	}

	// 4. 调用RenderStream依次把Still pin、Sample Grabber和系统默认Renderer Filter连接起来。
	hr = BuildPreviewGraph();
	if (FAILED(hr))
	{
		goto exit;
	}
#endif
	hr = m_pVCap->QueryInterface(IID_IKsControl, (void**)&m_pKsCtrl);

	return hr;
exit:
	Close();
	return hr;
}

HRESULT CaptureVideo::BuildPreviewGraph()
{
	HRESULT hr = -1;
	IAMStreamConfig *pStreamCfg = NULL;
	AM_MEDIA_TYPE *pMediaType = NULL;
	int nWidth = 0, nHeight = 0;
	GUID gdSubType = GUID_NULL;

	if (NULL == m_pCaptureGraphBuilder || NULL == m_pVCap)
	{
		return hr;
	}

	hr = m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE,
		&MEDIATYPE_Interleaved, m_pVCap, IID_IAMStreamConfig, (void**)&pStreamCfg);
	if (FAILED(hr))
	{
		hr = m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Video, m_pVCap, IID_IAMStreamConfig, (void**)&pStreamCfg);
	}

	if (SUCCEEDED(hr) && pStreamCfg)
	{
		hr = pStreamCfg->GetFormat(&pMediaType);
		if (SUCCEEDED(hr))
		{
			//HEADER(pMediaType->pbFormat)->biWidth = 1280;
			//HEADER(pMediaType->pbFormat)->biHeight = 720;
			HEADER(pMediaType->pbFormat)->biWidth = 1920;
			HEADER(pMediaType->pbFormat)->biHeight = 1080;
			nWidth = HEADER(pMediaType->pbFormat)->biWidth;
			nHeight = HEADER(pMediaType->pbFormat)->biHeight;
			// 设置支持的格式，比如H264/MJPEG/YUV等
			//gdSubType = pMediaType->subtype;		
			gdSubType = MEDIASUBTYPE_H264_EX;
			pMediaType->subtype = gdSubType;
			hr = pStreamCfg->SetFormat(pMediaType);
			if (FAILED(hr))
			{
				qCritical() << "set format failed";
			}
		}

	}


	if (GUID_NULL != gdSubType && (gdSubType == MEDIASUBTYPE_H264_EX || gdSubType == MEDIASUBTYPE_H265
		|| gdSubType == MEDIASUBTYPE_MJPG))
	{
		if (SUCCEEDED(hr))
		{
			hr = BuildPreviewGraphEx();
			if (SUCCEEDED(hr))
			{
			}
		}
	}
	else
	{
		if (SUCCEEDED(hr))
		{
			hr = m_pCaptureGraphBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
				m_pVCap, m_pGrabberFilter, NULL);
			if (FAILED(hr))
			{
				LPCTSTR errMsg;
				_com_error err(hr);
				errMsg = err.ErrorMessage();
				//TRACE(_T("RenderStream failed! Error Msg : %s. Error No : 0x%x\n"), errMsg, hr);
			}
		}
	}

	DeleteMediaType(pMediaType);
	SAFE_RELEASE(pStreamCfg);

	return hr;
}

HRESULT CaptureVideo::BuildPreviewGraphEx()
{
	HRESULT hr = -1;

	hr = GetUnconnectedPin(m_pVCap, PINDIR_OUTPUT, &m_pPinOutCapture);
	if (FAILED(hr))
	{
		//TRACE("get capture pin failed.\n");
		goto errorexit;
	}

	hr = GetUnconnectedPin(m_pGrabberFilter, PINDIR_INPUT, &m_pPinInGrabber);
	if (FAILED(hr))
	{
		//TRACE("get grabber pin in failed.\n");
		goto errorexit;
	}

	hr = m_pGraphBuilder->Connect(m_pPinOutCapture, m_pPinInGrabber);
	if (FAILED(hr))
	{
		//TRACE("connect pin out capture and pin in grabber failed.\n");
		goto errorexit;
	}

	//TRACE("BuildPreviewGraphEx end.\n");

	return hr;

errorexit:
	SAFE_RELEASE(m_pPinOutCapture);
	SAFE_RELEASE(m_pPinInGrabber);
	return hr;
}

HRESULT CaptureVideo::StartPreview()
{
	HRESULT hr = -1;

	if (m_pSampleGrabber)
	{
		hr = m_pSampleGrabber->SetCallback(this, 1);
		if (FAILED(hr))
		{
			//TRACE("Error : SampleGrabber set callback failed.\n");
			return hr;
		}
	}


	if (m_pMediaControl)
	{
		hr = m_pMediaControl->Run();
		if (SUCCEEDED(hr))
		{
			m_bPreview = true;
		}
		return hr;
	}

	//setParams();

	return -1;
}

HRESULT CaptureVideo::StopPreview()
{
	HRESULT hr = -1;

	if (m_pSampleGrabber)
	{
		hr = m_pSampleGrabber->SetCallback(NULL, 1);
		if (FAILED(hr))
		{
			//TRACE("Error : SampleGrabber SetCallback null failed.\n");
		}
	}

	// stop preview
	if (m_pMediaControl)
	{
		hr = m_pMediaControl->Stop();
		if (SUCCEEDED(hr))
		{
			m_bPreview = false;
		}
	}

	//InvalidateRect(m_hShowWnd, NULL, TRUE);
	return hr;
}

// Tear down everything downstream of the capture filters, so we can build
// a different capture graph.  Notice that we never destroy the capture filters
// and WDM filters upstream of them, because then all the capture settings
// we've set would be lost.
//
void CaptureVideo::TearDownGraph()
{
	if (m_pGrabberFilter)
	{
		//TRACE("diconnect grabber filter\n");
		//RemoveDownstream(m_pGrabberFilter, m_pGraphBuilder);
		//if (m_pPinOutGrabber)
		//{
		//	m_pGraphBuilder->Disconnect(m_pPinOutGrabber);
		//	SAFE_RELEASE(m_pPinOutGrabber);
		//}

		if (m_pPinInGrabber)
		{
			m_pGraphBuilder->Disconnect(m_pPinInGrabber);
			SAFE_RELEASE(m_pPinInGrabber);
		}
	}

	// destroy the graph downstream of our capture filters
	if (m_pVCap)
	{
		//TRACE("disconnect video capture filter\n");
		//RemoveDownstream(m_pVCap, m_pGraphBuilder);
		if (m_pPinOutCapture)
		{
			m_pGraphBuilder->Disconnect(m_pPinOutCapture);
			SAFE_RELEASE(m_pPinOutCapture);
		}
		else
		{
			//TRACE("video capture remove down stream\n");
			RemoveDownstream(m_pVCap, m_pGraphBuilder);
		}
	}

	//TRACE("Tear down graph!\n");
}

HRESULT CaptureVideo::ExtensionCtrl(int type, ULONG flag, byte* data, ULONG length)
{
	HRESULT hr = -1;
	KSP_NODE knod;
	ULONG dwRsz = 0;
	LPCTSTR errMsg;

	if (NULL == m_pVCap || NULL == m_pKsCtrl)
	{
		return hr;
	}

	knod.Property.Set = PROPSETID_VIDCAP_EXTENSION_UNIT;
	knod.Property.Id = type;
	knod.Property.Flags = (flag | KSPROPERTY_TYPE_TOPOLOGY);
	knod.NodeId = 2;
	knod.Reserved = 0;
	//TRACE("Extension control type : %d\n", type);
	//TRACE("Data size : %d\n", length);
	/*for (int i = 0; i < length; i++)
	{
		TRACE("Extension control Data[%d] : %d\n", i, data[i]);
	}*/

	hr = m_pKsCtrl->KsProperty((PKSPROPERTY)&knod, sizeof(knod), (PVOID)data, length, &dwRsz);
	if (FAILED(hr))
	{
		_com_error err(hr);
		errMsg = err.ErrorMessage();
		//TRACE(_T("KsProperty failed! Error msg : %s. hr = 0x%x\n"), errMsg, hr);
	}
	/*else
	{
		if (flag == KSPROPERTY_TYPE_GET)
		{
			for (int i = 0; i < length; i++)
			{
				TRACE("Extension control Get Data[%d] : %d\n", i, data[i]);
			}
		}
	}*/
	//TRACE("KsProperty success! dwRsz = %ld\n", dwRsz);

	return hr;
}

HRESULT CaptureVideo::InitSampleGrabber()
{
	HRESULT hr = -1;
	IAMStreamConfig *pConfig = NULL;
	AM_MEDIA_TYPE *fmt = NULL;
	//VIDEO_STREAM_CONFIG_CAPS scc;
	//GUID subType;

	// create grabber filter
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pGrabberFilter);
	if (FAILED(hr)) {
		//TRACE("create sample grabber filter failed.\n");
		return hr;
	}

	hr = m_pGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&(m_pSampleGrabber));
	if (FAILED(hr)) {
		//TRACE("query interface sample grabber failed.\n");
		return hr;
	}

	m_pGraphBuilder->AddFilter(m_pGrabberFilter, L"SampleGrabber");
	if (FAILED(hr)) {
		//TRACE("AddFilter sample grabber failed.\n");
		return hr;
	}
	/*hr = m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pVideoFilter, IID_IAMStreamConfig, (void**)&pConfig);
	if (FAILED(hr)) {
		TRACE("find interface(IID_IAMStreamConfig) failed.\n");
		return hr;
	}

	hr = pConfig->GetStreamCaps(2, &fmt, (byte*)&scc);
	if (FAILED(hr)) {
		TRACE("get stream caps failed.\n");
		return hr;
	}
	subType = fmt->subtype;
	pConfig->SetFormat(fmt);
	fmt->majortype = MEDIATYPE_Video;
	fmt->subtype = MEDIASUBTYPE_YUY2;
	hr = m_pSampleGrabber->SetMediaType(fmt);
	if (FAILED(hr)) {
		TRACE("sample grabber set media type failed.\n");
		return hr;
	}
	*/
	hr = m_pSampleGrabber->SetBufferSamples(FALSE);
	if (FAILED(hr)) {
		//TRACE("set buffer samples failed.\n");
		return hr;
	}

	/*hr = m_pSampleGrabber->SetCallback(this, 1);
	if (FAILED(hr)) {
		TRACE("set callback failed.\n");
	}*/


	//// 测试h264 decoder filter
	//IBaseFilter* m_pH264Filter = nullptr;
	//hr = CoCreateInstance(CLSID_CMPEG2VidDecoderDS, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pH264Filter);
	//if (FAILED(hr)) 
	//{
	//	TRACE("create h264 filter failed.\n");
	//	return hr;
	//}

	//ISampleGrabber* m_pH264SampleGrabber = nullptr;
	//hr = m_pH264Filter->QueryInterface(IID_ISampleGrabber, (void**)&(m_pH264SampleGrabber));
	//if (FAILED(hr)) {
	//	TRACE("query interface sample grabber failed.\n");
	//	return hr;
	//}

	return hr;
}

void CaptureVideo::process()
{
}


bool CaptureVideo::initDecoder()
{
    AVCodec* decoder = nullptr;
#ifdef Enable_Hardcode
#ifdef Enable_h264_qsv
    decoder = avcodec_find_decoder_by_name("h264_qsv");
#else Enable_h264_cuvid
    decoder = avcodec_find_decoder_by_name("h264_cuvid");
#endif // Enable_h264_cuvid
#else
    decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
#endif // Enable_Hardcode


	if (decoder == nullptr)
	{
		return false;
	}

	int ret;
	m_vDecodeCtx = avcodec_alloc_context3(decoder);
	//if ((ret = avcodec_parameters_to_context(m_vDecodeCtx, codecpar)) < 0)
	//{
	//	return false;
	//}

	//m_vDecodeCtx->width = 1920;
	//m_vDecodeCtx->height = 1080;
	//m_vDecodeCtx->flags |= AVFMT_FLAG_NOBUFFER;
	//m_vDecodeCtx->thread_type = 0;
	m_vDecodeCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
	m_vDecodeCtx->flags |= AV_CODEC_FLAG2_FAST;

#ifdef Enable_Hardcode
    m_vDecodeCtx->pix_fmt = *decoder->pix_fmts;
#else
	// TODO: 改成从dshow获取
    m_vDecodeCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
#endif // Enable_Hardcode

    // 硬解码：avcodec_open2后m_vDecodeCtx->pix_fmt从AV_PIX_FMT_CUDA变成了AV_PIX_FMT_NV12 为什么？
	int err = avcodec_open2(m_vDecodeCtx, decoder, nullptr);  // 打开解码器
	if (0 != err)   // 打开解码器错误
	{
		return false;
	}

#if IS_SET_FPS_WHEN_ERROR
	if (m_vDecodeCtx->framerate.den == 0
		|| m_vDecodeCtx->framerate.num == 0)
	{
		m_vDecodeCtx->framerate.num = FPS_CUSTOM;
		m_vDecodeCtx->framerate.den = 1;
	}
#endif // IS_SET_FPS_WHEN_ERROR



	return true;
}

bool CaptureVideo::yuv2Rgb(uchar *out, int dstWinWidth, int dstWinHeight)
{
    if (m_yuvFrame->linesize[0] == 0)
    {
        return 0;
    }

#ifdef Enable_D3dRender
#ifdef Enable_Hardcode
    BYTE *pNv12 = new BYTE[1920 * 1080 * 3 / 2];
    memset(pNv12, 0, 1920 * 1080 * 3 / 2);
    memcpy_s(pNv12, 1920 * 1080, m_yuvFrame->data[0], 1920 * 1080);
    memcpy_s(pNv12 + 1920 * 1080, 1920 * 1080 / 2, m_yuvFrame->data[1], 1920 * 1080 / 2);
    m_d3d.Render_NV12(pNv12, 1920, 1080);
    delete[] pNv12;
    return true;
#else
    BYTE *pYUV = new BYTE[1920 * 1080 * 3];
    memcpy(pYUV, m_yuvFrame->data[0], 1920 * 1080);
    memcpy(pYUV + 1920 * 1080, m_yuvFrame->data[1], 1920 * 1080 / 4);
    memcpy(pYUV + 1920 * 1080 * 5 / 4, m_yuvFrame->data[2], 1920 * 1080 / 4);
    m_d3d.Render_YUV(pYUV, 1920, 1080);
    delete[] pYUV;
    return true;
#endif // !Enable_Hardcode
#endif // Enable_D3dRender

    uint8_t *data[AV_NUM_DATA_POINTERS] = { nullptr };
    data[0] = out;     // 第一位输出RGB
    int lineSize[AV_NUM_DATA_POINTERS] = { 0 };
    lineSize[0] = dstWinWidth * 4;

#ifdef Enable_libyuv
	if (!m_yuvFrame->data[0])
	{
		return 0;
	}
	if (m_yuvFrame->linesize[0] == 0)
	{
		return 0;
	}
#ifdef Enable_Hardcode
#ifdef Enable_h264_qsv
	QTime t0 = QTime::currentTime();
	libyuv::NV12ToARGB(m_yuvFrame->data[0], 1920, m_yuvFrame->data[1], 1920, data[0], 1920 * 4, 1920, 1080);
	qDebug() << "libyuv::NV12ToARGB time: " << t0.elapsed();
#endif // !Enable_h264_qsv
#else

	QTime t = QTime::currentTime();
	int r2 = libyuv::I420ToARGB(m_yuvFrame->data[0], 1920, m_yuvFrame->data[1], 1920 / 2, m_yuvFrame->data[2], 1920 / 2, data[0], 1920 * 4, 1920, 1080);
	//int r2 = libyuv::I420ToARGB(m_yuvFrame->data[0], 1920, m_yuvFrame->data[1], 1920 / 2, m_yuvFrame->data[2], 1920 / 2, data[0], 720 * 4, 720, 404);
	qDebug() << "libyuv::I420ToARGB time: " << t.elapsed();
#endif // !Enable_Hardcode
	return 1;

#else
	m_swsCtx = sws_getContext(m_vDecodeCtx->width, m_vDecodeCtx->height, m_vDecodeCtx->pix_fmt,
		dstWinWidth, dstWinHeight, AV_PIX_FMT_RGB32,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!m_swsCtx)
	{
		qWarning() << "sws_getContext failed";
		return false;
	}
	QTime t2 = QTime::currentTime();
	int h = sws_scale(m_swsCtx,
		(uint8_t const * const *)m_yuvFrame->data, m_yuvFrame->linesize,
		0, m_vDecodeCtx->height,
		data, lineSize);
	qDebug() << "sws_scale time: " << t2.elapsed();

	sws_freeContext(m_swsCtx);

	return h > 0;
#endif // !Enable_libyuv

	//QImage img(m_rgbFrameBuf, m_vDecodeCtx->width, m_vDecodeCtx->height, QImage::Format_RGB32);
	//emit updateImage(QPixmap::fromImage(img));
}

bool CaptureVideo::initD3D_NV12(HWND hwnd, int img_width, int img_height)
{
    m_d3d.InitD3D_NV12(hwnd, 1920, 1080);
    return true;
}

bool CaptureVideo::initD3D_YUVJ420P(HWND hwnd, int img_width, int img_height)
{
    m_d3d.InitD3D_YUV(hwnd, 1920, 1080);
    return true;
}

bool CaptureVideo::setParams()
{
	HRESULT hr;
	int nBitrate;
	WORD wIFrame;
	BYTE byRCMode, byMinQP, byMaxQP, byFramerate;

	if (NULL == this)
	{
		return false;
	}
	//// 0是CBR
	//byRCMode = 0;
	//byMinQP = 10;
	//byMaxQP = 40;
	//nBitrate = 2048;
	//hr = SetBitrate(MRCONFC_VSTREAM_MASTER, byRCMode, byMinQP, byMaxQP, nBitrate);
	//if (FAILED(hr))
	//{
	//	qWarning() << "set bitrate failed";
	//}

	//byFramerate = 30;
	//hr = SetFrameRate(MRCONFC_VSTREAM_MASTER, byFramerate);
	//if (FAILED(hr))
	//{
	//	qWarning() << "set frame rate failed";
	//}

	//// I帧间距
	//wIFrame = 90;
	//hr = SetIDR(MRCONFC_VSTREAM_MASTER, wIFrame);
	//if (FAILED(hr))
	//{
	//	qWarning() << "set iframe failed";
	//}

	hr = RequestKeyFrame(MRCONFC_VSTREAM_MASTER);
	if (FAILED(hr))
	{
		qWarning() << "request key frame failed";
	}
	return true;
}

STDMETHODIMP CaptureVideo::SampleCB(double SampleTime, IMediaSample * pSample) {
	return 0;
}

/* 数据回调 */
STDMETHODIMP CaptureVideo::BufferCB(double dblSampleTime, BYTE * pBuffer, long lBufferSize) {
#ifdef __ENABLE_RECORD__
	//if (m_bFileOpen)
	//{
	//	m_fileRecorder.Write(pBuffer, lBufferSize);
	//}

#endif // __ENABLE_RECORD__

	qDebug() << QString("dblSampleTime: %1, lBufferSize: %2").arg(dblSampleTime).arg(lBufferSize);
	//return 0;

	static int s_dropFrame = 2;
	if (s_dropFrame > 0)
	{
		--s_dropFrame;
		return 0;
	}


	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = pBuffer;
	pkt.size = lBufferSize;
	pkt.stream_index = 0;
	
	QTime t = QTime::currentTime();
	int re = 1;
	//while (re)
	//{

	re = avcodec_send_packet(m_vDecodeCtx, &pkt);
	if (0 != re) // 如果发送失败
	{
		qDebug() << QString("avcodec_send_packet failed, %1, pkt.size: %2").arg(parseError(re)).arg(lBufferSize);
		return 0;
	}
	re = avcodec_receive_frame(m_vDecodeCtx, m_yuvFrame);
	if (0 != re) // 如果解码失败
	{
		qDebug() << QString("avcodec_receive_frame failed, %1, pkt.size: %2").arg(parseError(re)).arg(lBufferSize);
		//if (AVERROR(EAGAIN) != re)
		//{
		return 0;
		//}
	}
	//}

	static bool s_singleshot = false;
	if (!s_singleshot)
	{
		s_singleshot = true;
		//avcodec_flush_buffers(m_vDecodeCtx);
	}

	qDebug() << QStringLiteral("解码耗时：%1，frame->pkt_size: %2").arg(t.elapsed()).arg(m_yuvFrame->pkt_size);

    //av_hwframe_transfer_data
	return 0;
}

void CaptureVideo::StartRecord()
{
	//m_bRecord = true;
	//m_bFileOpen = m_fileRecorder.Open(_T("video.h264"), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
}

void CaptureVideo::StopRecord()
{
	m_bRecord = false;
#ifdef __ENABLE_RECORD__
	if (m_bFileOpen)
	{
		m_fileRecorder.Close();
		m_bFileOpen = FALSE;
		m_bFirstKeyFrame = FALSE;
	}
#endif // __ENABLE_RECORD__
}

/**
 * 打开流，主码流默认打开，子码流和三码流默认关闭
 * byStream : 0-主码流，1-子码流，2-三码流
**/
HRESULT CaptureVideo::OpenStream(BYTE byStream)
{
	byte byData[2];

	if (byStream > 2)
	{
		return -1;
	}

	byData[0] = byStream;
	byData[1] = 1;

	return ExtensionCtrl(XU_CONTROL_SUB_ENCODE_STREAMER, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}

/**
* 关闭流
* byStream : 0-主码流，1-子码流，2-三码流
**/
HRESULT CaptureVideo::CloseStream(BYTE byStream)
{
	byte byData[2];

	if (byStream > 2)
	{
		return -1;
	}

	byData[0] = byStream;
	byData[1] = 0;

	return ExtensionCtrl(XU_CONTROL_SUB_ENCODE_STREAMER, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}

/**
* 设置I帧间隔
* byStream : 0-主码流，1-子码流，2-三码流
* dwIDR : 0 - encode IDR immediately, 1-65535 : gop of IDR((byte2<<8) | byte1)
**/
HRESULT CaptureVideo::SetIDR(BYTE byStream, WORD dwIDR)
{
	byte byData[8] = { 0 };
	int nControl;

	if (byStream > 2 || dwIDR < 1 || dwIDR > 65535)
	{
		return -1;
	}

	byData[0] = byStream;	// 0-主码流，1-子码流，2-三码流
	byData[1] = dwIDR & 0xFF;			// 0 - encode IDR immediately, 1-65535:gop of IDR((byte2<<8) | byte1)
	byData[2] = (dwIDR >> 8) & 0xFF;
	nControl = XU_CONTROL_ENCODE_IDR;

	return ExtensionCtrl(nControl, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}

/**
 * 请求I帧
 * byStream : 0-主码流，1-子码流，2-三码流
**/
HRESULT CaptureVideo::RequestKeyFrame(BYTE byStream)
{
	byte byData[8] = { 0 };
	WORD dwGOP = 0;
	int nControl;

#ifdef MG101
	nControl = 1;
	byData[0] = 0x04;
	hr = ExtensionCtrl(nControl, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
#else
	if (byStream > 2)
	{
		return -1;
	}

	byData[0] = byStream;	// 0-主码流，1-子码流，2-三码流
	byData[1] = dwGOP & 0xFF;			// 0 - encode IDR immediately, 1-65535:gop of IDR((byte2<<8) | byte1)
	byData[2] = (dwGOP >> 8) & 0xFF;
	nControl = XU_CONTROL_ENCODE_IDR;
#endif

	return ExtensionCtrl(nControl, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}

/**
 * 设置码率kbps
 * byStream : 0-主码流，1-子码流，2-三码流
 * byRCMode :
 * byMinQP  : VBR: min qp/FixQP: qp
			  H264/H265 VBR min qp: 0 – 51
			  H264/H265 FixQP : I qp 0 – 51
			  MJPEG VBR min qp:1 – 99
			  MJPEG FixQP qp:1 – 99
			  VBR : min qp must < max qp
 * byMaxQP  : VBR: max qp
			  H264/H265 VBR max qp: 0 – 51
			  H264/H265 FixQP : P qp 0 – 51
			  MJPEG VBR max qp:1 – 99
  * nBitrate : H264/H265: 32 – 10 * 1024, MJPEG : 10 * 1024 – 100 * 102
**/
HRESULT CaptureVideo::SetBitrate(BYTE byStream, BYTE byRCMode, BYTE byMinQP, BYTE byMaxQP, UINT nBitrate)
{
	HRESULT hr;
	byte byData[8] = { 0 };
	int nControl;

	if (byStream > 2)
	{
		return -1;
	}
#ifdef MG101
	byData[0] = 0x08;
	byData[1] = 0xFF;
	byData[2] = nBitrate & 0xFF;
	byData[3] = (nBitrate >> 8) & 0xFF;
	nControl = 1;
#else
	byData[0] = byStream;					// 0-主码流，1-子码流，2-三码流;
	byData[1] = byRCMode;					// 0-CBR, 1-VBR, 2-FIXQP
	byData[2] = byMinQP;					// Min QP
	byData[3] = byMaxQP;					// Max QP
	byData[4] = nBitrate & 0xff;
	byData[5] = (nBitrate >> 8) & 0xff;
	byData[6] = (nBitrate >> 16) & 0xff;
	byData[7] = (nBitrate >> 24) & 0xff;
	nControl = XU_CONTROL_ENCODE_BITSRATE;
#endif

	hr = ExtensionCtrl(nControl, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
	return hr;
}

/**
 * 设置Profile
 * byStream : 0-主码流，1-子码流，2-三码流
 * byProfile : 0: Baseline
			   1: Main profile
			   2: High profile
 * byFrameRateMode : 0: 1X Frame Rate Mode(default)
					 1: 2X Frame Rate Mode(svc_t 2 level)
					 2: 4X Frame Rate Mode(svc_t 3 level)
**/
HRESULT CaptureVideo::SetProfile(BYTE byStream, BYTE byProfile, BYTE byFrameRateMode)
{
	BYTE byData[3];

	if (byStream > 2 || byProfile > 2 || byFrameRateMode > 2)
	{
		return -1;
	}

	byData[0] = byStream;
	byData[1] = byProfile;
	byData[2] = byFrameRateMode;

	return ExtensionCtrl(XU_CONTROL_ENCODE_PROFILE, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}

/**
 * 设置视频大小
 * byStream	: 0-主码流，1-子码流，2-三码流
 * wWidth : Main stream : 32 – 1920
			substream0 : 32 – 1280
			substream1 : 32 - 720
 * wHeight : Main stream : 32 – 1080
			 substream0 : 32 – 720
			 substream1 : 32 - 576
**/
HRESULT CaptureVideo::SetVideoSize(BYTE byStream, WORD wWidth, WORD wHeight)
{
	BYTE byData[5];

	if (byStream > 2)
	{
		return -1;
	}

	if (byStream == 0 && ((wWidth < 32 || wWidth > 1920) || (wHeight < 32 || wHeight > 1080)))
	{
		return -1;
	}

	if (byStream == 1 && ((wWidth < 32 || wWidth > 1280) || (wHeight < 32 || wHeight > 720)))
	{
		return -1;
	}

	if (byStream == 2 && ((wWidth < 32 || wWidth > 720) || (wHeight < 32 || wHeight > 576)))
	{
		return -1;
	}

	byData[0] = byStream;
	byData[1] = wWidth & 0xff;
	byData[2] = (wWidth >> 8) & 0xff;
	byData[3] = wHeight & 0xff;
	byData[4] = (wHeight >> 8) & 0xff;

	return ExtensionCtrl(XU_CONTROL_ENCODE_VIDEOSIZE, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}

/**
 * 设置帧率
 * byStream	: 0-主码流，1-子码流，2-三码流
 * byFrameRate : 1-30
**/
HRESULT CaptureVideo::SetFrameRate(BYTE byStream, BYTE byFrameRate)
{
	BYTE byData[2];

	if (byStream > 2 || byFrameRate < 1 || byFrameRate > 30)
	{
		return -1;
	}

	byData[0] = byStream;
	byData[1] = byFrameRate;

	return ExtensionCtrl(XU_CONTROL_ENCODE_FRAMERATE, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}

/**
* 设置编码格式
* byStream	: 0-主码流，1-子码流，2-三码流
* byEncode : 1 – MJPEG
			 5 – H264
			 7 – H265
**/
HRESULT CaptureVideo::SetEncodeFormate(BYTE bySteam, BYTE byEncode)
{
	BYTE byData[2];

	if (bySteam > 2 || byEncode != 1 || byEncode != 5 || byEncode != 7)
	{
		return -1;
	}

	byData[0] = bySteam;
	byData[1] = byEncode;

	return ExtensionCtrl(XU_CONTROL_ENCODE_CODEC, KSPROPERTY_TYPE_SET, byData, sizeof(byData));
}
