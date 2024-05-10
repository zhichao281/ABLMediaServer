#ifndef _NetServerHTTP_H
#define _NetServerHTTP_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else
#include <memory>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <random>
#endif

#define Send_ResponseHttp_MaxPacketCount   1024*48  //回复http包最大发送一次字节

#define  MaxNetServerHttpBuffer      1024*1024*2 

#define  MaxMediaSourceInfoLength    1024*1024*6   
typedef  map<string, RequestKeyValue*, less<string> > RequestKeyValueMap;

class CNetServerHTTP : public CNetRevcBase
{
public:
	CNetServerHTTP(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetServerHTTP() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   bool                    index_api_controlRecordPlay();
   bool                    RequesePauseRtpServer(bool bFlag);
   bool                    index_api_pauseRtpServer();
   bool                    index_api_resumeRtpServer();
   bool                    index_api_setServerConfig();
   bool                    index_api_listServerPort();
   bool                    index_api_getTranscodingCount();
   bool                    WriteParamValue(char* szSection, char* szKey, char* szValue);
   bool                    index_api_restartServer();
   bool                    index_api_shutdownServer();
   int                     bindRtpServerPort();//绑定国标接收端口
   bool                    index_api_setConfigParamValue();
   bool                    index_api_setTransFilter();
   bool                    index_api_controlStreamProxy();
   bool                    index_api_downloadImage(char* szHttpURL);
   bool                    index_api_queryPictureList();
   bool                    index_api_getSnap();
   bool                    index_api_queryRecordList();
   bool                    index_api_startStopRecord(bool bFlag);

   bool                    index_api_close_streams();
   bool                    index_api_getOutList();

   //index/api/各种函数 
   bool                    index_api_getServerConfig();
   bool                    index_api_getMediaList();

   bool                    index_api_startSendRtp();

   bool                    index_api_openRtpServer();
 
   bool                    index_api_addPushProxy();
 
   bool                    index_api_addStreamProxy(NetRevcBaseClientType nType);
   bool                    index_api_delRequest();

   bool                    ResponseSuccess(char* szSuccessInfo);
   bool                    GetKeyValue(char* key, char* value);
   void                    DeleteAllHttpKeyValue();
   bool                    SplitterTextParam(char* szTextParam);
   bool                    SplitterJsonParam(char* szJsonParam);
   bool                    ResponseHttpRequest(char* szModem, char* httpURL, char* requestParam);
   int                     CheckHttpHeadEnd();

   RequestKeyValueMap      requestKeyValueMap;
   CABLSipParse            httpParse;
   std::mutex              NetServerHTTPLock;
   unsigned char           netDataCache[MaxNetServerHttpBuffer]; //网络数据缓存
   int                     netDataCacheLength;//网络数据缓存大小
   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
   char                    szHttpHead[1024 * 64];
   char                    szHttpBody[1024 * 64];
   char                    szContentLength[128];
   int                     nContent_Length = 0;
   char                    szHttpPath[1024 * 64];
   char                    szConnection[string_length_2048];
   char                    szMediaSourceInfoBuffer[MaxMediaSourceInfoLength];
   int64_t                 nDelKey;
};

#endif