/*
功能：
       实现ws-flv服务器的媒体数据发送功能 
日期    2021-11-05
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerWS_FLV.h"
#ifdef USE_BOOST
extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);

extern CMediaFifo                      pDisconnectBaseNetFifo; //清理断裂的链接 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                 ABL_MediaServerPort;
extern bool                            AddClientToMapAddMutePacketList(uint64_t nClient);
extern bool                            DelClientToMapFromMutePacketList(uint64_t nClient);
#else
extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);

extern CMediaFifo                      pDisconnectBaseNetFifo; //清理断裂的链接 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                 ABL_MediaServerPort;
extern bool                            AddClientToMapAddMutePacketList(uint64_t nClient);
extern bool                            DelClientToMapFromMutePacketList(uint64_t nClient);


#endif //USE_BOOST
//FLV合成回调函数 
static int NetServerWS_FLV_MuxerCB(void* flv, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetServerWS_FLV* pHttpFLV = (CNetServerWS_FLV*)flv;

	if (!pHttpFLV->bRunFlag)
		return -1;

#ifdef WriteHttp_FlvFileFlag
	if (pHttpFLV)
 		return flv_writer_input(pHttpFLV->flvWrite, type, data, bytes, timestamp);
#else 
	if (pHttpFLV)
		return flv_writer_input(pHttpFLV->flvWrite, type, data, bytes, timestamp);
#endif
}

int  NetServerWS_FLV_OnWrite_CB(void* param, const struct flv_vec_t* vec, int n)
{
	CNetServerWS_FLV* pHttpFLV = (CNetServerWS_FLV*)param;

	if (pHttpFLV != NULL && pHttpFLV->bRunFlag )
	{
		for (int i = 0; i < n; i++)
		{
			pHttpFLV->nWriteRet = pHttpFLV->WSSendFlvData((unsigned char*)vec[i].ptr, vec[i].len);// XHNetSDK_Write(pHttpFLV->nClient, (unsigned char*)vec[i].ptr, vec[i].len, true);
			if (pHttpFLV->nWriteRet != 0)
			{
				pHttpFLV->nWriteErrorCount ++;//发送出错累计 
				if (pHttpFLV->nWriteErrorCount >= 30)
				{
					pHttpFLV->bRunFlag = false;
					WriteLog(Log_Debug, "NetServerWS_FLV_OnWrite_CB 发送失败，次数 nWriteErrorCount = %d ", pHttpFLV->nWriteErrorCount);
					DeleteNetRevcBaseClient(pHttpFLV->nClient);
				}
			}
			else
				pHttpFLV->nWriteErrorCount = 0;//复位 
  		}
	}
	return 0;
}

CNetServerWS_FLV::CNetServerWS_FLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	bCheckHttpFlvFlag = false;
	strcpy(m_szShareMediaURL, szShareMediaURL);

	nWebSocketCommStatus = WebSocketCommStatus_Connect;

	MaxNetDataCacheCount = MaxHttp_WsFlvNetCacheBufferLength;
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//网络数据缓存大小
	bFindFlvNameFlag = false;
	memset(szFlvName, 0x00, sizeof(szFlvName));
	flvMuxer = NULL;

	videoDts =  0;
	flvWrite  = NULL ;
	nWriteRet = 0;
	nWriteErrorCount = 0;

	netBaseNetType = NetBaseNetType_WsFLVServerSendPush;
	bRunFlag = true;

	memset(szSec_WebSocket_Key, 0x00, sizeof(szSec_WebSocket_Key));
	memset(szSec_WebSocket_Protocol, 0x00, sizeof(szSec_WebSocket_Protocol));
	strcpy(szSec_WebSocket_Protocol, "Protocol1");
 
	WriteLog(Log_Debug, "CNetServerWS_FLV 构造 = %X,  nClient = %llu ",this, nClient);
}

CNetServerWS_FLV::~CNetServerWS_FLV()
{
	std::lock_guard<std::mutex> lock(NetServerWS_FLVLock);

	WriteLog(Log_Debug, "CNetServerWS_FLV =%X Step 1 nClient = %llu ",this, nClient);

	WriteLog(Log_Debug, "CNetServerWS_FLV =%X Step 2 nClient = %llu ",this, nClient);
	
	if (flvMuxer)
	{
		flv_muxer_destroy(flvMuxer);
		flvMuxer = NULL;
	}
	WriteLog(Log_Debug, "CNetServerWS_FLV =%X Step 3 nClient = %llu ",this, nClient);

	if (flvWrite)
	{
		flv_writer_destroy(flvWrite);
		flvWrite = NULL;
	}
	WriteLog(Log_Debug, "CNetServerWS_FLV =%X Step 4 nClient = %llu ",this, nClient);

	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
	
	//从静音链表删除 
	if (bAddMuteFlag)
	  DelClientToMapFromMutePacketList(nClient);
	
	WriteLog(Log_Debug, "CNetServerWS_FLV 析构 =%X szFlvName = %s, nClient = %llu \r\n", this, szFlvName, nClient);
	malloc_trim(0);
}

int CNetServerWS_FLV::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0;

	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	m_videoFifo.push(pVideoData, nDataLength);
	return 0 ;
}

int CNetServerWS_FLV::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;
	if (ABL_MediaServerPort.nEnableAudio == 0)
		return -1;

	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}
	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

void  CNetServerWS_FLV::MuxerVideoFlV(char* codeName, unsigned char* pVideo, int nLength)
{
	//只有视频，或者屏蔽音频
	if(ABL_MediaServerPort.nEnableAudio == 0 )
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	if (strcmp(codeName, "H264") == 0)
	{
		if (flvMuxer)
			flv_muxer_avc(flvMuxer, pVideo, nLength, videoDts, videoDts);
	}
	else if (strcmp(codeName, "H265") == 0)
	{
		if (flvMuxer)
			flv_muxer_hevc(flvMuxer, pVideo, nLength, videoDts, videoDts);
	}

	//printf("flvPS = %d \r\n", videoDts);
	videoDts += nVideoStampAdd;
}

void  CNetServerWS_FLV::MuxerAudioFlV(char* codeName, unsigned char* pAudio, int nLength)
{
	if (flvMuxer)
	{
		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			flv_muxer_aac(flvMuxer, pAudio, nLength, audioDts, audioDts);
		else if (strcmp(mediaCodecInfo.szAudioName, "MP3") == 0)
			flv_muxer_mp3(flvMuxer, pAudio, nLength, audioDts, audioDts);
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
		{
			flv_muxer_g711a(flvMuxer, pAudio, nLength, audioDts, audioDts);
			audioDts += nLength / 8;
		}
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		{
			flv_muxer_g711u(flvMuxer, pAudio, nLength, audioDts, audioDts);
			audioDts += nLength / 8;
		}
	}
 
	//同步音视频 
	SyncVideoAudioTimestamp();
}

int CNetServerWS_FLV::SendVideo()
{
	std::lock_guard<std::mutex> lock(NetServerWS_FLVLock);
	
	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ",nClient);
		DeleteNetRevcBaseClient(nClient);
 		return -1;
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if((pData = m_videoFifo.pop(&nLength)) != NULL )
	{
		if (nMediaSourceType == MediaSourceType_LiveMedia)
			MuxerVideoFlV(mediaCodecInfo.szVideoName, pData, nLength);
		else
			MuxerVideoFlV(mediaCodecInfo.szVideoName, pData + 4, nLength - 4);

		m_videoFifo.pop_front();
	}

	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ", nClient);
		DeleteNetRevcBaseClient(nClient);
	}

	return 0;
}

int CNetServerWS_FLV::SendAudio()
{
	std::lock_guard<std::mutex> lock(NetServerWS_FLVLock);
	
	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ", nClient);
		DeleteNetRevcBaseClient(nClient);
		return -1;
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (nMediaSourceType == MediaSourceType_LiveMedia)
			MuxerAudioFlV(mediaCodecInfo.szAudioName, pData, nLength);
		else
			MuxerAudioFlV(mediaCodecInfo.szAudioName, pData+4, nLength - 4);

		m_audioFifo.pop_front();
  	}
	if (nWriteErrorCount >= 30)
	{
		DeleteNetRevcBaseClient(nClient);
		WriteLog(Log_Debug, "发送flv 失败,nClient = %llu ", nClient);
	}

	return 0;
}

int CNetServerWS_FLV::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
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

			if (MaxNetDataCacheCount - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetServerWS_FLV = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
				DeleteNetRevcBaseClient(nClient);
				return 0;
			}
 		}
		else
		{//没有剩余，那么 首，尾指针都要复位 
			nNetStart = 0;
			nNetEnd = 0;
			netDataCacheLength = 0;
 		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}

	WriteLog(Log_Debug, "InputNetData() ... ");

	return true;
}

int CNetServerWS_FLV::ProcessNetData()
{
	int nRet = 5;
	char maskingKey[4] = { 0 };
	char payloadData[4096] = { 0 };
	unsigned short nLength;
	unsigned char  nCommand = 0x00 ;
	unsigned char szPong[4] = { 0x8A,0x80,0x00,0x00 };

	if (netDataCacheLength > string_length_4096)
	{
		WriteLog(Log_Debug, "CNetServerWS_FLV = %X , nClient = %llu ,netDataCacheLength = %d, 发送过来的url数据长度非法 ,立即删除 ", this, nClient, netDataCacheLength);
		DeleteNetRevcBaseClient(nClient);
		return -1;
	}

	if (nWebSocketCommStatus == WebSocketCommStatus_Connect)
	{//WebSocket协议握手，检查
		Create_WS_FLV_Handle() ;
	}
	else if (nWebSocketCommStatus == WebSocketCommStatus_ShakeHands)
	{//把数据，通过掩码进行解码 

		nCommand = 0x0F & netDataCache[0];
		if (nCommand == 0x08)
		{//关闭命令
			WriteLog(Log_Debug, "CNetServerWS_FLV = %X ,nClient = %llu, 收到 websocket 关闭命令 ",this,nClient);
			DeleteNetRevcBaseClient(nClient);
		}
		else if (nCommand == 0x09)
		{//客户端发送 ping 命令，需要回放 0x0A  {0x8A, 0x80} 
			WriteLog(Log_Debug, "CNetServerWS_FLV = %X ,nClient = %llu, 收到 websocket 心跳 ",this,nClient);
			XHNetSDK_Write(nClient, szPong, 2, true);
		}

		nNetStart = nNetEnd = netDataCacheLength = 0;
 
#if  0
		//接收 websocke 数据，并且解包 ，通过掩码解包
		memcpy((char*)&nLength, netDataCache + 2, 2);
		nLength = ntohs(nLength);
		unsigned char bLength = netDataCache[1];
		bLength = bLength & 0x7F;
		memcpy(maskingKey, netDataCache + 2 + 2 , 4);
		memcpy(payloadData, netDataCache + 2 + 2 + 4, nLength);
		for (int i = 0; i < nLength; i++)
		{
			payloadData[i] = payloadData[i] ^ maskingKey[i % 4];
		}

		unsigned char szWebSocketHead[4] = { 0 };
		szWebSocketHead[0] = 0x81;
		szWebSocketHead[1] = 0x7E;
		unsigned short nTrueLength = nLength;
		nLength = htons(nLength);
		memcpy(szWebSocketHead + 2, (unsigned char*)&nLength, sizeof(nLength));
		XHNetSDK_Write(nClient, szWebSocketHead, 4, true);
		XHNetSDK_Write(nClient, (unsigned char*)payloadData,nTrueLength, true);
#endif
	}

	return 0;
}

bool  CNetServerWS_FLV::Create_WS_FLV_Handle()
{
	if (!bFindFlvNameFlag)
	{
		if (strstr((char*)netDataCache, "\r\n\r\n") == NULL)
		{
			WriteLog(Log_Debug, "数据尚未接收完整 ");
			if (memcmp(netDataCache, "GET ", 4) != 0)
			{
				WriteLog(Log_Debug, "CNetServerWS_FLV = %X , nClient = %llu , 接收的数据非法 ", this, nClient);
				DeleteNetRevcBaseClient(nClient);
			}
			return -1;
		}
	}

	if (!bCheckHttpFlvFlag)
	{
		bCheckHttpFlvFlag = true;

		//把请求的FLV文件读取出来　
		char    szTempName[string_length_2048] = { 0 };
		string  strHttpHead = (char*)netDataCache;
		int     nPos1, nPos2;
		nPos1 = strHttpHead.find("GET ", 0);
		if (nPos1 >= 0 && nPos1 != string::npos)
		{
			nPos2 = strHttpHead.find(" HTTP/", 0);
			if (nPos2 > 0 && nPos2 != string::npos)
			{
				if ((nPos2 - nPos1 - 4) > string_length_2048)
				{
					WriteLog(Log_Debug, "CNetServerWS_FLV = %X,请求文件名称长度非法 nClient = %llu ", this, nClient);
					DeleteNetRevcBaseClient(nClient);
					return -1;
				}

				bFindFlvNameFlag = true;
				memset(szTempName, 0x00, sizeof(szTempName));
				memcpy(szTempName, netDataCache + nPos1 + 4, nPos2 - nPos1 - 4);

				string strFlvName = szTempName;
				nPos2 = strFlvName.find("?", 0);
				if (nPos2 > 0)
				{//有？，需要去掉？后面的字符串 
					if (strlen(szPlayParams) == 0)//拷贝鉴权参数
						memcpy(szPlayParams, szTempName + (nPos2 + 1), strlen(szTempName) - nPos2 - 1);
					memset(szFlvName, 0x00, sizeof(szFlvName));
					memcpy(szFlvName, szTempName, nPos2);
				}
				else//没有？，直接拷贝 
					strcpy(szFlvName, szTempName);

				if(strlen(szFlvName) < 512)
				  WriteLog(Log_Debug, "CNetServerWS_FLV=%X ,nClient = %llu ,拷贝出FLV 文件名字 %s ", this, nClient, szFlvName);
			}
		}

		if (!bFindFlvNameFlag)
		{
			WriteLog(Log_Debug, "CNetServerWS_FLV=%X, 检测出 非法的 Http-flv 协议数据包 nClient = %llu ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		//根据FLV文件，进行简单判断是否合法
		if (!(strstr(szFlvName, ".flv") != NULL || strstr(szFlvName, ".FLV") != NULL))
		{
			WriteLog(Log_Debug, "CNetServerWS_FLV = %X,  nClient = %llu ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		//根据FLV文件，进行查找推流对象 
		if (strstr(szFlvName, ".flv") != NULL || strstr(szFlvName, ".FLV") != NULL)
			szFlvName[strlen(szFlvName) - 4] = 0x00;

		strcpy(szMediaSourceURL, szFlvName);
#ifdef USE_BOOST
		boost::shared_ptr<CMediaStreamSource> pushClient = NULL;
#else
		std::shared_ptr<CMediaStreamSource> pushClient = NULL;
#endif

		if (strstr(szFlvName, RecordFileReplaySplitter) == NULL)
		{//实况点播
			pushClient = GetMediaStreamSource(szFlvName, true);
			if (pushClient == NULL)
			{
				WriteLog(Log_Debug, "CNetServerWS_FLV=%X, 没有推流对象的地址 %s nClient = %llu ", this, szFlvName, nClient);

				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

				DeleteNetRevcBaseClient(nClient);
				return -1;
			}
		}
		else
		{//录像点播
		    //查询点播的录像是否存在
			if (QueryRecordFileIsExiting(szFlvName) == false)
			{
				WriteLog(Log_Debug, "CNetServerWS_FLV = %X, 没有点播的录像文件 %s nClient = %llu ", this, szFlvName, nClient);
				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
				DeleteNetRevcBaseClient(nClient);
				return -1;
			}

			//创建录像文件点播
			pushClient = CreateReplayClient(szFlvName, &nReplayClient);
			if (pushClient == NULL)
			{
				WriteLog(Log_Debug, "CNetServerWS_FLV=%X, 建录像文件点播失败 %s nClient = %llu ", this, szFlvName, nClient);
				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
				DeleteNetRevcBaseClient(nClient);
				return -1;
			}
		}

		//记下媒体源
		SplitterAppStream(szFlvName);
		sprintf(m_addStreamProxyStruct.url, "ws://%s:%d/%s/%s.flv", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);

		wsParse.ParseSipString((char*)netDataCache);
		char szWebSocket[64] = { 0 };
		char szConnect[64] = { 0 };
		char szOrigin[256] = { 0 };
		wsParse.GetFieldValue("Connection", szConnect);
		wsParse.GetFieldValue("Upgrade", szWebSocket);
		wsParse.GetFieldValue("Sec-WebSocket-Key", szSec_WebSocket_Key);
		wsParse.GetFieldValue("Sec_WebSocket_Protocol", szSec_WebSocket_Protocol);
		
		wsParse.GetFieldValue("Origin", szOrigin);

		if (strcmp(szConnect, "Upgrade") != 0 || strcmp(szWebSocket, "websocket") != 0 || strlen(szSec_WebSocket_Key) == 0)
		{
			WriteLog(Log_Debug, "CNetServerWS_FLV = %X , nClient = %llu , 不是websocket 协议通讯，立即删除 ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
		}
		nWebSocketCommStatus = WebSocketCommStatus_ShakeHands;

		nNetStart = nNetEnd = netDataCacheLength = 0;
		memset(netDataCache, 0x00, MaxHttp_WsFlvNetCacheBufferLength);

		char szResponseClientKey[256] = { 0 };
		char szSHA1Buffer[256] = { 0 };
		strcat(szSec_WebSocket_Key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
		strcpy(szSHA1Buffer, SHA1::encode_bin(szSec_WebSocket_Key).c_str());
		base64_encode(szResponseClientKey, szSHA1Buffer, strlen(szSHA1Buffer));

		memset(szWebSocketResponse, 0x00, sizeof(szWebSocketResponse));
		sprintf(szWebSocketResponse, "HTTP/1.1 101 Switching Protocol\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: Upgrade\r\nDate: Mon, Nov 08 2021 01:52:45 GMT\r\nKeep-Alive: timeout=30, max=100\r\nSec-WebSocket-Accept: %s\r\nServer: %s\r\nUpgrade: websocket\r\nSec_WebSocket_Protocol: %s\r\n\r\n", szOrigin, szResponseClientKey, MediaServerVerson, szSec_WebSocket_Protocol);
		XHNetSDK_Write(nClient, (unsigned char*)szWebSocketResponse, strlen(szWebSocketResponse), true);
		if (nWriteRet != 0)
		{
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		flvMuxer = flv_muxer_create(NetServerWS_FLV_MuxerCB, this);
		if (flvMuxer == NULL)
		{
			WriteLog(Log_Debug, "创建 flv 打包器失败 ");
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

#ifdef WriteHttp_FlvFileFlag //写入FLV文件
		char  szWriteFlvName[256] = { 0 };
		sprintf(szWriteFlvName, ".\\%X_%llu.flv", this, nClient);
		flvWrite = flv_writer_create(szWriteFlvName);
#else //通过网络传输 
		if ((strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H264") == 0 || strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H265") == 0) &&
			strlen(pushClient->m_mediaCodecInfo.szAudioName) > 0 && ABL_MediaServerPort.nEnableAudio == 1)
		{//H264、H265  && AAC，创建音频，视频
			flvWrite = flv_writer_create2(1, 1, NetServerWS_FLV_OnWrite_CB, (void*)this);
			WriteLog(Log_Debug, "创建ws-flv 输出格式为： 视频 %s、音频 %s  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, pushClient->m_mediaCodecInfo.szAudioName, nClient);
		}
		else if ((strlen(pushClient->m_mediaCodecInfo.szAudioName) == 0 || ABL_MediaServerPort.nEnableAudio == 0) && ( strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H264") == 0 || strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H265") == 0))
		{//H264、H265 只创建视频
 			flvWrite = flv_writer_create2(0, 1, NetServerWS_FLV_OnWrite_CB, (void*)this);
			WriteLog(Log_Debug, "创建ws-flv  输出格式为： 视频 %s、音频：无音频  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, nClient);
  		}
		else if (strlen(pushClient->m_mediaCodecInfo.szVideoName) == 0 && strlen(pushClient->m_mediaCodecInfo.szAudioName) > 0 )
		{//只创建音频
			flvWrite = flv_writer_create2(1, 0, NetServerWS_FLV_OnWrite_CB, (void*)this);
			WriteLog(Log_Debug, "创建ws-flv 输出格式为： 无视频 、只有音频 %s  nClient = %llu ", pushClient->m_mediaCodecInfo.szAudioName, nClient);
		}
		else
		{
			WriteLog(Log_Debug, "视频 %s、音频 %s 格式有误，不支持ws-flv 输出,即将删除 nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, pushClient->m_mediaCodecInfo.szAudioName, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		if (flvWrite == NULL)
		{
			WriteLog(Log_Debug, "创建 ws-flv 发送组件失败 ");
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
#endif
		nWebSocketCommStatus = WebSocketCommStatus_ShakeHands;

		m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
		m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

		//把客户端 加入源流媒体拷贝队列 
		pushClient->AddClientToMap(nClient);

	}
}

//发送第一个请求
int CNetServerWS_FLV::SendFirstRequst()
{
	return 0;
}

//请求m3u8文件
bool  CNetServerWS_FLV::RequestM3u8File()
{
	return true;
}

//flv数据前面增加 websocket头 
int  CNetServerWS_FLV::WSSendFlvData(unsigned char* pData, int nDataLength)
{
	if (nDataLength >= 0 && nDataLength <= 125)
	{
		memset(webSocketHead, 0x00, sizeof(webSocketHead));
		webSocketHead[0] = 0x82;
		webSocketHead[1] = nDataLength;
		XHNetSDK_Write(nClient, webSocketHead, 2, true);
		XHNetSDK_Write(nClient, pData, nDataLength, true);
	}
	else if (nDataLength >= 126 && nDataLength <= 0xFFFF)
	{
		memset(webSocketHead, 0x00, sizeof(webSocketHead));
		webSocketHead[0] = 0x82;
		webSocketHead[1] = 0x7E;

		wsLength16 = nDataLength;
		wsLength16 = htons(wsLength16);
		memcpy(webSocketHead + 2, (unsigned char*)&wsLength16, sizeof(wsLength16));
		XHNetSDK_Write(nClient, webSocketHead, 4, true);
		XHNetSDK_Write(nClient, pData, nDataLength, true);
	}
	else if (nDataLength > 0xFFFF)
	{
		memset(webSocketHead, 0x00, sizeof(webSocketHead));
		webSocketHead[0] = 0x82;
		webSocketHead[1] = 0x7F;

		wsLenght64 = nDataLength;
		wsLenght64 = htonl(wsLenght64);
		memcpy(webSocketHead + 2+4, (unsigned char*)&wsLenght64, sizeof(wsLenght64));
		XHNetSDK_Write(nClient, webSocketHead, 10, true);
		XHNetSDK_Write(nClient, pData, nDataLength, true);
	}
	else
		return -1;

	return 0;
}