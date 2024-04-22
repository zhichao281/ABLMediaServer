#ifndef  _cudeCodecDLL_H
#define  _cudeCodecDLL_H

#include <stdint.h>
#ifdef WIN32

#include <Windows.h>
#ifdef CUDACODECDLL_EXPORTS
#define CUDACODECDLL_API __declspec(dllexport)
#else
#define CUDACODECDLL_API __declspec(dllimport)
#endif

#else
#define CUDACODECDLL_API 
#define WINAPI 
#endif // WIN32



//视频格式 
typedef enum cudaCodecVideo_enum {
	cudaCodecVideo_MPEG1 = 0,                                         /**<  MPEG1             */
	cudaCodecVideo_MPEG2,                                           /**<  MPEG2             */
	cudaCodecVideo_MPEG4,                                           /**<  MPEG4             */
	cudaCodecVideo_VC1,                                             /**<  VC1               */
	cudaCodecVideo_H264,                                            /**<  H264              */
	cudaCodecVideo_JPEG,                                            /**<  JPEG              */
	cudaCodecVideo_H264_SVC,                                        /**<  H264-SVC          */
	cudaCodecVideo_H264_MVC,                                        /**<  H264-MVC          */
	cudaCodecVideo_HEVC,                                            /**<  HEVC              */
	cudaCodecVideo_VP8,                                             /**<  VP8               */
	cudaCodecVideo_VP9,                                             /**<  VP9               */
	cudaCodecVideo_NumCodecs,                                       /**<  Max codecs        */
	// Uncompressed YUV
	cudaCodecVideo_YUV420 = (('I' << 24) | ('Y' << 16) | ('U' << 8) | ('V')),   /**< Y,U,V (4:2:0)      */
	cudaCodecVideo_YV12 = (('Y' << 24) | ('V' << 16) | ('1' << 8) | ('2')),   /**< Y,V,U (4:2:0)      */
	cudaCodecVideo_NV12 = (('N' << 24) | ('V' << 16) | ('1' << 8) | ('2')),   /**< Y,UV  (4:2:0)      */
	cudaCodecVideo_YUYV = (('Y' << 24) | ('U' << 16) | ('Y' << 8) | ('V')),   /**< YUYV/YUY2 (4:2:2)  */
	cudaCodecVideo_UYVY = (('U' << 24) | ('Y' << 16) | ('V' << 8) | ('Y'))    /**< UYVY (4:2:2)       */
} cudaCodecVideo;

/*
功能：
   初始化
参数：
   无
*/
typedef bool (WINAPI* ABL_cudaDecode_Init) ();

/*
功能：
   获取英伟达显卡数量
参数
   无
返回值：
   0   没有英伟达显卡
   N   有N个英伟达显卡
*/
typedef int (WINAPI* ABL_cudaDecode_GetDeviceGetCount) ();

/*
功能:
   获取英伟达显卡名称
参数：
   int    nOrder   显卡序号
   char*  szName   显卡名称
返回值
   true            获取成功
   false           获取失败 
*/
typedef bool (WINAPI* ABL_cudaDecode_GetDeviceName) (int nOrder, char* szName);

/*
功能:
   获取英伟达显卡的使用率
参数：
   int    nOrder   显卡序号

返回值
   显卡使用率 
*/
typedef int (WINAPI* ABL_cudaDecode_GetDeviceUse) (int nOrder);

/*
功能:
   创建视频解码句柄
参数：
   cudaCodecVideo_enum videoCodec,      待解码的视频格式 cudaCodecVideo_H264 、 cudaCodecVideo_HEVC 等等 
   cudaCodecVideo_enum outYUVType       解码后支持输出的YUV格式，现在只支持输出两种YUV格式　cudaCodecVideo_NV12　、cudaCodecVideo_YV12　
   int                 nWidth,          比如 1920 
   int                 nHeight,         比如 1080 
   uint64_t&           nCudaChan        如果成功，返回大于0 的数字 

返回值
 
*/
typedef bool (WINAPI* ABL_CreateVideoDecode) (cudaCodecVideo_enum videoCodec, cudaCodecVideo_enum outYUVType, int nWidth, int nHeight, uint64_t& nCudaChan);

/*
功能：
  调用cuda解码视频 
参数：
  uint64_t        nCudaChan           cuda通道号
  unsigned char*  pVideoData          未解码数据
  int             nVideoLength        数据长度
  int             nDecodeFrameCount   解码成功后返回多少帧， 1、4 等等 
  int&            nOutDecodeLength    解码返回一帧buffer长度 
*/
#ifdef  WIN32
typedef unsigned char* (WINAPI* ABL_CudaVideoDecode) (uint64_t nCudaChan, unsigned char* pVideoData, int nVideoLength, int& nDecodeFrameCount, int& nOutDecodeLength);

#else
typedef unsigned char* (WINAPI* ABL_CudaVideoDecode) (uint64_t nCudaChan, unsigned char* pVideoData, int nVideoLength, int& nDecodeFrameCount, int& nOutDecodeLength);

#endif

/*
功能：
   删除cuda视频解码句柄
参数：
uint64_t        nCudaChan           cuda通道号
 
*/
typedef bool (WINAPI* ABL_DeleteVideoDecode) (uint64_t nCudaChan);

/*
功能：
  返回已经启用CUDA硬解总数量，包括多路显卡硬解
参数：
无
*/
typedef int (WINAPI* ABL_GetCudaDecodeCount) ();

/*
功能：
  反初始化
参数：
  无
*/
typedef bool (WINAPI* ABL_VideoDecodeUnInit) ();

#endif

