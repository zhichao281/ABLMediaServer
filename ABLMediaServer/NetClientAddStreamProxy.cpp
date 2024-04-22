/*
功能：
    负责控制主动拉流 rtsp\rtmp\flv\hls 
	检测拉流是否断线、断线重连、彻底销毁拉流等等功能 
	 
日期    2021-07-27
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientAddStreamProxy.h"
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

CNetClientAddStreamProxy::CNetClientAddStreamProxy(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL, szShareMediaURL);
	netBaseNetType = NetBaseNetType_addStreamProxyControl;
	nMediaClient = 0;
	nServer = hServer;
	nClient = hClient;
	WriteLog(Log_Debug, "CNetClientAddStreamProxy 构造 = %X nClient = %llu ", this, hClient);
}


CNetClientAddStreamProxy::~CNetClientAddStreamProxy()
{
	pDisconnectBaseNetFifo.push((unsigned char*)&nMediaClient,sizeof(nMediaClient));

	WriteLog(Log_Debug, "CNetClientAddStreamProxy 析构 = %X nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetClientAddStreamProxy::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetClientAddStreamProxy::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetClientAddStreamProxy::SendVideo()
{
	return 0;
}

int CNetClientAddStreamProxy::SendAudio()
{

	return 0;
}

int CNetClientAddStreamProxy::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CNetClientAddStreamProxy::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetClientAddStreamProxy::SendFirstRequst()
{
	nProxyDisconnectTime = GetTickCount64();

	if (strlen(m_szShareMediaURL) > 0)
	{
#ifdef USE_BOOST
		boost::shared_ptr<CNetRevcBase> pClient = NULL;
#else
		std::shared_ptr<CNetRevcBase> pClient = NULL;

#endif
	

		if (nServer == NetRevcBaseClient_addStreamProxyControl)//自研代理拉流 
			pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, m_addStreamProxyStruct.url, 0, m_szShareMediaURL);
		else if (nServer == NetRevcBaseClient_addFFmpegProxyControl)//调用ffmepg函数实现代理拉流
			pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addFFmpegProxy, 0, 0, m_addStreamProxyStruct.url, 0, m_szShareMediaURL);

		if (pClient)
		{
		 ParseRtspRtmpHttpURL(m_addStreamProxyStruct.url);
		 strcpy(szClientIP, m_rtspStruct.szIP);
		 nClientPort = atoi(m_rtspStruct.szPort);
		 nMediaClient = pClient->nClient;
		 memcpy((char*)&pClient->m_addStreamProxyStruct, (char*)&m_addStreamProxyStruct, sizeof(m_addStreamProxyStruct));
		 memcpy((char*)&pClient->m_h265ConvertH264Struct, (char*)&m_h265ConvertH264Struct, sizeof(m_h265ConvertH264Struct));
		 m_nXHRtspURLType = atoi(m_addStreamProxyStruct.isRtspRecordURL);

		 pClient->nClient_http = nClient_http ;//把http 连接号 赋给 rtsp 对象中的 nClient_http ;
		 pClient->hParent      = nClient ;     //把代理连接号 赋值给rtsp对象中的 父连接号 
		 WriteLog(Log_Debug, "CNetClientAddStreamProxy->nClient_http = %d ",nClient_http );
	  }
 	}

	 return 0;
}

//请求m3u8文件
bool  CNetClientAddStreamProxy::RequestM3u8File()
{
	return true;
}