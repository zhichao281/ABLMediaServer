/*
���ܣ�
 	 ʵ�֣������ͻ��ˣ�����֪ͨ�����¼���Ϣ����Ϣ���ݴӡ�PushVideo���������룬���̳߳ظ����͡�
	 ͨ��������netBaseNetType ��ָ����������֪ͨ��Ϣ��

����    2022-03-12
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientHttp.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern CNetBaseThreadPool*                   MessageSendThreadPool;//��Ϣ�����̳߳�
extern CMediaFifo                            pMessageNoticeFifo; //��Ϣ֪ͨFIFO
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern CNetBaseThreadPool* MessageSendThreadPool;//��Ϣ�����̳߳�
extern CMediaFifo                            pMessageNoticeFifo; //��Ϣ֪ͨFIFO
#endif
extern void LIBNET_CALLMETHOD	             onconnect(NETHANDLE clihandle,uint8_t result, uint16_t nLocalPort);
extern void LIBNET_CALLMETHOD                onread(NETHANDLE srvhandle,NETHANDLE clihandle,uint8_t* data,uint32_t datasize,void* address);
extern void LIBNET_CALLMETHOD	             onclose(NETHANDLE srvhandle,NETHANDLE clihandle);

CNetClientHttp::CNetClientHttp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nCreateDateTime = GetTickCount64();
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_HttpClient_None_reader;
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	nNetStart = nNetEnd = netDataCacheLength = 0;
 
 	if (ParseRtspRtmpHttpURL(szIP) == true)
 	  uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1, memcmp(m_rtspStruct.szSrcRtspPullUrl, "https://", 8) == 0 ? true : false );
 
	m_videoFifo.InitFifo(MaxNetClientHttpBuffer);
	string strResponeURL = szClientIP;
	int nPos = 0;
	nPos = strResponeURL.find("/", 10);
	if (nPos > 0 && nPos != string::npos)
	{
		memset(szResponseURL, 0x00, sizeof(szResponseURL));
		memcpy(szResponseURL, szClientIP + nPos, strlen(szClientIP) - nPos);
	}
	nMediaClient = 0;
	WriteLog(Log_Debug, "CNetClientHttp ���� = %X nClient = %llu , szResponseURL = %s ", this, nClient, szResponseURL);
}

CNetClientHttp::~CNetClientHttp()
{
	bRunFlag.exchange(false);
	std::lock_guard<std::mutex> lock(NetClientHTTPLock);

	m_videoFifo.FreeFifo();

	WriteLog(Log_Debug, "CNetClientHttp ���� = %X nClient = %llu ,nMediaClient = %llu", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetClientHttp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag.load())
		return -1;

	m_videoFifo.push(pVideoData, nDataLength);

	return 0;
}

int CNetClientHttp::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetClientHttp::SendVideo()
{

	return 0;
}

int CNetClientHttp::SendAudio()
{

	return 0;
}

int CNetClientHttp::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag.load())
		return -1;
	nRecvDataTimerBySecond = 0;
	std::lock_guard<std::mutex> lock(NetClientHTTPLock);

	if (MaxNetClientHttpBuffer - nNetEnd >= nDataLength)
	{//ʣ��ռ��㹻
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//ʣ��ռ䲻������Ҫ��ʣ���buffer��ǰ�ƶ�
		if (netDataCacheLength > 0)
		{//���������ʣ��
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;

			//�ѿ����buffer��� 
			memset(netDataCache + nNetEnd, 0x00, MaxNetClientHttpBuffer - nNetEnd);

			if (MaxNetClientHttpBuffer - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				memset(netDataCache, 0x00, MaxNetClientHttpBuffer);
				WriteLog(Log_Debug, "CNetClientHttp = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return 0;
			}
		}
		else
		{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
			nNetStart = nNetEnd = netDataCacheLength = 0;
			memset(netDataCache, 0x00, MaxNetClientHttpBuffer);
		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	return 0;
}

int CNetClientHttp::ProcessNetData()
{
	if (!bRunFlag.load())
		return -1;
	std::lock_guard<std::mutex> lock(NetClientHTTPLock);

	unsigned char* pData = NULL;
	int            nLength = 0;

	if ((pData = m_videoFifo.pop(&nLength)) != NULL && strlen(szResponseURL) > 0 )
	{
		memset(szResponseData, 0x00, sizeof(szResponseData));
 		memcpy(szResponseData, pData, nLength);
 
		HttpRequest(szResponseURL, szResponseData, nLength);

		m_videoFifo.pop_front();
	}

	if(m_videoFifo.GetSize() > 0 )
	  MessageSendThreadPool->InsertIntoTask(nClient);
 
	return 0;
}

//���͵�һ������
int CNetClientHttp::SendFirstRequst()
{
	if (!bRunFlag.load())
		return -1;

	bConnectSuccessFlag = true;

	MessageSendThreadPool->InsertIntoTask(nClient);
 
	return 0;
}

//����m3u8�ļ�
bool  CNetClientHttp::RequestM3u8File()
{
	return true;
}

void CNetClientHttp::HttpRequest(char* szUrl, char* szBody,int nLength)
{
	if (nLength > 0)
	 szBody[0] = '{';

	if (!bConnectSuccessFlag || strlen(szBody) == 0 || nLength <= 0 || !bRunFlag.load())
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
 		return;
	}

 	memset(szResponseHttpHead, 0X00, sizeof(szResponseHttpHead));
 	sprintf(szResponseHttpHead, "POST %s HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-CN,zh;q=0.8\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: application/json\r\nHost: 127.0.0.1\r\nTools: %s\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36\r\n\r\n",szUrl, nLength, MediaServerVerson);
	memcpy(szResponseHttpHead+strlen(szResponseHttpHead), szBody,nLength);
	szBody[nLength] = 0x00;

	int nRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), ABL_MediaServerPort.nSyncWritePacket);
	if (nRet != 0)
	{
		WriteLog(Log_Debug, "CNetClientHttp = %X nClient = %llu ,netBaseNetType = %d HttpRequest() ֪ͨ��Ϣ����ʧ�� ", this, nClient, netBaseNetType);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));

		return;
	}

	WriteLog(Log_Debug, "CNetClientHttp = %X nClient = %llu ,netBaseNetType = %d \r\nURL = %s \r\nbody = %s ", this, nClient, netBaseNetType,szUrl,szBody);
}
