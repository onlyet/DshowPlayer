#ifndef __COMMOM_H__
#define __COMMOM_H__


/*
 * ���Է����������ӳ�300����룬Ӳ�����ӳ�600�����
 * �������ã�
 * ������	Ӣ�ض� Core i5-8600K @ 3.60GHz ����
 * �ڴ�	8 GB ( ʮ�� DDR4 3000MHz )
 * �Կ�	Nvidia GeForce GTX 1650 SUPER ( 4 GB / Ӱ�� )
 */
#define Enable_Hardcode
 #define Enable_h264_qsv

 /*
  * libyuv��sws_scaleռ��CPU����30%����
  * ��ʱ��libyuv��1���룬sws_scale��2����
  * �������ã�
  * I5-9400 16G�ڴ�
  */
  //#define Enable_libyuv

// ����d3d��Ƶ��Ⱦ����Ҫsws_scale��setPixmap
#define Enable_D3dRender

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }
#endif

#define TRY_DELETE(__p) \
	if( (__p) ) \
	{ \
	try{ delete (__p); \
		}catch(...){ \
		TRACE("delete buffer error\n"); \
		} \
		(__p) = NULL; \
	}

void DeleteMediaType(AM_MEDIA_TYPE *mediaType);
void RemoveDownstream(IBaseFilter *pFilter, IGraphBuilder *pGraphBuilder);
HRESULT GetUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION pinDir, IPin **ppPin);

typedef enum {
	XU_CONTROL_FLIP = 0x01,
	XU_CONTROL_STRING_SUBTITLE = 0x02,
	XU_CONTROL_IDR = 0x03,
	XU_CONTROL_ENCODE_PROPERTY = 0x04,
	XU_CONTROL_SUBSTREAM_PROPERTY = 0x05,
	XU_CONTROL_UPDATE = 0x09,
	XU_CONTROL_DEFAULTENV = 0x0a,
	XU_CONTROL_TIME_SYNC = 0x05,
	XU_CONTROL_FOCUS_MODE = 0x06,
	XU_CONTROL_PRESET_CONTROL = 0x07,
	XU_CONTROL_PTZ_CONTROL = 0x08,
	XU_CONTROL_OSD_STRING = 0x0B,
	XU_CONTROL_OSD_DATE = 0x0C,
	XU_CONTROL_ENCODE_IDR = 0x0D,
	XU_CONTROL_ENCODE_PROFILE = 0x0E,
	XU_CONTROL_ENCODE_BITSRATE = 0x0F,
	XU_CONTROL_ENCODE_VIDEOSIZE = 0x10,
	XU_CONTROL_ENCODE_FRAMERATE = 0x11,
	XU_CONTROL_SUB_ENCODE_STREAMER = 0x12,
	XU_CONTROL_UART_CMD = 0x13,
	XU_CONTROL_ISP = 0x14,
	XU_CONTROL_ENCODE_CODEC = 0x15,
	XU_CONTROL_REBOOT = 0x18,
	XU_CONTROL_SLICESPLIT = 0x19,
	XU_CONTROL_IMAGESTYLE = 0x1A,
	XU_CONTROL_IFRAMEMINQP = 0x1B,
	XU_CONTROL_WORK_MODE = 0x1C,
	XU_CONTROL_IR_ENABLE = 0x1D,
	XU_CONTROL_CALIBRATION_LENS = 0x1E,
	//XU_CONTROL_IRIS_CALIBRATION_LENS = 0x1F,
	XU_CONTROL_ENCRYPTION = 0x1F,
	XU_CONTROL_NUMS
}XU_CONTROL;

typedef enum {
	MRCONFC_VSTREAM_MASTER = 0,		// ������
	MRCONFC_VSTREAM_SLAVE,			// ������
	MRCONFC_VSTREAM_MOBILE,			// �ֻ���
	MRCONFC_VSTREAM_NUMS,
}MRCONFC_VSTREAM_TYPE;

//#define MG101
#define UPDATE_BUFFER_LEN 60

#define PAN_TILT_DIR_UP		1
#define PAN_TILT_DIR_DOWN	-1
#define PAN_TILT_DIR_LEFT	-1
#define PAN_TILT_DIR_RIGHT	1
#define PAN_TILT_DIR_STOP	0

#define ZOOM_TELE			-1
#define ZOOM_WIDE			1
#define ZOOM_STOP			0

typedef enum {
	STOP = 0,
	UP,
	DOWN,
	LEFT,
	RIGHT,
	UP_LEFT,
	UP_RIGHT,
	DOWN_LEFT,
	DOWN_RIGHT,
	HOME_OK,
	ESC,
	MENU_SHOW,
	MENU_HIDE
}PTZ_CONTROL;

#endif