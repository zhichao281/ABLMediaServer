/*
功能：
 	 实现ｈｔｔｐ客户端，用于通知各种事件消息，消息内容从　PushVideo　函数传入，由线程池负责发送　
	 通过变量　netBaseNetType 来指定发送哪种通知消息　

日期    2022-03-12
作者    罗家兄弟
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

extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern CNetBaseThreadPool*                   MessageSendThreadPool;//消息发送线程池
extern CMediaFifo                            pMessageNoticeFifo; //消息通知FIFO
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern CNetBaseThreadPool* MessageSendThreadPool;//消息发送线程池
extern CMediaFifo                            pMessageNoticeFifo; //消息通知FIFO
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
	bRunFlag = true;

	if (ParseRtspRtmpHttpURL(szIP) == true)
		uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);

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
	WriteLog(Log_Debug, "CNetClientHttp 构造 = %X nClient = %llu , szResponseURL = %s ", this, nClient, szResponseURL);
}

CNetClientHttp::~CNetClientHttp()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(NetClientHTTPLock);

	switch (netBaseNetType)
	{
		case NetBaseNetType_HttpClient_None_reader:
			 ABL_MediaServerPort.nClientNoneReader = 0;
			break;
		case NetBaseNetType_HttpClient_Not_found:
			 ABL_MediaServerPort.nClientNotFound = 0;
			break;
		case NetBaseNetType_HttpClient_Record_mp4:
			 ABL_MediaServerPort.nClientRecordMp4 = 0;
			break;
		case NetBaseNetType_HttpClient_Record_Progress:
			ABL_MediaServerPort.nClientRecordProgress = 0;
			break;
		case NetBaseNetType_HttpClient_on_stream_arrive:
			ABL_MediaServerPort.nClientArrive = 0;
			break;
		case NetBaseNetType_HttpClient_on_stream_not_arrive:
			ABL_MediaServerPort.nClientNotArrive = 0;
			break;
		case NetBaseNetType_HttpClient_on_stream_disconnect:
			ABL_MediaServerPort.nClientDisconnect = 0;
			break;
		case NetBaseNetType_HttpClient_on_record_ts:
			ABL_MediaServerPort.nClientRecordTS = 0;
			break;
		case NetBaseNetType_HttpClient_ServerStarted:
			ABL_MediaServerPort.nServerStarted = 0;
			break;
		case NetBaseNetType_HttpClient_ServerKeepalive:
			ABL_MediaServerPort.nServerKeepalive = 0;
			break;
		case NetBaseNetType_HttpClient_DeleteRecordMp4:
			ABL_MediaServerPort.nClientDeleteRecordMp4 = 0;
			break;
		case NetBaseNetType_HttpClient_on_play:
			ABL_MediaServerPort.nPlay = 0;
			break;
		case NetBaseNetType_HttpClient_on_publish:
			ABL_MediaServerPort.nPublish = 0;
			break;
		case NetBaseNetType_HttpClient_on_iframe_arrive:
			ABL_MediaServerPort.nFrameArrive = 0;
			break;
	}

	m_videoFifo.FreeFifo();

	WriteLog(Log_Debug, "CNetClientHttp 析构 = %X nClient = %llu ,nMediaClient = %llu", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetClientHttp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag)
		return -1;

	m_videoFifo.push(pVideoData, nDataLength);

	MessageSendThreadPool->InsertIntoTask(nClient);

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
	if (!bRunFlag)
		return -1;
	nRecvDataTimerBySecond = 0;
	std::lock_guard<std::mutex> lock(NetClientHTTPLock);

	if (MaxNetClientHttpBuffer - nNetEnd >= nDataLength)
	{//剩余空间足够
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//剩余空间不够，需要把剩余的buffer往前移动
		if (netDataCacheLength > 0)
		{//如果有少量剩余
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;

			//把空余的buffer清空 
			memset(netDataCache + nNetEnd, 0x00, MaxNetClientHttpBuffer - nNetEnd);

			if (MaxNetClientHttpBuffer - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				memset(netDataCache, 0x00, MaxNetClientHttpBuffer);
				WriteLog(Log_Debug, "CNetClientHttp = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return 0;
			}
		}
		else
		{//没有剩余，那么 首，尾指针都要复位 
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
	if (!bRunFlag)
		return -1;

	netDataCache[netDataCacheLength] = 0x00;
	nNetStart = nNetEnd = netDataCacheLength = 0;

	unsigned char* pData = NULL;
	int            nLength = 0;

	if ((pData = m_videoFifo.pop(&nLength)) != NULL && strlen(szResponseURL) > 0 )
	{
		memset(szResponseData, 0x00, sizeof(szResponseData));
		memcpy(szResponseData, pData, nLength);

		if (netBaseNetType == NetBaseNetType_HttpClient_None_reader)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else if (netBaseNetType == NetBaseNetType_HttpClient_Not_found)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else if (netBaseNetType == NetBaseNetType_HttpClient_Record_mp4)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else if (netBaseNetType == NetBaseNetType_HttpClient_Record_Progress)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else if (netBaseNetType == NetBaseNetType_HttpClient_on_stream_arrive)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else if (netBaseNetType == NetBaseNetType_HttpClient_on_stream_not_arrive)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else if (netBaseNetType == NetBaseNetType_HttpClient_on_stream_disconnect)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else if (netBaseNetType == NetBaseNetType_HttpClient_on_record_ts)
			HttpRequest(szResponseURL, szResponseData, nLength);
		else
			HttpRequest(szResponseURL, (char*)pData, nLength);

		m_videoFifo.pop_front();
	}

	return 0;
}

//发送第一个请求
int CNetClientHttp::SendFirstRequst()
{
	if (!bRunFlag)
		return -1;
	switch (netBaseNetType)
	{
	case NetBaseNetType_HttpClient_None_reader:
		ABL_MediaServerPort.nClientNoneReader = nClient;
		break;
	case NetBaseNetType_HttpClient_Not_found:
		ABL_MediaServerPort.nClientNotFound = nClient;
		break;
	case NetBaseNetType_HttpClient_Record_mp4:
		ABL_MediaServerPort.nClientRecordMp4 = nClient;
		break;
	case NetBaseNetType_HttpClient_Record_Progress:
		ABL_MediaServerPort.nClientRecordProgress = nClient;
		break;
	case NetBaseNetType_HttpClient_on_stream_arrive:
		ABL_MediaServerPort.nClientArrive = nClient;
		break;
	case NetBaseNetType_HttpClient_on_stream_not_arrive:
		ABL_MediaServerPort.nClientNotArrive = nClient;
		break;
	case NetBaseNetType_HttpClient_on_stream_disconnect:
		ABL_MediaServerPort.nClientDisconnect = nClient;
		break;
	case NetBaseNetType_HttpClient_on_record_ts:
		ABL_MediaServerPort.nClientRecordTS = nClient;
		break;
 	case NetBaseNetType_HttpClient_ServerStarted:
		ABL_MediaServerPort.nServerStarted = nClient;
		break;
	case NetBaseNetType_HttpClient_ServerKeepalive:
		ABL_MediaServerPort.nServerKeepalive = nClient;
		break;
	case NetBaseNetType_HttpClient_DeleteRecordMp4:
		ABL_MediaServerPort.nClientDeleteRecordMp4 = nClient;
		break;
	case NetBaseNetType_HttpClient_on_play:
		ABL_MediaServerPort.nPlay = nClient;
		break;
	case NetBaseNetType_HttpClient_on_publish:
		ABL_MediaServerPort.nPublish = nClient;
		break;
	case NetBaseNetType_HttpClient_on_iframe_arrive:
		ABL_MediaServerPort.nFrameArrive = nClient;
		break;
	}

	PushVideo((unsigned char*)msgNotice.szMsg, strlen(msgNotice.szMsg), "JSON");

	return 0;
}

//请求m3u8文件
bool  CNetClientHttp::RequestM3u8File()
{
	return true;
}

void CNetClientHttp::HttpRequest(char* szUrl, char* szBody,int nLength)
{
	if (!bConnectSuccessFlag || strlen(szBody) == 0 || nLength <= 0 || !bRunFlag)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
 		return;
	}

 	memset(szResponseHttpHead, 0X00, sizeof(szResponseHttpHead));
 	sprintf(szResponseHttpHead, "POST %s HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-CN,zh;q=0.8\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: application/json\r\nHost: 127.0.0.1\r\nTools: %s\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36\r\n\r\n",szUrl, nLength, MediaServerVerson);
	memcpy(szResponseHttpHead+strlen(szResponseHttpHead), szBody,nLength);
	szBody[nLength] = 0x00;

	int nRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), 1); 
	if (nRet != 0)
	{
		WriteLog(Log_Debug, "CNetClientHttp = %X nClient = %llu ,netBaseNetType = %d HttpRequest() 通知消息发送失败 ", this, nClient, netBaseNetType);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));

		return;
	}

	WriteLog(Log_Debug, "CNetClientHttp = %X nClient = %llu ,netBaseNetType = %d \r\nURL = %s \r\nbody = %s ", this, nClient, netBaseNetType,szUrl,szBody);

	pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
}
