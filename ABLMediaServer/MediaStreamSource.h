#ifndef _MediaStreamSource_H
#define _MediaStreamSource_H

#define   MediaStreamSource_VideoFifoLength   1024*1024*3  //视频存储长度
#define   MediaStreamSource_AudioFifoLength   1024*256     //音频存储长度 
#define   Default_TS_MediaFileByteCount       1024*1024*3  //缺省的切片文件大小 
#define   MaxStoreTsFileCount                 4            //最大允许保持TS文件数量　

#include "hls-fmp4.h"
#include "mpeg-ps.h"
#include "hls-m3u8.h"
#include "hls-media.h"
#include "hls-param.h"
#include "mpeg-ps.h"
#include "mov-format.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"

#include "mov-format.h"
#include "fmp4-writer.h"

#include "MediaFifo.h"
#include "mpeg4-hevc.h"
#include "mpeg4-aac.h"
#include "g711_table.h"
#include "AACEncode.h"
#include "FFVideoDecode.h"
#include "FFVideoEncode.h"
#include <unordered_map>
//#define  WriteCudaDecodeYUVFlag    1
//#define    WriteInputVdideoFlag      1
//#define   WriteInputVideoFileFlag      1 

#define   CudaDecodeH264EncodeH264FIFOBufferLength  2048*1024*1

//用于记录rtsp推流时的SDP信息
struct RtspSDPContentStruct
{
	char szSDPContent[string_length_1024];
	char szVideoName[64];
	int  nVidePayload;

	char szAudioName[64];
	int  nAudioPayload;
	int  nChannels;
	int  nSampleRate;
	RtspSDPContentStruct()
	{
		memset(szSDPContent, 0x00, sizeof(szSDPContent));
		memset(szVideoName, 0x00, sizeof(szVideoName));
		memset(szAudioName, 0x00, sizeof(szAudioName));

		nVidePayload = 0;
		nAudioPayload = 0;
		nChannels = 0;
		nSampleRate = 0 ;
	}
};
#ifdef USE_BOOST
typedef  boost::unordered_map<NETHANDLE, NETHANDLE> MediaSendMap;//媒体发送列表
#else
typedef  std::unordered_map<NETHANDLE, NETHANDLE> MediaSendMap;//媒体发送列表
#endif

#define  OneFrame_MP4_Stream_BufferLength    1024*1024*2 

class CMediaStreamSource
{
public:
   CMediaStreamSource(char* szURL,uint64_t nClientTemp, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct h265ConvertH264Struct);
   ~CMediaStreamSource();

#ifdef WriteInputVideoFileFlag
   FILE*   fWriteInputVideoFile;
#endif
   int                nWebRtcPlayerCount;//webRtc播放次数统计 
   uint64_t           nWebRtcPushStreamID;//给webRTC推流提供者 
#ifdef USE_BOOST
   boost::atomic_bool bCreateWebRtcPlaySourceFlag;//创建webrtc源标志 
#else
   std::atomic<bool> bCreateWebRtcPlaySourceFlag;//创建webrtc源标志 
#endif
   bool            CopyAudioFrameBufer();
   bool            CopyVideoGopFrameBufer();//拷贝一个gop视频帧 

   CMediaFifo      pCacheAudioFifo;
   CMediaFifo      pCopyCacheAudioFifo;
   char            szSnapPicturePath[string_length_512];
   uint64_t        iFrameArriveNoticCount;
   uint64_t        nCreateDateTime;
   volatile bool   m_bNoticeOnPublish;//发版事件是否通知
   volatile bool   m_bPauseFlag;//媒体源是否暂停发流

   bool  SetPause(bool bFlag);
   bool  FFMPEGGetWidthHeight(unsigned char * videooutdata, int videooutdatasize, char* videoName, int * outwidth, int * outheight);

#ifdef  WriteInputVdideoFlag
   FILE*   fWriteInputVideo;
#endif
#ifdef WriteCudaDecodeYUVFlag
   int                    nWriteYUVCount;
   FILE*                  fCudaWriteYUVFile;
#endif
   int                    nSrcWidth, nSrcHeight;//输入原始视频宽、高 

   H265ConvertH264Struct  m_h265ConvertH264Struct;
   bool                   ChangeVideoFilter(char *filterText, int fontSize, char *fontColor, float fontAlpha, int fontLeft, int fontTop);
   CFFVideoFilter*        pFFVideoFilter ;        
   uint64_t               nEncodeCudaChan ;

   void                   GetCreateTSDateTime();
   char                   szCreateTSDateTime[128];//创建ts文件时间
   volatile  bool         enable_hls;
   void                   InitHlsResoure();
   volatile bool          bInitHlsResoureFlag;
   volatile bool          bNoticeClientArriveFlag;//是否通知码流到达
   unsigned int           nVideoBitrate;//视频码流
   unsigned int           nAudioBitrate;//音频码流
   volatile uint64_t      nCalcBitrateTimestamp;//计算码流时间戳
   CMediaFifo             pVideoGopFrameBuffer;//保存最近的一个Gop所有视频帧
   CMediaFifo             pCopyVideoGopFrameBuffer;//拷贝保存最近的一个Gop所有视频帧
   int                    nIDRFrameLengh;//最新的一个I帧长度 
   int                    nCudaWidth, nCudaHeight;
   uint64_t               nCudaDecodeChan;//cuda解码通道
   int                    nCudaDecodeFrameCount;//解码成功后的帧数量
   int                    nEncodeBufferLengthCount;//转码后buffer的总长度
#ifdef OS_System_Windows
   unsigned char*        pCudaDecodeYUVFrame;//解码成功的帧数据
#else
   unsigned char*         pCudaDecodeYUVFrame;//解码成功的帧数据	
#endif
   int                    nCudeDecodeOutLength;//解码成功后的长度

   unsigned char*         pOutEncodeBuffer;
   int                    nOneFrameLength;
   int                    nGetFrameCountLength;
   CFFVideoDecode         videoDecode;
   CAVFrameSWS            avFrameSWS;
   CFFVideoEncode         videoEncode;
   int                    nOutLength;
   volatile  bool         H265ConvertH264_enable; //是否启用了转码
   static int             nConvertObjectCount;//当期转码路数 
   bool                   H265ConvertH264(unsigned char* szVideo, int nLength, char* szVideoCodec);

   uint64_t               tCopyVideoTime; //拷贝视频时间戳 
   int                    netBaseNetType; //媒体提供者的对象网络类型
   char                   szJson[string_length_4096] ;  //生成的json

   bool                   ConvertG711ToAAC(int nCodec, unsigned char* pG711, int nBytes,unsigned char* szOutAAC, int& nAACLength);
   CAACEncode             aacEnc;
   unsigned char          pOutAACData[2048];
   int                    nOutAACDataLength;
   char                   g711ToPCMCache[1024 * 16];
   unsigned char          g711CacheBuffer[8192];
   int                    nG711CacheLength;
   int                    nG711CacheProcessLength;//切割前总长度 
   int                    nG711SplittePos; //切割移动位置 
   int                    nG711ToPCMCacheLength;
   int                    nAACEncodeLength;
   int                    nRetunEncodeLength;
   char                   g711toPCM[string_length_2048];
   char                   g711toPCMResample[2048];

   MediaSourceType        nMediaSourceType;//媒体源类型，实况播放，录像点播
   uint32_t               nMediaDuration;//媒体源时长，单位秒，当录像点播时有效

   char                   szRecordPath[string_length_2048];
   volatile   bool        enable_mp4;//是否录制mp4文件
   uint64_t               recordMP4;

   bool                   GetVideoWidthHeight(char* szVideoCodeName,unsigned char* pVideoData, int nDataLength);

   void                   UpdateVideoFrameSpeed(int nVideoSpeed,int netType);
	   
   std::mutex             mediaTsMp4CutLock; //ts FMP 切片锁 
   int64_t                nTsCutFileCount;
   volatile      bool     hls_init_segmentFlag;
   void                   SaveTsMp4M3u8File(); //统一保存TS、MP4、M3u8 文件 
   unsigned char          pFmp4SPSPPSBuffer[1024 * 128];
   int                    nFmp4SPSPPSLength;

   struct mpeg4_hevc_t    hevc; 
   bool                   H265FrameToFMP4File(unsigned char* szVideoData, int nLength); 
   unsigned  int          FindSpsPosition(char* szVideoCodeName, unsigned char* szVideoBuffer, int nBufferLength, bool &bFind);

   unsigned   char*       pTsFileCacheBuffer;//TS切片临时缓冲区
   int                    nMaxTsFileCacheBufferSize; //当前pTsFileCacheBuffer 字节大小 

   char                  s_bufferH264TS[188];
   int64_t               nVideoOrder ;
   int64_t               nAudioOrder ;
   struct mpeg_ts_func_t tshandler;
   CMediaFifo            tsFileNameFifo;
   int64_t               tsCreateTime;
   CMediaFifo            m3u8FileFifo;
   int64_t               m3u8FileOrder;
   void*                 tsPacketHandle ;
   FILE*                 fTSFileWrite;
   int                   fTSFileWriteByteCount;
   char                  szOutputName[string_length_2048];
   char                  szHookTSFileName[string_length_2048];
   char                  szH264TempBuffer[string_length_2048];
   char                  szM3u8Buffer[string_length_2048];
   int                   avtype;
   int                   flags;
   int64_t               ptsVideo;
   std::map<int, int>    streamsTS;
   int                   ts_stream(void* ts, int codecid);
   bool                  H264H265FrameToTSFile(unsigned char* szVideo, int nLength);
 
   int                   nVideoStampAdd;//视频时间戳增量
   int64_t               nAsyncAudioStamp;//同步的时间点

   bool                  GetRtspSDPContent(RtspSDPContentStruct* sdpContent);
   RtspSDPContentStruct  rtspSDPContent;
   unsigned char         pSPSPPSBuffer[string_length_2048];
   int                   nSPSPPSLength; 

   //检测是否是I帧
   bool                bIFrameFlag;
   bool                CheckVideoIsIFrame(char* szVideoCodecName ,unsigned char* szPVideoData, int nPVideoLength);
   unsigned char       szVideoFrameHead[4];

   bool                 CopyTsFileBuffer(int64_t nTsFileNameOrder, unsigned char* pOutTsBuffer);
   CMediaFifo           mediaFileBuffer[MaxStoreTsFileCount];//切片到内存时的存储FIFO
   int                  nTsFileSizeArray[MaxStoreTsFileCount];//ts切片文件对应的文件字节大小
   int                  GetTsFileSizeByOrder(int64_t nTsFileNameOrder);

   std::mutex           mediaTsFileLock;//媒体切片锁
   char                 szTempBuffer[string_length_2048];
   void                 CopyM3u8Buffer(char* szM3u8Buffer);
   bool                 ReturnM3u8Buffer(char* szOutM3u8);

   void                 ABLDeletePath(char* szDeletePath,char* srcPath);
   void                 CreateSubPathByURL(char* szMediaURL);
   char                 szTSFileSubPath[string_length_2048];//TS文件二级路径，用于补充TS文件的路径
   char                 szHLSPath[string_length_2048]; //HLS路径，包括子路径，比如 D:\ABLMediaServer\www\Media\Camera_00001\

   unsigned char*       pH265Buffer;
   int                  nMp4BufferLength;
   int                  vcl  ;
   int                  update  ;
   unsigned char        s_packet[1024*16];
   unsigned char        szExtenVideoData[4*1024];
   int                  extra_data_sizeH265;
   hls_fmp4_t*          hlsFMP4; 
   int                  track_265 ;
   struct mpeg4_aac_t   aacHandle;
   int                  track_aac ;
   unsigned char        szExtenAudioData[string_length_2048];
   int                  nExtenAudioDataLength;
   int                  nAACLength;

   int64_t              s_dts ;
   int                  nTsFileCount;
   int64_t              nTsFileOrder;
   int64_t              videoDts;
   int64_t              audioDts;
   char                 szDataM3U8[48 * 1024];
   char                 szTsName[string_length_2048] ;
   char                 szM3u8Name[string_length_2048];
   char                 szTempName[string_length_2048];

   bool                 PushVideo(unsigned char* szVideo, int nLength, char* szVideoCodec);
   bool                 PushAudio(unsigned char* szAudio, int nLength, char* szAudioCodec, int nChannels, int SampleRate);
   
   bool                 AddClientToMap(NETHANDLE nClient);
   bool                 DeleteClientFromMap(NETHANDLE nClient);

   char                 m_szURL[string_length_2048]; //比如  /Media/Camera_00001     /Live/Camera_00001 ,  url 建议至少有2级 
   char                 app[string_length_1024];
   char                 stream[string_length_1024];
   MediaCodecInfo       m_mediaCodecInfo;
   char                 sim[string_length_256];

   std::mutex           mediaSendMapLock;
   MediaSendMap         mediaSendMap;//本数据 需要 发送、拷贝的链接列表  

   uint64_t             nClient; //记录是那个链接接收的推流 
   uint64_t             nLastWatchTime, nRecordLastWatchTime, nLastWatchTimeDisconect;//最后观看时间
   volatile bool        bUpdateVideoSpeed;//是否更新视频速度
};

#endif
