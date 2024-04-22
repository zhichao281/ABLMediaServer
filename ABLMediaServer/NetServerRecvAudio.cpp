/*
功能：
       实现websocket服务器音频数据接收 
日期    2023-12-02
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerRecvAudio.h"
#ifdef USE_BOOST
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#else
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#endif

extern bool                                  DeleteMediaStreamSource(char* szURL);
extern             bool                      DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern             char                      ABL_MediaSeverRunPath[256] ; //当前路径

CNetServerRecvAudio::CNetServerRecvAudio(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	memset(m_szShareMediaURL, 0x00, sizeof(m_szShareMediaURL));
	pMediaSouce = NULL;
	SplitterMaxPcmLength = SplitterMaxPcmLength_G711;
	src_linesize = 0;
	pcm_buffer = NULL;
	pcm_OutputBuffer = NULL;
	outSampleRate = 8000;
	swr_ctx = NULL;
	memset(pPcmSplitterBuffer, 0x00, MaxPcmChacheBufferLength);
	nPcmSplitterLength = 0;
	nSplitterCount = 0;
 	memset((char*)&audioRegisterStruct, 0x00, sizeof(audioRegisterStruct));
	nPcmCacheBufferLength = 0;
	memset(pPcmCacheBuffer, 0x00, MaxPcmChacheBufferLength);
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	bCheckHttpFlvFlag = false;
	m_nMaxRecvBufferLength = MaxPcmChacheBufferLength ;
	dePacket = new unsigned char[MaxPcmChacheBufferLength];
	memset(dePacket, 0x00, m_nMaxRecvBufferLength);
	packageLen = 0;
    packageHeadLen = 0;

	nWebSocketCommStatus = WebSocketCommStatus_Connect;

	netDataCache = new unsigned char[m_nMaxRecvBufferLength];
	MaxNetDataCacheCount = m_nMaxRecvBufferLength;
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//网络数据缓存大小
	bFindFlvNameFlag = false;
	memset(szFlvName, 0x00, sizeof(szFlvName));
 
	netBaseNetType = NetBaseNetType_WebSocektRecvAudio;
	bRunFlag = true;

	memset(szSec_WebSocket_Key, 0x00, sizeof(szSec_WebSocket_Key));
	memset(szSec_WebSocket_Protocol, 0x00, sizeof(szSec_WebSocket_Protocol));
	strcpy(szSec_WebSocket_Protocol, "Protocol1");

	WriteLog(Log_Debug, "CNetServerRecvAudio 构造 = %X nClient = %llu ", this, nClient);

#ifdef WritePCMDaFile
	 char szFileName[256] = { 0 };
	 nWritePCMCount  = 0 ;
	 sprintf(szFileName, "%s%X.pcm", ABL_MediaSeverRunPath, this);
     fWritePCMFile = fopen(szFileName,"wb");
#endif
}

CNetServerRecvAudio::~CNetServerRecvAudio()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(NetServerWS_FLVLock);

	XHNetSDK_Disconnect(nClient);

	if (netDataCache)
	{
		delete [] netDataCache;
		netDataCache = NULL;
	}
	if (dePacket)
	{
		delete[] dePacket;
		dePacket = NULL;
	}

	wsParse.FreeSipString();

	if (swr_ctx != NULL)
	{
	  swr_free(&swr_ctx);
	  swr_ctx = NULL;
 	}
	if (pcm_buffer)
	{
		av_freep(&pcm_buffer[0]);
	   av_freep(&pcm_buffer);
	}
	if (pcm_OutputBuffer)
	{
		av_freep(&pcm_OutputBuffer[0]);
	    av_freep(&pcm_OutputBuffer);
	}

	aacEnc.ExitAACEncodec();

	//删除媒体源
	if (strlen(m_szShareMediaURL) > 0 && pMediaSouce != NULL)
		DeleteMediaStreamSource(m_szShareMediaURL);

#ifdef WritePCMDaFile
 	fclose(fWritePCMFile);
#endif
	malloc_trim(0);
	WriteLog(Log_Debug, "CNetServerRecvAudio 析构 = %X nClient = %llu ", this, nClient);
}

int CNetServerRecvAudio::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{

 	return 0 ;
}

int CNetServerRecvAudio::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
 	return 0;
}

int CNetServerRecvAudio::SendVideo()
{

	return 0;
}

int CNetServerRecvAudio::SendAudio()
{

	return 0;
}

int CNetServerRecvAudio::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(NetServerWS_FLVLock);
	nRecvDataTimerBySecond = 0;

	if (!bRunFlag)
		return -1;

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

	return true;
}

int CNetServerRecvAudio::ProcessNetData()
{
	if (!bRunFlag)
		return -1;

	int nRet = 5;
	char maskingKey[4] = { 0 };
	char payloadData[4096] = { 0 };
	unsigned short nLength;
	unsigned char  nCommand = 0x00 ;
	unsigned char szPong[4] = { 0x8A,0x80,0x00,0x00 };

	if (nWebSocketCommStatus == WebSocketCommStatus_Connect)
	{//WebSocket协议握手，检查
		Create_WS_FLV_Handle() ;
	}
	else if (nWebSocketCommStatus == WebSocketCommStatus_ShakeHands)
	{//把数据，通过掩码进行解码 
		while (netDataCacheLength > 0)
		{
			nCommand = 0x0F & netDataCache[0];
			if (nCommand == 0x08)
			{//关闭命令
				DeleteNetRevcBaseClient(nClient);
				nNetStart = nNetEnd = netDataCacheLength = 0;
			}
			else if (nCommand == 0x09)
			{//客户端发送 ping 命令，需要回放 0x0A  {0x8A, 0x80} 
				XHNetSDK_Write(nClient, szPong, 2, true);
				nNetStart = nNetEnd = netDataCacheLength = 0;
			}
			else
			{
				memset(dePacket, 0x00, m_nMaxRecvBufferLength);
				webSocket_dePackage(netDataCache, netDataCacheLength, dePacket, m_nMaxRecvBufferLength, &packageLen, &packageHeadLen);

				if (netDataCacheLength >= packageLen + packageHeadLen)
				{//数据接收完毕
					if (netDataCacheLength - (packageLen + packageHeadLen) > 0)
					{
						int nSize = netDataCacheLength - (packageLen + packageHeadLen);
						memmove(netDataCache, netDataCache + (packageLen + packageHeadLen), nSize);
						netDataCacheLength = nNetEnd = nSize;
						nNetStart = 0;
					}
					else
					{
						nNetStart = nNetEnd = netDataCacheLength = 0;
						memset(netDataCache, 0x00, m_nMaxRecvBufferLength);
					}

					if (packageLen > 0 && packageHeadLen > 0)
					{
						if (MaxPcmChacheBufferLength - nPcmCacheBufferLength > packageLen)
						{
							memcpy(pPcmCacheBuffer + nPcmCacheBufferLength, dePacket, packageLen);
							nPcmCacheBufferLength += packageLen;

							//私有数据解析处理 
							ProcessPcmCacheBuffer();
						}
						else
							nPcmCacheBufferLength = 0;//防止出错 
					}
				}
				else
				{
					printf("数据尚未接收完毕 \r\n");
					break;
				}
			}
 		}
 	}

	return 0;
}

//私有格式的pcm解析处理
void   CNetServerRecvAudio::ProcessPcmCacheBuffer()
{
	memcpy((char*)&pcmHead, pPcmCacheBuffer, sizeof(WebSocketPCMHead));
	nPcmLength = htons(pcmHead.nLength);

	//合法性检测 
	if (!(pcmHead.head[0] == 0xab && pcmHead.head[1] == 0xcd && pcmHead.head[2] == 0xef && pcmHead.head[3] == 0xab))
	{
		WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu ,pcm 数据的包头不合法！ ", this, nClient);
		DeleteNetRevcBaseClient(nClient);
		return;
	}

	if (pcmHead.nType == 0x01)
	{
		WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu ,接收到音频注册包 ", this, nClient);
		rapidjson::Document doc;
		doc.Parse<0>((char*)pPcmCacheBuffer+sizeof(WebSocketPCMHead));
		if (!doc.HasParseError())
		{
			strcpy(audioRegisterStruct.method ,doc["method"].GetString());
			strcpy(audioRegisterStruct.app, doc["app"].GetString());
			strcpy(audioRegisterStruct.stream, doc["stream"].GetString());
			strcpy(audioRegisterStruct.audioCodec, doc["audioCodec"].GetString());
			strcpy(audioRegisterStruct.targetAudioCodec, doc["targetAudioCodec"].GetString());

			audioRegisterStruct.channels = doc["channels"].GetInt64();
			audioRegisterStruct.sampleRate = doc["sampleRate"].GetInt64();
 		}

		//检测期望的编码格式 
		if (!(strcmp(audioRegisterStruct.targetAudioCodec, "g711a") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "g711u") == 0 ||
			  strcmp(audioRegisterStruct.targetAudioCodec, "G711A") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "G711U") == 0 ||
			  strcmp(audioRegisterStruct.targetAudioCodec, "aac") == 0  || strcmp(audioRegisterStruct.targetAudioCodec, "AAC") == 0   ))
		{
			WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu , 不支持期望的音频编码格式 %s ", this, nClient,audioRegisterStruct.targetAudioCodec);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		//检测媒体流是否存在 
		strcpy(m_addStreamProxyStruct.app, audioRegisterStruct.app);
		strcpy(m_addStreamProxyStruct.stream, audioRegisterStruct.stream);
		sprintf(m_szShareMediaURL, "/%s/%s", audioRegisterStruct.app, audioRegisterStruct.stream);
		pMediaSouce = GetMediaStreamSource(m_szShareMediaURL, false);
		if (pMediaSouce != NULL)
		{
			WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu , 媒体源 %s 已经存在，重新启用新的名字", this, nClient, m_szShareMediaURL);
			DeleteNetRevcBaseClient(nClient);
			return;
		}
		pMediaSouce =  CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
		if(pMediaSouce == NULL)
		{
			WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu , 创建媒体源失败 %s ", this, nClient, m_szShareMediaURL);
			DeleteNetRevcBaseClient(nClient);
			return;
		}
		WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu , 创建媒体源成功  %s", this, nClient, m_szShareMediaURL);

		//确定PCM切割长度 
		if (strcmp(audioRegisterStruct.targetAudioCodec, "g711a") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "g711u") == 0 ||
			  strcmp(audioRegisterStruct.targetAudioCodec, "G711A") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "G711U") == 0)
			SplitterMaxPcmLength = SplitterMaxPcmLength_G711;
		else if (strcmp(audioRegisterStruct.targetAudioCodec, "aac") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "AAC") == 0)
		{
			SplitterMaxPcmLength = SplitterMaxPcmLength_AAC;
			aacEnc.InitAACEncodec(64000, audioRegisterStruct.sampleRate, audioRegisterStruct.channels, &nAACEncodeLength);
		}
	}
	else if (pcmHead.nType == 0x02)
	{
		//WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu , 收到 pcm 数据 nPcmLength = %d ", this, nClient, nPcmLength);
		if (MaxPcmChacheBufferLength - nPcmSplitterLength >= nPcmLength)
		{//pcm数据进行拼接 
			memcpy(pPcmSplitterBuffer + nPcmSplitterLength, pPcmCacheBuffer + sizeof(WebSocketPCMHead), nPcmLength);
 		    nPcmSplitterLength += nPcmLength ;
		}
		else
		{//数据异常 
			nPcmSplitterLength = 0;
			return;
		}

		//对PCM数据进行切割 ，长度为  SplitterMaxPcmLength
		nSplitterCount = 0;
		while (nPcmSplitterLength >= SplitterMaxPcmLength)
		{
			if (strcmp(audioRegisterStruct.targetAudioCodec, "g711a") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "g711u") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "G711A") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "G711U") == 0)
 		        AudioPcmResamle(pPcmSplitterBuffer + (nSplitterCount * SplitterMaxPcmLength), SplitterMaxPcmLength);
			else if (strcmp(audioRegisterStruct.targetAudioCodec, "aac") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "AAC") == 0)
			{//AAC编码
				memcpy(aacEnc.pbPCMBuffer, pPcmSplitterBuffer + (nSplitterCount * SplitterMaxPcmLength), nAACEncodeLength);
				aacEnc.EncodecAAC(&nRetunEncodeLength);

				if (nRetunEncodeLength > 0)
 					pMediaSouce->PushAudio(aacEnc.pbAACBuffer, nRetunEncodeLength, "AAC", audioRegisterStruct.channels, audioRegisterStruct.sampleRate);
   			}

		   nPcmSplitterLength -= SplitterMaxPcmLength;
		   nSplitterCount ++;
		}

		//把剩余数据往前移动
		if (nPcmSplitterLength > 0)
		{
			memmove(pPcmSplitterBuffer, pPcmSplitterBuffer + (nSplitterCount * SplitterMaxPcmLength), nPcmSplitterLength);
		}
	}
	else if (pcmHead.nType == 0x03)
	{
		WriteLog(Log_Debug, "ProcessPcmCacheBuffer() = %X  nClient = %llu , 接收到音频注销包", this, nClient);
		DeleteNetRevcBaseClient(nClient);
		return;
	}

	//移动缓存位置
 	nPcmCacheBufferLength -= nPcmLength + sizeof(WebSocketPCMHead);
	if (nPcmCacheBufferLength > 0)
	{//有剩余数据 ，需要往前移动 

	}
}

//音频数据重采样
void   CNetServerRecvAudio::AudioPcmResamle(unsigned char* inPCM, int nPcmDataLength)
{
	int ret;
	int src_nb_channels;
	if (swr_ctx == NULL)
	{//初始化转换
 		swr_ctx = swr_alloc();

		if(audioRegisterStruct.channels == 1)
		  nInChannels = AV_CH_LAYOUT_MONO; //单声道
		else if(audioRegisterStruct.channels == 2)
		  nInChannels = AV_CH_LAYOUT_STEREO; //双声道

	

#ifdef  FFMPEG6

		AVChannelLayout out_ch_layout;
		out_ch_layout.nb_channels = 1;
		AVChannelLayout inChannelLayout;
		inChannelLayout.nb_channels = nInChannels;

		int32_t error = swr_alloc_set_opts2(&swr_ctx,
			&out_ch_layout,
			AV_SAMPLE_FMT_S16,
			outSampleRate,
			&inChannelLayout,
			AV_SAMPLE_FMT_S16,
			audioRegisterStruct.sampleRate,
			0,
			nullptr);
		if (error < 0) {
			std::cerr << "Failed to allocate SwrContext" << std::endl;
			// 处理错误，可以抛出异常或采取其他措施
		}

		error = swr_init(swr_ctx);
		if (error < 0) {
			std::cerr << "Failed to initialize SwrContext" << std::endl;
			// 处理错误，可以抛出异常或采取其他措施
		}
#else
		in_channel_layout = av_get_default_channel_layout(1);
		swr_alloc_set_opts(swr_ctx, in_channel_layout, AV_SAMPLE_FMT_S16, outSampleRate,
			nInChannels, AV_SAMPLE_FMT_S16, audioRegisterStruct.sampleRate, 0, NULL);

		swr_init(swr_ctx);

		src_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);
		ret = av_samples_alloc_array_and_samples(&pcm_buffer, &src_linesize, src_nb_channels, SplitterMaxPcmLength / 2, AV_SAMPLE_FMT_S16, 0);

		src_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);
		ret = av_samples_alloc_array_and_samples(&pcm_OutputBuffer, &dst_linesize, src_nb_channels, SplitterMaxPcmLength, AV_SAMPLE_FMT_S16, 0);
#endif //  FFMPEG6
		
  	}
 	memcpy(pcm_buffer[0], inPCM, nPcmDataLength);
	nResampleLength = swr_convert(swr_ctx,pcm_OutputBuffer, dst_linesize,(const uint8_t **)pcm_buffer, nPcmDataLength / 2 );

	if (nResampleLength == (SplitterMaxPcmLength / 4))
	{//够640 
		memset(szG711Encodec, 0x00, sizeof(szG711Encodec));
		if (strcmp(audioRegisterStruct.targetAudioCodec, "g711a") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "G711A") == 0)
		{
		   pcm16_to_alaw(640, (char*)pcm_OutputBuffer[0],(char*)szG711Encodec);
		   pMediaSouce->PushAudio(szG711Encodec, 320, "G711_A", 1, 8000);
		}
		else if (strcmp(audioRegisterStruct.targetAudioCodec, "g711u") == 0 || strcmp(audioRegisterStruct.targetAudioCodec, "G711U") == 0)
		{
			pcm16_to_ulaw(640, (char*)pcm_OutputBuffer[0], (char*)szG711Encodec);
			pMediaSouce->PushAudio(szG711Encodec, 320, "G711_U", 1, 8000);
		}
	}

#ifdef WritePCMDaFile
	if (fWritePCMFile && nResampleLength == (SplitterMaxPcmLength / 4  ))
	{
 		fwrite(pcm_OutputBuffer[0], 1, nResampleLength * 2 , fWritePCMFile);
		fflush(fWritePCMFile);
	}
#endif
}

bool  CNetServerRecvAudio::Create_WS_FLV_Handle()
{
	std::lock_guard<std::mutex> lock(NetServerWS_FLVLock);

	if (!bFindFlvNameFlag)
	{
		if (strstr((char*)netDataCache, "\r\n\r\n") == NULL)
		{
 			if (memcmp(netDataCache, "GET ", 4) != 0)
			{
 				DeleteNetRevcBaseClient(nClient);
			}
			return -1;
		}
	}

	if (!bCheckHttpFlvFlag)
	{
		bCheckHttpFlvFlag = true;

		//把请求的FLV文件读取出来　
		string  strHttpHead = (char*)netDataCache;
		int     nPos1, nPos2;
		nPos1 = strHttpHead.find("GET ", 0);
		if (nPos1 >= 0)
		{
			nPos2 = strHttpHead.find(" HTTP/", 0);
			if (nPos2 > 0)
			{
				bFindFlvNameFlag = true;
				memset(szFlvName, 0x00, sizeof(szFlvName));
				memcpy(szFlvName, netDataCache + nPos1 + 4, nPos2 - nPos1 - 4);
  			}
		}

		if (!bFindFlvNameFlag)
		{
 			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

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
 			DeleteNetRevcBaseClient(nClient);
		}
		nWebSocketCommStatus = WebSocketCommStatus_ShakeHands;

		nNetStart = nNetEnd = netDataCacheLength = 0;
		memset(netDataCache, 0x00, m_nMaxRecvBufferLength);

		char szResponseClientKey[256] = { 0 };
		char szSHA1Buffer[256] = { 0 };
		strcat(szSec_WebSocket_Key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
		string sha1sum = SHA1::encode_bin(szSec_WebSocket_Key); 
		memcpy(szSHA1Buffer, sha1sum.c_str(), sha1sum.length());
		base64_encode(szResponseClientKey, szSHA1Buffer, strlen(szSHA1Buffer));

		memset(szWebSocketResponse, 0x00, sizeof(szWebSocketResponse));
		sprintf(szWebSocketResponse, "HTTP/1.1 101 Switching Protocol\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: Upgrade\r\nDate: Mon, Nov 08 2021 01:52:45 GMT\r\nKeep-Alive: timeout=30, max=100\r\nSec-WebSocket-Accept: %s\r\nServer: %s\r\nUpgrade: websocket\r\nSec_WebSocket_Protocol: %s\r\n\r\n", szOrigin, szResponseClientKey, MediaServerVerson, szSec_WebSocket_Protocol);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)szWebSocketResponse, strlen(szWebSocketResponse), true);
		if (nWriteRet != 0)
		{
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		///printf(szWebSocketResponse);

		nWebSocketCommStatus = WebSocketCommStatus_ShakeHands;
 
		return true;
	}
}

//发送第一个请求
int CNetServerRecvAudio::SendFirstRequst()
{
	return 0;
}

//请求m3u8文件
bool  CNetServerRecvAudio::RequestM3u8File()
{
	return true;
}

//flv数据前面增加 websocket头 
bool  CNetServerRecvAudio::SendWebSocketData(unsigned char* pData, int nDataLength)
{
	std::lock_guard<std::mutex> lock(NetServerWS_FLVLock);

	if (!bRunFlag)
		return false;
	if (nDataLength >= 0 && nDataLength <= 125)
	{
		memset(webSocketHead, 0x00, sizeof(webSocketHead));
		webSocketHead[0] = 0x81;
		webSocketHead[1] = nDataLength;
		nWriteRet = XHNetSDK_Write(nClient, webSocketHead, 2, true);
		nWriteRet2 = XHNetSDK_Write(nClient, pData, nDataLength, true);
	}
	else if (nDataLength >= 126 && nDataLength <= 0xFFFF)
	{
		memset(webSocketHead, 0x00, sizeof(webSocketHead));
		webSocketHead[0] = 0x81;
		webSocketHead[1] = 0x7E;

		wsLength16 = nDataLength;
		wsLength16 = htons(wsLength16);
		memcpy(webSocketHead + 2, (unsigned char*)&wsLength16, sizeof(wsLength16));
		nWriteRet = XHNetSDK_Write(nClient, webSocketHead, 4, true);
		nWriteRet2 = XHNetSDK_Write(nClient, pData, nDataLength, true);
 	}
	else if (nDataLength > 0xFFFF)
	{
		memset(webSocketHead, 0x00, sizeof(webSocketHead));
		webSocketHead[0] = 0x81;
		webSocketHead[1] = 0x7F;

		wsLenght64 = nDataLength;
		wsLenght64 = htonl(wsLenght64);
		memcpy(webSocketHead + 2+4, (unsigned char*)&wsLenght64, sizeof(wsLenght64));
		nWriteRet = XHNetSDK_Write(nClient, webSocketHead, 10, true);
		nWriteRet2 = XHNetSDK_Write(nClient, pData, nDataLength, true);
	}
	else
		return false;

	if(nWriteRet == 0 && nWriteRet2 == 0)
	  return true ;
	else 
	{
		DeleteNetRevcBaseClient(nClient);
		return false;
	}
}

/*******************************************************************************
* 名称: webSocket_dePackage
* 功能: websocket数据收发阶段的数据解包, 通常client发server的数据都要isMask(掩码)处理, 反之server到client却不用
* 形参: *data：解包的数据
* dataLen : 长度
* *package : 解包后存储地址
* packageMaxLen : 存储地址可用长度
* *packageLen : 解包所得长度
* 返回: 解包识别的数据类型 如 : txt数据, bin数据, ping, pong等
* 说明: 无
******************************************************************************/
int CNetServerRecvAudio::webSocket_dePackage(unsigned char *data, unsigned int dataLen, unsigned char *package, unsigned int packageMaxLen, unsigned int *packageLen, unsigned int *packageHeadLen)
{
	unsigned char maskKey[4] = { 0 }; // 掩码
	unsigned char temp1, temp2;
	char Mask = 0, type;
	int count, ret;
	unsigned int i, len = 0, dataStart = 2;
	if (dataLen < 2)
		return WDT_ERR;

	type = data[0] & 0x0F;
	if ((data[0] & 0x80) == 0x80)
	{
		if (type == 0x01)
			ret = WDT_TXTDATA;
		else if (type == 0x02)
			ret = WDT_BINDATA;
		else if (type == 0x08)
			ret = WDT_DISCONN;
		else if (type == 0x09)
			ret = WDT_PING;
		else if (type == 0x0A)
 			ret = WDT_PONG;
 		else
 			return WDT_ERR;
 	}
 	else if (type == 0x00)
 		ret = WDT_MINDATA;
 	else
 		return WDT_ERR;
 
	if ((data[1] & 0x80) == 0x80)
 	{
		Mask = 1;
 		count = 4;
 	}
 	else
 	{
		Mask = 0;
 		count = 0;
 	}
 
	len = data[1] & 0x7F;
 
	if (len == 126)
 	{
		if (dataLen < 4)
 			return WDT_ERR;

		len = data[2];
 		len = (len << 8) + data[3];
 		if (packageLen) *packageLen = len;//转储包长度
 		if (packageHeadLen) *packageHeadLen = 4 + count;
 
		if (dataLen < len + 4 + count)
 			return WDT_ERR;
 		if (Mask)
 		{
			maskKey[0] = data[4];
 			maskKey[1] = data[5];
 			maskKey[2] = data[6];
 			maskKey[3] = data[7];
 			dataStart = 8;
 		}
 		else
 			dataStart = 4;
 	}
	else if (len == 127)
 	{
		if (dataLen < 10)
 			return WDT_ERR;
 		if (data[2] != 0 || data[3] != 0 || data[4] != 0 || data[5] != 0) //使用8个字节存储长度时,前4位必须为0,装不下那么多数据...
 			return WDT_ERR;

		len = data[6];
 		len = (len << 8) + data[7];
 		len = (len << 8) + data[8];
 		len = (len << 8) + data[9];
 		if (packageLen) 
			*packageLen = len;//转储包长度
 		if (packageHeadLen) 
		  *packageHeadLen = 10 + count;
 		//

		if (dataLen < len + 10 + count)
 			return WDT_ERR;

		if (Mask)
 		{
			maskKey[0] = data[10];
 			maskKey[1] = data[11];
 			maskKey[2] = data[12];
 			maskKey[3] = data[13];
 			dataStart = 14;
 		}
 		else
 			dataStart = 10;
 	}
 	else
 	{
		if (packageLen) *packageLen = len;//转储包长度
 		if (packageHeadLen) *packageHeadLen = 2 + count;
 
		if (dataLen < len + 2 + count)
 			return WDT_ERR;

		if (Mask)
 		{
			maskKey[0] = data[2];
 			maskKey[1] = data[3];
 			maskKey[2] = data[4];
 			maskKey[3] = data[5];
 			dataStart = 6;
 		}
 		else
 			dataStart = 2;
 	}

 	if (dataLen < len + dataStart)
 		return WDT_ERR;
 
	if (packageMaxLen < len + 1)
 		return WDT_ERR;

	//检测数据是否接收完毕 
 
	if (Mask) // 解包数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下
 	{
		for (i = 0, count = 0; i < len; i++)
 		{
			temp1 = maskKey[count];
 			temp2 = data[i + dataStart];
 			*package++ = (char)(((~temp1)&temp2) | (temp1&(~temp2))); // 异或运算后得到数据###与接收端"^"运算符结果一样 by cy###
 			count += 1;
 			if (count >= sizeof(maskKey)) // maskKey[4]循环使用
 				count = 0;
 		}
 		*package = '\0';
	}
	else // 解包数据没使用掩码, 直接复制数据段
	{
		memcpy(package, &data[dataStart], len);
		package[len] = '\0';
 	}

	return ret;
}