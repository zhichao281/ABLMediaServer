/*
功能：
    负责控制主动推流 rtsp\rtmp 
	检测推流是否断线、断线重连、彻底销毁拉流等等功能 
	 
日期    2021-07-27
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientAddPushProxy.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);

#endif

CNetClientAddPushProxy::CNetClientAddPushProxy(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_addPushProxyControl;
	nMediaClient = 0;
	nCreateDateTime = GetTickCount64();
	WriteLog(Log_Debug, "CNetClientAddPushProxy 构造 = %X  nClient = %llu ", this, nClient);
}

CNetClientAddPushProxy::~CNetClientAddPushProxy()
{
	pDisconnectBaseNetFifo.push((unsigned char*)&nMediaClient,sizeof(nMediaClient));

	WriteLog(Log_Debug, "CNetClientAddPushProxy 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetClientAddPushProxy::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetClientAddPushProxy::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetClientAddPushProxy::SendVideo()
{
	return 0;
}

int CNetClientAddPushProxy::SendAudio()
{

	return 0;
}

int CNetClientAddPushProxy::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CNetClientAddPushProxy::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetClientAddPushProxy::SendFirstRequst()
{
	if (strlen(m_szShareMediaURL) > 0 && strlen(m_addPushProxyStruct.url) > 0)
	{
	  auto pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addPushStreamProxy, 0, 0, m_addPushProxyStruct.url, 0, m_szShareMediaURL);
	  if (pClient)
	  {
		 nMediaClient = pClient->nClient;
		 memcpy((char*)&pClient->m_addPushProxyStruct, (char*)&m_addPushProxyStruct, sizeof(m_addPushProxyStruct));

		 pClient->nClient_http = nClient_http;//把http 连接号 赋给 rtsp 对象中的 nClient_http ;
		 pClient->hParent = nClient;     //把代理连接号 赋值给rtsp对象中的 父连接号 
		 WriteLog(Log_Debug, "CNetClientAddPushProxy->nClient_http = %d ", nClient_http);
	  }
 	}

	 return 0;
}

//请求m3u8文件
bool  CNetClientAddPushProxy::RequestM3u8File()
{
	return true;
}