//#ifndef _NetClientWebrtcPlayer_H
//#define _NetClientWebrtcPlayer_H
//#ifdef USE_BOOST
//#include <boost/unordered/unordered_map.hpp>
//#include <boost/smart_ptr/shared_ptr.hpp>
//#include <boost/unordered/unordered_map.hpp>
//#include <boost/make_shared.hpp>
//#include <boost/algorithm/string.hpp>
//
//using namespace boost;
//#else
//#include <memory>
//#endif
//#include "./ffmpeg/ffmpeg_headers.h"
//#include "./ffmpeg/thread_pool.h"
////#define  WebRtcVideoFileFlag     1 //д��webrtc��Ƶ����
//
//class CNetClientWebrtcPlayer : public CNetRevcBase
//{
//public:
//	CNetClientWebrtcPlayer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
//   ~CNetClientWebrtcPlayer() ;
//
//   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
//   virtual int ProcessNetData();
//
//   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
//   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
//   virtual int SendVideo();//������Ƶ����
//   virtual int SendAudio();//������Ƶ����
//   virtual int SendFirstRequst();//���͵�һ������
//   virtual bool RequestM3u8File();//����m3u8�ļ�
//#ifdef USE_BOOST
//   boost::shared_ptr<CMediaStreamSource> pMediaSource;
//#else
//   std::shared_ptr<CMediaStreamSource> pMediaSource;
//#endif
//
//   int                                   nSpsPositionPos;
//
//#ifdef WebRtcVideoFileFlag
//   FILE*     fWriteVideoFile;
//   int64_t   nWriteFileCount;
//   FILE*     fWriteFrameLengthFile;
//#endif
//
//   AudioResamplerAPI* m_resampler = nullptr;
//   FFmpegAudioDecoderAPI* m_AudioDecder = nullptr;
//   int m_nb_channels = 2;
//
//   int m_sample_rate = 48000;
//
//   std::atomic<bool> stopThread;
//   uint8_t* outData[AV_NUM_DATA_POINTERS] = { 0 };
//};
//
//#endif