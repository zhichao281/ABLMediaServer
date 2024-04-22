#ifndef _NetClientWebrtcPlayer_H
#define _NetClientWebrtcPlayer_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else
#include <memory>
#endif
#include "./ffmpeg/ffmpeg_headers.h"
#include "./ffmpeg/thread_pool.h"
//#define  WebRtcVideoFileFlag     1 //写入webrtc视频数据

class CNetClientWebrtcPlayer : public CNetRevcBase
{
public:
	CNetClientWebrtcPlayer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetClientWebrtcPlayer() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

   int                                   nSpsPositionPos;

#ifdef WebRtcVideoFileFlag
   FILE*     fWriteVideoFile;
   int64_t   nWriteFileCount;
   FILE*     fWriteFrameLengthFile;
#endif

   AudioResamplerAPI* m_resampler = nullptr;
   FFmpegAudioDecoderAPI* m_decder = nullptr;
   int m_nb_channels = 2;

   int m_sample_rate = 48000;

   std::atomic<bool> stopThread;
};

#endif