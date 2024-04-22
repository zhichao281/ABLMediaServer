
/*
 *
 * 本库使用到的数据类型定义声明
 */
#pragma once

#ifdef _WIN32
#ifdef   LIBMPEG_EXPORTS
#define CE_API __declspec(dllexport)
#define CE_APICALL  __stdcall
#else
#define CE_API __declspec(dllimport)
#define CE_APICALL  __stdcall
#endif 

#else
#define CE_API
#define CE_APICALL __attribute__((visibility("default")))
#endif



#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif



#ifdef __ANDROID__       //android的编译器会自动识别到这个为真。

#include <android/log.h>

#undef printf  
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "Car-eye-ffmpeg", __VA_ARGS__)


#define __STDC_CONSTANT_MACROS
#define CarEyeLog(...) __android_log_print(ANDROID_LOG_DEBUG, "Car-eye-ffmpeg", __VA_ARGS__)

#endif

 //error number
#define NO_ERROR1			0
#define PARAMTER_ERROR		1
#define NULL_MEMORY			2
#define MAX_FILTER_DESCR	512

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>
#include <map>

#define DebugPrintf(fmt,args,...)  printf("%s(%d)-%s -> " #fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args);
typedef std::function<void(uint8_t* raw_data, const char* codecid, int raw_len, bool bKey, int nWidth, int nHeight, int64_t nTimeStamp)> VideoCllBack;
typedef std::function<void(uint8_t* raw_data, const char* codecid, int raw_len, int channels, int sample_rate, int bytes, int64_t nTimeStamp)>AudioCallBack;
typedef std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)> PcmCallBack;
typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)> YuvCallBack;


// 编码器对象句柄定义
#define CarEye_Encoder_Handle void*
 // 解码器对象句柄定义
#define CarEye_Decoder_Handle void*
 // 水印编码器对象句柄定义
#define CarEye_OSD_Handle void*

 // 最大音频帧大小 1 second of 48khz 32bit audio
#define MAX_AUDIO_FRAME_SIZE 192000

 // 媒体编码类型定义 与FFMPEG中一一对应，H265定义与其他库定义需要转换
typedef enum
{
	// 不进行编码
	CAREYE_CODEC_NONE = 0,
	// H264编码
	CAREYE_CODEC_H264 = 27,
	// H265编码
	CAREYE_CODEC_H265 = 173,
	// MJPEG编码
	CAREYE_CODEC_MJPEG = 7,
	// MPEG4编码
	CAREYE_CODEC_MPEG4 = 12,

	// G711 Ulaw编码 对应FFMPEG中的AV_CODEC_ID_PCM_MULAW定义
	CAREYE_CODEC_G711U = 0x10006,
	// G711 Alaw编码 对应FFMPEG中的AV_CODEC_ID_PCM_ALAW定义
	CAREYE_CODEC_G711A = 0x10007,
	// G726编码 对应FFMPEG中的AV_CODEC_ID_ADPCM_G726定义
	CAREYE_CODEC_G726 = 0x1100B,
	CAREYE_CODEC_G726LE = 0x11804,

	// AAC编码
	CAREYE_CODEC_AAC = 0x15002,
	// OPUS编码 对应FFMPEG中的AV_CODEC_ID_OPUS定义
	CAREYE_CODEC_OPUS = 0x1503C


}FFmpeg_CodecType;

enum DecodeType {
	kDecodeSoft = 0, //软解
	kNvencDecode = 1, //英伟达编码 
	kQsvDecode =2, //AMD编码
	kJestonDecode = 3 //英伟达编码 
};


// YUV视频流格式定义，与FFMPEG中一一对应
typedef enum
{
	CAREYE_FMT_YUV420P = 0,///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
	CAREYE__FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
	CAREYE__FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
	CAREYE_FMT_RGB24=2,
	CAREYE_FMT_YUV422P = 4,
	CAREYE_FMT_YUV444P = 5,
	CAREYE_FMT_YUV410P = 6,
	CAREYE_FMT_YUV411P = 7,
	CAREYE_FMT_NV12 = 23
}FFMPEG_AVFormat;

// YUV媒体流结构定义
typedef struct
{
	// Y分量数据存储区
	unsigned char* Y;
	// Y分量数据字节数
	int YSize;
	// U分量数据存储区
	unsigned char* U;
	// U分量数据字节数
	int USize;
	// V分量数据存储区
	unsigned char* V;
	// V分量数据字节数
	int VSize;
}CarEye_YUVFrame;


// Supported rotation.
typedef enum  {
	kRotate0 = 0,      // No rotation.
	kRotate90 = 90,    // Rotate 90 degrees clockwise.
	kRotate180 = 180,  // Rotate 180 degrees.
	kRotate270 = 270,  // Rotate 270 degrees clockwise.

	// Deprecated.
	kRotateNone = 0,
	kRotateClockwise = 90,
	kRotateCounterClockwise = 270,
} RotationEnum;


#define MAX_STRING_LENGTH 1024
#define MAX_FILE_NAME 64
// 水印结果定义
typedef struct
{
	// 视频宽度
	int Width;
	// 视频高度
	int Height;
	// 视频帧率（FPS）
	int FramesPerSecond;
	// 添加水印的视频格式
	FFMPEG_AVFormat YUVType;
	// 水印起始X轴坐标
	int X;
	// 水印起始Y轴坐标
	int Y;
	// 水印字体大小
	int FontSize;
	// 16进制的RGB颜色值，如绿色：0x00FF00
	unsigned int FontColor;
	// 水印透明度 0~1
	float Transparency;
	// 水印内容
	char SubTitle[MAX_STRING_LENGTH];
	// 字体名称，字体文件放到库的同目录下，如“arial.ttf”
	// Windows下系统目录使用格式："C\\\\:/Windows/Fonts/msyh.ttc"
	char FontName[MAX_FILE_NAME];
}CarEye_OSDParam;


struct I420Frame
{
	uint8_t* buffer;
	bool key_frame;
};

class H264Packet
{
public:
	H264Packet()
	{
	
	
	
	};
	~H264Packet()
	{
		if (data)
		{
			delete []data;
			data = nullptr;
		}
	

	};

	uint8_t* data=nullptr;
	int size;

};
struct  ACCHLSContext
{
	int64_t startReadPacktTime;
	int timeout;//超时时间
};

/*******************
//1 超时 0 继续等待
********************/
#ifdef __cplusplus
extern "C"
{
#endif
	CE_API int  interruptCallBack(void* ctx);

	CE_API int interrupt_cb(void* context);


#ifdef __cplusplus
}
#endif


/*
* Comments: 使用本库之前必须调用一次本方法
* Param : None
* @Return void
*/
#ifdef __cplusplus
extern "C"
{
#endif
	CE_API void CE_APICALL ffmpeg_init();

#ifdef __cplusplus
}
#endif




