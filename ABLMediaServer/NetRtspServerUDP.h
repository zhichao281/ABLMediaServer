#ifndef _NetRtspServerUDP_H
#define _NetRtspServerUDP_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;

#else

#include <memory>
#include <unordered_map>
#endif
//#define  WriteRtpDecodeFile     1

class CNetRtspServerUDP : public CNetRevcBase
{
public:
	CNetRtspServerUDP(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetRtspServerUDP() ;

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
 
#ifdef WriteRtpDecodeFile
   FILE*          fWriteVideoFile;
   FILE*          fWriteAudioFile;
#endif
#ifdef USE_BOOST
   bool           CreateVideoRtpDecode(boost::shared_ptr<CMediaStreamSource> mediaServer, char* VideoName,int nVidoePT);
   bool           CreateAudioRtpDecode(boost::shared_ptr<CMediaStreamSource> mediaServer, char* AudioName, int nAudioPT,int Channels,int SampleRate,int nSampleIndex);
 
#else
   bool           CreateVideoRtpDecode(std::shared_ptr<CMediaStreamSource> mediaServer, char* VideoName, int nVidoePT);
   bool           CreateAudioRtpDecode(std::shared_ptr<CMediaStreamSource> mediaServer, char* AudioName, int nAudioPT, int Channels, int SampleRate, int nSampleIndex);


#endif
  void           AddADTSHeadToAAC(unsigned char* szData, int nAACLength); 
  void           SplitterRtpAACData(unsigned char* rtpAAC, int nLength);
   void           SplitterMp3Buffer(unsigned char* szMp3Buffer, int nLength);

   unsigned char  szFullMp3Buffer[2048];
   unsigned char  szMp3HeadFlag[8];
   int            au_header_length;
   uint8_t        *ptr, *pau, *pend;
   int            au_size; // only AU-size
   int            au_numbers;
   int            SplitterSize[16];
   unsigned char  aacData[string_length_2048];

   uint32_t       hRtpHandle[2];
   char           szVideoName[64];
   char           szAudioName[64];
   int            nVideoPayload;
   int            nAudioPayload;
   int            sample_index;//采样频率所对应的序号 
   int            nChannels; //音频通道数
   int            nSampleRate; //音频采样频率
};

#endif