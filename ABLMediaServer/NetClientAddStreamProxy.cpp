/*
���ܣ�
    ��������������� rtsp\rtmp\flv\hls 
	��������Ƿ���ߡ������������������������ȵȹ��� 
	 
����    2021-07-27
����    �޼��ֵ�
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

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CNetRevcBase>  CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>  CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);

#endif

CNetClientAddStreamProxy::CNetClientAddStreamProxy(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_addStreamProxyControl;
	nMediaClient = 0;
	nServer = hServer;
	nClient = hClient;
	WriteLog(Log_Debug, "CNetClientAddStreamProxy ���� = %X nClient = %llu ", this, hClient);
}

CNetClientAddStreamProxy::~CNetClientAddStreamProxy()
{
	pDisconnectBaseNetFifo.push((unsigned char*)&nMediaClient,sizeof(nMediaClient));

	WriteLog(Log_Debug, "CNetClientAddStreamProxy ���� = %X nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
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

//���͵�һ������
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
	  if(nServer == NetRevcBaseClient_addStreamProxyControl)//���д������� 
	    pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, m_addStreamProxyStruct.url, 0, m_szShareMediaURL);
	  else if(nServer == NetRevcBaseClient_addFFmpegProxyControl)//����ffmepg����ʵ�ִ�������
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

		 pClient->nClient_http = nClient_http ;//��http ���Ӻ� ���� rtsp �����е� nClient_http ;
		 pClient->hParent      = nClient ;     //�Ѵ������Ӻ� ��ֵ��rtsp�����е� �����Ӻ� 
		 WriteLog(Log_Debug, "CNetClientAddStreamProxy->nClient_http = %d ",nClient_http );
	  }
 	}

	 return 0;
}

//����m3u8�ļ�
bool  CNetClientAddStreamProxy::RequestM3u8File()
{
	return true;
}