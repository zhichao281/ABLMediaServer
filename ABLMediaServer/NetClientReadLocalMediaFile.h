#ifndef _NetBaseNetType_ReadLocalMediaFile_H
#define _NetBaseNetType_ReadLocalMediaFile_H

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

#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else
#include <memory>
#include <unordered_map>
#endif


#define     ReadRecordFileInput_MaxPacketCount     1024*1024*3 
//#define     WriteAACFileFlag                       1    //�Ƿ񱣴�AAC�ļ�

class CNetClientReadLocalMediaFile : public CNetRevcBase
{
public:
	CNetClientReadLocalMediaFile(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
    ~CNetClientReadLocalMediaFile() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   char           szReadFileError[512];
   char           szFileNameUTF8[512] ;
   int            open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);
   void           AddADTSHeadToAAC(unsigned char* szData, int nAACLength);
   AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx = NULL;
   AVStream       *video_stream = NULL, *audio_stream = NULL;
   enum AVPixelFormat pix_fmt;
   volatile  bool bResponseHttpFlag;

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
   unsigned char   pAACBufferADTS[2048];//����adtsͷ��AAC���� 

   uint64_t             mov_readerTime;//��һ�ζ�ȡʱ��
   volatile  int        nWaitTime ;//��Ҫ�ȴ���ʱ��Ȼ����ж�ȡ
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
   unsigned char         audioBuffer[4096];

   int64_t               nReadRet;

   volatile  ABLAVType   nAVType, nOldAVType;//��һ֡ý������ 
   uint64_t              nOldPTS;
   volatile   int        nVidepSpeedTime;//��Ƶ֡�ٶ� 
   volatile   double     dBaseSpeed ;
   volatile   double     m_dScaleValue;//��ǰ�ٶ�
   volatile   uint64_t   m_nStartTimestamp;//��ʼ��ʱ���
   uint64_t              nVideoFirstPTS;
   uint64_t              nAudioFirstPTS;

   volatile   bool       bRestoreVideoFrameFlag;//�Ƿ���Ҫ�ָ���Ƶ֡����
   volatile   bool       bRestoreAudioFrameFlag;//�Ƿ���Ҫ�ָ���Ƶ֡����
};

#endif