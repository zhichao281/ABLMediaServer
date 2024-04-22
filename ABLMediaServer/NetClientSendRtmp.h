#ifndef _NetClientSendRtmp_H
#define _NetClientSendRtmp_H

#include "rtmp-client.h"
#include "flv-writer.h"
#include "flv-proto.h"
#include "flv-demuxer.h"
#include "flv-muxer.h"

#include "MediaStreamSource.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif


//#define  WriteFlvFileByDebug   1 //用于定义是否写FLV文件 

//#define    WriteFlvToEsFileFlagSend  1 //用于写FLV解包后的ES流，测试flv解包视频、音频是否正确 

class CNetClientSendRtmp : public CNetRevcBase
{
public:
	CNetClientSendRtmp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetClientSendRtmp() ;

   uint32_t       nVideoDTS ;
   uint32_t       nAudioDTS;
   uint32_t       nWriteRet;
   volatile  int  nWriteErrorCount;
   char           szRtmpName[string_length_1024];

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   struct rtmp_client_handler_t handler;
   rtmp_client_t*               rtmp;
   volatile bool                bCheckRtspVersionFlag;

   int64_t                      nAsyncAudioStamp;

   flv_muxer_t*                 flvMuxer;
   char                         szURL[string_length_2048];

   volatile bool                bDeleteRtmpPushH265Flag; //因为推rtmp265被删除标志 

   volatile bool                bAddMediaSourceFlag;//是否加入媒体库
   int                          nRtmpState;        //rtmp状态 
   int                          nRtmpState3Count; //状态3的次数 

#ifdef  WriteFlvFileByDebug
   void*                        s_flv;
#endif

#ifdef  WriteFlvToEsFileFlagSend
   FILE*                      fWriteVideo;
   FILE*                      fWriteAudio;
#endif
};

#endif