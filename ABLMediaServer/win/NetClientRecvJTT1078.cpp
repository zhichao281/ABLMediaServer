/*
功能：
    实现交通部http码流接收 
	 
日期    2021-03-30
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientRecvJTT1078.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaSendThreadPool*                 pMediaSendThreadPool;
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern CMediaFifo                            pRemoveBaseNetFromThreadFifo;       //从媒体拷贝线程、媒体发送线程移除掉Client  
extern int                                   SampleRateArray[] ;
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径

extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result);

extern void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);


CNetClientRecvJTT1078::CNetClientRecvJTT1078(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	bCheckRtspVersionFlag = false;
	bDeleteRtmpPushH265Flag = false;
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	strcpy(m_szShareMediaURL, szShareMediaURL);

	pMediaSource = NULL;
	nWriteRet = 0;
	nWriteErrorCount = 0;

	if (ParseRtspRtmpHttpURL(szIP) == true)
		uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, 5000, 1);

	nVideoDTS = 0;
	nAudioDTS = 0;
	memset(szRtmpName, 0x00, sizeof(szRtmpName));
	netDataCacheLength = nNetStart = nNetEnd = 0;
	netBaseNetType = NetBaseNetType_HttpFlvClientRecv;

	tPutVideoTime = GetTickCount64();
	m_videoFifo.InitFifo(MaxNetDataCacheBufferLength);

#ifdef  SaveNetDataToJTT1078File
 		fileJTT1078 = fopen("d:\\jtt1078.data","wb");
#endif
	WriteLog(Log_Debug, "CNetClientRecvJTT1078 构造 = %X nClient = %llu ", this, nClient);
}

CNetClientRecvJTT1078::~CNetClientRecvJTT1078()
{
	std::lock_guard<std::mutex> lock(NetClientRecvFLVLock);

	pRemoveBaseNetFromThreadFifo.push((unsigned char*)&nClient, sizeof(nClient)); //从媒体拷贝线程、媒体发送线程移除掉Client  

	m_videoFifo.FreeFifo();

	XHNetSDK_Disconnect(nClient);

	 //如果是接收推流，并且成功接收推流的，则需要删除媒体数据源 szURL ，比如 /Media/Camera_00001 
   	 DeleteMediaStreamSource(m_szShareMediaURL);

#ifdef  SaveNetDataToJTT1078File
	if (fileJTT1078 != NULL)
		fclose(fileJTT1078);
#endif

#ifdef  WriteHTTPFlvToEsFileFlag
	 fclose(fWriteVideo); 
#endif

	WriteLog(Log_Debug, "CNetClientRecvJTT1078 析构 = %X nClient = %llu \r\n", this, nClient);
	
	malloc_trim(0);
}

int CNetClientRecvJTT1078::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetClientRecvJTT1078::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}
int CNetClientRecvJTT1078::SendVideo()
{
	return 0;
}

int CNetClientRecvJTT1078::SendAudio()
{

	return 0;
}

int CNetClientRecvJTT1078::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(NetClientRecvFLVLock);

	nRecvDataTimerBySecond = 0;

	_m3u8.append((char*)pData, nDataLength);
	bool isIdr = false;
	//解析1078音视频数据
	Head1708 head;
	while (true)
	{
		const char cDest[] = { 0x30, 0x31,0x63,0x64,0x00 };
		const int HEADLEN = sizeof(Head1708);// 30;
		const int LENPOS = 28;
		size_t posFind = _m3u8.find(std::string(cDest, strlen(cDest)));
		if (posFind == std::string::npos)
		{
			_m3u8.clear();
			return -1;
		}
		if (posFind + HEADLEN >= _m3u8.length())
		{
			return -1;
		}

		memcpy(&head, _m3u8.c_str() + posFind, HEADLEN);
		short blockLen = head.m_length;
		unsigned char ucTm0 = *((unsigned char*)(&blockLen));
		unsigned char ucTm1 = *((unsigned char*)(&blockLen) + 1);
		*((unsigned char*)(&blockLen)) = ucTm1;
		*((unsigned char*)(&blockLen) + 1) = ucTm0;
		if (posFind + HEADLEN + blockLen >= _m3u8.length())
		{
			return -1;
		}
		m_es += _m3u8.substr(posFind + HEADLEN, blockLen);
		_m3u8.erase(0, posFind + HEADLEN + blockLen);
		if ((head.m_type & 0x0f) == 0x00 || (head.m_type & 0x0f) == 0x02)
		{
			//I帧
			if (head.m_type & 0xf0 == 00)
			{
				isIdr = true;
			}
			break;
		}
	}

#ifdef  SaveNetDataToJTT1078File
	if (fileJTT1078 != NULL)
	{
		fwrite(m_es.c_str(), 1, m_es.length(), fileJTT1078);
		fflush(fileJTT1078);
	}
#endif

	m_videoFifo.push((unsigned char*)m_es.c_str(), m_es.length());
	m_es.clear();
	_m3u8.clear();

	unsigned char* pVideo;
	int            nLength;
	if (GetTickCount64() - tPutVideoTime >= 40)
	{
		tPutVideoTime = GetTickCount64();
		pVideo = m_videoFifo.pop(&nLength);
		if (pVideo != NULL && nLength > 0)
		{
			pMediaSource->PushVideo(pVideo, nLength, "H264");

			m_videoFifo.pop_front();
		}
	}

	//防止积累视频过多
	while (m_videoFifo.GetSize() > 5)
	{
		pVideo = m_videoFifo.pop(&nLength);
		if (pVideo != NULL && nLength > 0)
		{
			pMediaSource->PushVideo(pVideo, nLength, "H264");

			m_videoFifo.pop_front();
		}
	}
 
    return 0;
}

int CNetClientRecvJTT1078::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(NetClientRecvFLVLock);
 

	return 0;
}

//发送第一个请求
int CNetClientRecvJTT1078::SendFirstRequst()
{
	string  strHttpFlvURL = m_rtspStruct.szSrcRtspPullUrl;
	int nPos1, nPos2;
	char    szSubPath[256] = { 0 };
	char    szSubPathUTF_8[256] = { 0 };

	sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%d}", hParent);
	ResponseHttp(nClient_http, szResponseBody, false);

	nPos1 = strHttpFlvURL.find("//", 0);
	if (nPos1 > 0)
	{
		nPos2 = strHttpFlvURL.find("/", nPos1 + 2);
		if (nPos2 > 0)
		{
			//flvDemuxer = flv_demuxer_create(NetClientRecvFLVCallBack, this);

			//创建媒体分发资源
			if (strlen(m_szShareMediaURL) > 0)
			{
				pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hParent, MediaSourceType_LiveMedia,0);
				if (pMediaSource)
					pMediaSource->enable_mp4 = (strcmp(m_addStreamProxyStruct.enable_mp4, "1") == 0) ? true : false;
  			}

			memcpy(szSubPath, m_rtspStruct.szSrcRtspPullUrl + nPos2, strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos2);
			//GBK2UTF8(szSubPath, szSubPathUTF_8, sizeof(szSubPathUTF_8));
			sprintf(szRequestFLVFile, "GET %s HTTP/1.1\r\nUser-Agent: %s\r\nAccept: */*\r\nUpgrade-Insecure-Requests: 1\r\nConnection: keep-alive\r\nHost: 119.136.25.35:6604\r\Accept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n\r\n", szSubPath, MediaServerVerson);
			XHNetSDK_Write(nClient, (unsigned char*)szRequestFLVFile, strlen(szRequestFLVFile), 1);
		}else
			pDisconnectBaseNetFifo.push((unsigned char*)nClient, sizeof(nClient));
	}else
		pDisconnectBaseNetFifo.push((unsigned char*)nClient, sizeof(nClient));

#ifdef  SaveNetDataToFlvFile
	sprintf(szRequestFLVFile, "%s%X.flv", ABL_MediaSeverRunPath, this);
	fileFLV = fopen(szRequestFLVFile, "wb"); ;
#endif

#ifdef  WriteHTTPFlvToEsFileFlag
	bStartWriteFlag = false;
	sprintf(szRequestFLVFile, "%s%X.264", ABL_MediaSeverRunPath, this);
	fWriteVideo = fopen(szRequestFLVFile, "wb"); ;
#endif

	return 0;
}

//请求m3u8文件
bool  CNetClientRecvJTT1078::RequestM3u8File()
{
	return true;
}