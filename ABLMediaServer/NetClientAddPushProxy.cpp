/*
���ܣ�
    ��������������� rtsp\rtmp 
	��������Ƿ���ߡ������������������������ȵȹ��� 
	 
����    2021-07-27
����    �޼��ֵ�
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

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��

extern boost::shared_ptr<CNetRevcBase>  CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL,  bool bLock = true);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>  CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL,  bool bLock = true);

#endif

CNetClientAddPushProxy::CNetClientAddPushProxy(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_addPushProxyControl;
	nMediaClient = 0;
	nCreateDateTime = GetTickCount64();
	WriteLog(Log_Debug, "CNetClientAddPushProxy ���� = %X  nClient = %llu ", this, nClient);
}

CNetClientAddPushProxy::~CNetClientAddPushProxy()
{
	pDisconnectBaseNetFifo.push((unsigned char*)&nMediaClient,sizeof(nMediaClient));

	WriteLog(Log_Debug, "CNetClientAddPushProxy ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
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

//���͵�һ������
int CNetClientAddPushProxy::SendFirstRequst()
{
	if (strlen(m_szShareMediaURL) > 0 && strlen(m_addPushProxyStruct.url) > 0)
	{
	  auto pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addPushStreamProxy, 0, 0, m_addPushProxyStruct.url, 0, m_szShareMediaURL);
	  if (pClient)
	  {
		 nMediaClient = pClient->nClient;
		 memcpy((char*)&pClient->m_addPushProxyStruct, (char*)&m_addPushProxyStruct, sizeof(m_addPushProxyStruct));

		 pClient->nClient_http = nClient_http;//��http ���Ӻ� ���� rtsp �����е� nClient_http ;
		 pClient->hParent = nClient;     //�Ѵ������Ӻ� ��ֵ��rtsp�����е� �����Ӻ� 
		 WriteLog(Log_Debug, "CNetClientAddPushProxy->nClient_http = %d ", nClient_http);
	  }
 	}

	 return 0;
}

//����m3u8�ļ�
bool  CNetClientAddPushProxy::RequestM3u8File()
{
	return true;
}