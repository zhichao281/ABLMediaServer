#ifndef _ReadRecordFileInput_H
#define _ReadRecordFileInput_H

#include "mov-reader.h"
#include "mov-format.h"
#include "mpeg4-hevc.h"
#include "mpeg4-avc.h"
#include "mpeg4-aac.h"
#include "opus-head.h"
#include "webm-vpx.h"
#include "aom-av1.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif
#define     ReadRecordFileInput_MaxPacketCount     1024*1024*3 
#define     OpenMp4FileToReadWaitMaxMilliSecond    300  //打开mp4文件，500毫秒后 才开始读取文件 
//#define     WriteAACFileFlag                       1    //是否保存AAC文件

//视频，音频
enum ABLAVType
{
	AVType_Video = 0, //视频
	AVType_Audio = 1, //音频 
};

class CReadRecordFileInput : public CNetRevcBase
{
public:
	CReadRecordFileInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
    ~CReadRecordFileInput() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   char           szFileNameUTF8[512] ;
   int            open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);
   void           AddADTSHeadToAAC(unsigned char* szData, int nAACLength);
   AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx = NULL;
   AVStream       *video_stream = NULL, *audio_stream = NULL;
   enum AVPixelFormat pix_fmt;

#ifdef WriteAACFileFlag
   FILE*                 fWriteAAC;
#endif 

   int64_t               nCurrentDateTime;
   CMediaFifo            m_audioCacheFifo;
   int                   nInputAudioDelay;
   int64_t               nInputAudioTime;

   int                   sample_index;
   int                   ret1, ret2;
   AVBitStreamFilter *   buffersrc;
   AVBSFContext *        bsf_ctx;
   AVCodecParameters *   codecpar;
   AVFormatContext *     pFormatCtx2;
   AVPacket*             packet2;
   int                   stream_isVideo;
   int                   stream_isAudio;
   int64_t               nRangeCount;
   unsigned char   pAACBufferADTS[2048];//增加adts头的AAC数据 

   uint64_t             mov_readerTime;//上一次读取时间
   volatile  int        nWaitTime ;//需要等待的时间然后进行读取
   bool                 ReaplyFileSeek(uint64_t nTimestamp);
   bool                 GetMediaShareURLFromFileName(char* szRecordFileName, char* szMediaURL);
   bool                 UpdateReplaySpeed(double dScaleValue, ABLRtspPlayerType rtspPlayerType);
   bool                 UpdatePauseFlag(bool bFlag);

   uint64_t              nDownloadFrameCount;
#ifdef USE_BOOST

	boost::shared_ptr<CMediaStreamSource> pMediaSource;

#else
	std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif
   int                   nRetLength;
   std::mutex            readRecordFileInputLock;
   unsigned char         s_packet[ReadRecordFileInput_MaxPacketCount];
   unsigned char         audioBuffer[4096];

   int64_t               nReadRet;

   volatile  ABLAVType   nAVType, nOldAVType;//上一帧媒体类型 
   uint64_t              nOldPTS;
   volatile   int        nVidepSpeedTime;//视频帧速度 
   volatile   double     dBaseSpeed ;
   volatile   double     m_dScaleValue;//当前速度
   volatile   uint64_t   m_nStartTimestamp;//开始的时间戳
   uint64_t              nVideoFirstPTS;
   uint64_t              nAudioFirstPTS;

   volatile   bool       bRestoreVideoFrameFlag;//是否需要恢复视频帧总数
   volatile   bool       bRestoreAudioFrameFlag;//是否需要恢复音频帧总数
};

#endif