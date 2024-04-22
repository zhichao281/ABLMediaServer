/*
功能：
       实现HLS服务器的媒体数据发送功能 
日期    2021-05-20   实况hls支持
        2023-11-06   增加录像回放hls的支持 

作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerHLS.h"

extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);

#ifdef USE_BOOST
extern boost::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#else
extern std::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#endif


extern CMediaFifo                      pDisconnectBaseNetFifo; //清理断裂的链接 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern char                            ABL_wwwMediaPath[256]; //www 子路径
extern MediaServerPort                 ABL_MediaServerPort;
extern uint64_t                        ABL_nBaseCookieNumber ; //Cookie 序号 
extern uint64_t                        GetCurrentSecond();
extern CMediaFifo                      pMessageNoticeFifo; //消息通知FIFO

CNetServerHLS::CNetServerHLS(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	memset(szOrigin, 0x00, sizeof(szOrigin));
	strcpy(m_szShareMediaURL, szShareMediaURL);

	MaxNetDataCacheCount = MaxHttp_FlvNetCacheBufferLength;
	memset(netDataCache, 0x00, sizeof(netDataCache));
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//网络数据缓存大小
	bFindHLSNameFlag = false;
	memset(szRequestFileName, 0x00, sizeof(szRequestFileName));

	netBaseNetType = NetBaseNetType_HttpHLSServerSendPush;
	bRequestHeadFlag = false;
	memset(szCookieNumber, 0x00, sizeof(szCookieNumber));

	//首次分配内存 
	pTsFileBuffer = NULL;
	while(pTsFileBuffer == NULL)
	{
		nCurrentTsFileBufferSize = MaxDefaultTsFmp4FileByteCount;
		pTsFileBuffer = new unsigned char[nCurrentTsFileBufferSize];
	}
	WriteLog(Log_Debug, "CNetServerHLS 构造 = %X,  nClient = %llu ",this, nClient);
}

CNetServerHLS::~CNetServerHLS()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(netDataLock);
	
	if (pTsFileBuffer != NULL)
	{
	  delete [] pTsFileBuffer;
	  pTsFileBuffer;
	}
 
	WriteLog(Log_Debug, "CNetServerHLS 析构 = %X  szRequestFileName = %s, nClient = %llu \r\n", this, szRequestFileName, nClient);
	malloc_trim(0);
}

int CNetServerHLS::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	return 0 ;
}

int CNetServerHLS::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	return 0;
}

int CNetServerHLS::SendVideo()
{

	return 0;
}

int CNetServerHLS::SendAudio()
{

	return 0;
}

int CNetServerHLS::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag)
		return -1;
	std::lock_guard<std::mutex> lock(netDataLock);

	//网络断线检测
	nRecvDataTimerBySecond = 0;

	//WriteLog(Log_Debug, "CNetServerHLS= %X , nClient = %llu ,收到片段数据\r\n%s",this,nClient,pData);
 
	if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
	{//剩余空间足够
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//剩余空间不够，需要把剩余的buffer往前移动
		if (netDataCacheLength > 0 && netDataCacheLength == (nNetEnd - nNetStart) )
		{//如果有少量剩余
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;

			if (MaxNetDataCacheCount - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetRtspServer = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return 0;
			}
			//清空历史废旧数据
			memset(netDataCache + nNetEnd, 0x00, sizeof(netDataCache) - nNetEnd);
		}
		else
		{//没有剩余，那么 首，尾指针都要复位 
			nNetStart = 0;
			nNetEnd = 0;
			netDataCacheLength = 0;

			//清空历史废旧数据
			memset(netDataCache , 0x00, sizeof(netDataCache));
		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	return true;
}

//从HTTP头中获取请求的文件名
bool CNetServerHLS::GetHttpRequestFileName(char* szGetRequestFile,char* szHttpHeadData)
{
	string  strHttpHead = (char*)szHttpHeadData;
	int     nPos1, nPos2;
	char    szFindHead[64] = { 0 };
 	int     nHeadLength = 4 ;

	strcpy(szFindHead, "HEAD ");
	nPos1 = strHttpHead.find(szFindHead, 0);
	if (nPos1 < 0)
	{
		memset(szFindHead, 0x00, sizeof(szFindHead));
		strcpy(szFindHead, "GET ");
		nPos1 = strHttpHead.find(szFindHead, 0);
	}
	else//采用head 请求m3u8 文件 
		bRequestHeadFlag = true;

	nHeadLength = strlen(szFindHead);

	if (nPos1 >= 0)
	{
		nPos2 = strHttpHead.find(" HTTP/", 0);
		if (nPos2 > 0)
		{
			memcpy(szGetRequestFile, szHttpHeadData + nPos1 + nHeadLength, nPos2 - nPos1 - nHeadLength);
 			//WriteLog(Log_Debug, "CNetServerHLS=%X ,nClient = %llu ,拷贝出HTTP 请求 文件名字 %s ", this, nClient, szGetRequestFile);
			return true;
		}
	}
	return false;
}

//读取http请求buffer 
bool  CNetServerHLS::ReadHttpRequest()
{
	std::lock_guard<std::mutex> lock(netDataLock);

	if (netDataCacheLength <= 0 || nNetStart <  0)
  		return false;
 
	string strNetData = (char*)netDataCache ;
	int    nPos;
	nPos = strNetData.find("\r\n\r\n", nNetStart);

	if (nPos <= 0 || (nPos - nNetStart) + 4 > sizeof(szHttpRequestBuffer))
 		return false;
 
	memset(szHttpRequestBuffer, 0x00, sizeof(szHttpRequestBuffer));
	memcpy(szHttpRequestBuffer, netDataCache + nNetStart , (nPos - nNetStart) + 4);

	netDataCacheLength   -=   (nPos - nNetStart) + 4 ; //这个4 ，就是 "\r\n\r\n" , 已经拷贝走了。
	nNetStart             =    nPos  + 4 ; //这个4 ，就是 "\r\n\r\n"， 已经拷贝走了。

 	return true;
}
 
int CNetServerHLS::ProcessNetData()
{
	if (!bRunFlag)
		return -1;

	if (netDataCacheLength > 512 || strstr((char*)netDataCache, "%") != NULL)
	{
		WriteLog(Log_Debug, "CNetServerHLS = %X , nClient = %llu ,netDataCacheLength = %d, 发送过来的url数据长度非法 ,立即删除 ", this, nClient, netDataCacheLength);
		DeleteNetRevcBaseClient(nClient);
	}

  	if (ReadHttpRequest() == false )
	{
		WriteLog(Log_Debug, "CNetServerHLS = %X ,nClient = %llu , 数据尚未接收完整 ",this,nClient);
		if (memcmp(netDataCache, "GET ", 4) != 0)
		{
			WriteLog(Log_Debug, "CNetServerHLS = %X , nClient = %llu , 接收的数据非法 ",this, nClient);
			DeleteNetRevcBaseClient(nClient);
		}
		return -1;
 	}

	memset(szDateTime1, 0x00, sizeof(szDateTime1));
	memset(szDateTime2, 0x00, sizeof(szDateTime2));
	
#ifdef OS_System_Windows	
	SYSTEMTIME st;
	GetLocalTime(&st);//Tue, Jun 31 2021 06:19:02 GMT
	sprintf(szDateTime1,"Tue, Jun %02d %04d %02d:%02d:%02d GMT", st.wDay, st.wYear, st.wHour - 8,st.wMinute, st.wSecond);
	sprintf(szDateTime2, "Tue, Jun %02d %04d %02d:%02d:%02d GMT", st.wDay, st.wYear, st.wHour - 8, st.wMinute+2, st.wSecond);
#else
	time_t now;
	time(&now);
	struct tm *local;
	local = localtime(&now);
	
	sprintf(szDateTime1,"Tue, Jun %02d %04d %02d:%02d:%02d GMT", local->tm_mday, local->tm_year+1900, local->tm_hour - 8,local->tm_min, local->tm_sec);
	sprintf(szDateTime2, "Tue, Jun %02d %04d %02d:%02d:%02d GMT", local->tm_mday,local->tm_year+1900, local->tm_hour - 8, local->tm_min+2, local->tm_sec);
#endif

	//分析http头
	httpParse.ParseSipString((char*)szHttpRequestBuffer);

	//获取出HTTP请求的文件名字 
	memset(szRequestFileName, 0x00, sizeof(szRequestFileName));
	if (!GetHttpRequestFileName(szRequestFileName, szHttpRequestBuffer))
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, 获取http 请求文件失败！ nClient = %llu ", this, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}  

 	string strRequestFileName = szRequestFileName;
	int    nPos = 0 ;

    //拷贝鉴权参数
	if (strlen(szPlayParams) == 0)
	{
		nPos = strRequestFileName.find("?", 0);
		if(nPos > 0 )
		  memcpy(szPlayParams, szRequestFileName + (nPos + 1), strlen(szRequestFileName) - nPos - 1);
 	}

	memset(szPushName, 0x00, sizeof(szPushName));
	nPos = strRequestFileName.rfind(".m3u8",strlen(szRequestFileName));
	if (nPos > 0)
	{
		memcpy(szPushName,szRequestFileName,nPos);
 	}else 
	{
		nPos = strRequestFileName.rfind("/", strlen(szRequestFileName));
		if(nPos > 0)
		   memcpy(szPushName, szRequestFileName, nPos);
	}
	SplitterAppStream(szPushName);

	//发送播放事件通知，用于播放鉴权
	if (ABL_MediaServerPort.hook_enable == 1 && bOn_playFlag == false)
	{
 		bOn_playFlag = true;
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_on_play;
		sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu,\"ip\":\"%s\" ,\"port\":%d,\"params\":\"%s\"}", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, nClient, szClientIP, nClientPort, szPlayParams);
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}

  	//WriteLog(Log_Debug, "CNetServerHLS=%X, 获取到HLS的推流源名字 szPushName = %s , nClient = %llu ", this, szPushName, nClient);

	//获取Connection 连接方式： Close ,Keep-Live  
	memset(szConnectionType, 0x00, sizeof(szConnectionType));
	if (!httpParse.GetFieldValue("Connection", szConnectionType))
		strcpy(szConnectionType, "Close");

	//更新Cookie 
 	memset(szCookieNumber, 0x00, sizeof(szCookieNumber));
#if  1 //采用自研的Cookie算法
	sprintf(szCookieNumber, "ABLMediaServer%018llu", ABL_nBaseCookieNumber);
	ABL_nBaseCookieNumber ++;
#else 
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()(); // 这里是两个() ，因为这里是调用的 () 的运算符重载
	string tmp_uuid = boost::uuids::to_string(a_uuid);
	boost::algorithm::erase_all(tmp_uuid, "-");
	strcpy(szCookieNumber,tmp_uuid.c_str());
#endif

	//获取 szOrigin
	memset(szOrigin, 0x00, sizeof(szOrigin));
	httpParse.GetFieldValue("Origin", szOrigin);
	if (strlen(szOrigin) == 0)
		strcpy(szOrigin, "*");
 	
	//WriteLog(Log_Debug, "CNetServerHLS= %X, nClient = %llu , 获取的请求文件 szRequestFileName = %s ", this,  nClient, szRequestFileName);
	if (strstr(szRequestFileName, RecordFileReplaySplitter) != NULL)
	{//录像回放
		SendRecordHLS();
	}else //实况播放 
	    SendLiveHLS();//发送实况的hls 

#if 0  //服务器不能主动断开，否则VLC播放不正常 ,ffplay 也经常播放不正常
	  //发送完毕,如果是短连接，立即删除
	  if(strcmp(szConnectionType,"Close") == 0 || strcmp(szConnectionType, "close") == 0 || bRequestHeadFlag == true)
	      pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
#endif


	return 0;
}

int CNetServerHLS::SendLiveHLS()
{
	//根据推流名字找到
	auto pushClient = GetMediaStreamSource(szPushName, true);
	if (pushClient == NULL)
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, 没有推流对象的地址 %s nClient = %llu ", this, szPushName, nClient);

		sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

		DeleteNetRevcBaseClient(nClient);
		return -1;
	}

	//更新HLS最后观看时间,因为HLS播放，采用这种方式来判断某路流是否在观看　
	pushClient->nLastWatchTime = pushClient->nLastWatchTimeDisconect = GetCurrentSecond();

	//记下媒体源
	sprintf(m_addStreamProxyStruct.url, "http://localhost:%d/%s/%s.m3u8", ABL_MediaServerPort.nHlsPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);

	if (strstr(szRequestFileName, ".m3u8") != NULL)
	{//请求M3U8文件
		if (strlen(pushClient->szDataM3U8) == 0)
		{//m3u8文件尚未生成，即刚刚开始切片，但是还不够3个文件 
			WriteLog(Log_Debug, "CNetServerHLS=%X, m3u8文件尚未生成，即刚刚开始切片，但是还不够3个TS文件 %s nClient = %llu ", this, szPushName, nClient);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		memset(szM3u8Content, 0x00, sizeof(szM3u8Content));
		strcpy(szM3u8Content, pushClient->szDataM3U8);
		if (bRequestHeadFlag == true)
		{//HEAD 请求
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: close\r\nContent-Length: 0\r\nDate: %s\r\nServer: %s\r\n\r\n", szOrigin, szDateTime1, MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
			WriteLog(Log_Debug, "CNetServerHLS=%X, 回复HEAD请求 httpResponseData = %s, nClient = %llu ", this, httpResponseData, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return 0;
		}
		else
		{
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: application/vnd.apple.mpegurl; charset=utf-8\r\nDate: %s\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\nSet-Cookie: AB_COOKIE=%s;expires=%s;path=%s/\r\n\r\n", szOrigin, szConnectionType,
				strlen(szM3u8Content), szDateTime1, MediaServerVerson, szCookieNumber, szDateTime2, szPushName);
		}

		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
		nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)szM3u8Content, strlen(szM3u8Content), 1);
		if (nWriteRet != 0 || nWriteRet2 != 0)
		{
			WriteLog(Log_Debug, "CNetServerHLS=%X, 回复http失败 szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		WriteLog(Log_Debug, "CNetServerHLS=%X, 发送完毕m3u8文件 szRequestFileName = %s, nClient = %llu , 文件字节大小 %d ", this, szRequestFileName, nClient, strlen(szM3u8Content));
		//WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu 发出http回复：\r\n%s", this, nClient, httpResponseData);
		//WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu 发出http回复：\r\n%s", this, nClient, szM3u8Content);
	}
	else if (strstr(szRequestFileName, ".ts") != NULL || strstr(szRequestFileName, ".mp4") != NULL)
	{//请求TS文件 
		nTsFileNameOrder = GetTsFileNameOrder(szRequestFileName);
		if (nTsFileNameOrder == -1)
		{
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		//TS文件总字节数
		fFileByteCount = pushClient->GetTsFileSizeByOrder(nTsFileNameOrder);
		if (fFileByteCount <= 0)
		{//TS文件字节数量有误
			WriteLog(Log_Debug, "CNetServerHLS=%X, 文件打开失败 szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		//如果要读取的文件字节数大于  nCurrentTsFileBufferSize
		if (fFileByteCount > nCurrentTsFileBufferSize)
		{
			delete[] pTsFileBuffer;
			nCurrentTsFileBufferSize = fFileByteCount + 1024 * 512; //再扩大512K 
			pTsFileBuffer = new unsigned char[nCurrentTsFileBufferSize];
		}

		if (ABL_MediaServerPort.nHLSCutType == 1)
		{//切片到硬盘
#ifdef OS_System_Windows
			sprintf(szReadFileName, "%s\\%s", ABL_wwwMediaPath, szRequestFileName);
#else 
			sprintf(szReadFileName, "%s/%s", ABL_wwwMediaPath, szRequestFileName);
#endif
			WriteLog(Log_Debug, "CNetServerHLS=%X, 开始读取TS文件 szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);
			FILE* tsFile = fopen(szReadFileName, "rb");
			if (tsFile == NULL)
			{//打开TS文件失败
				WriteLog(Log_Debug, "CNetServerHLS=%X, 文件打开失败 szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return -1;
			}
			fread(pTsFileBuffer, 1, fFileByteCount, tsFile);
			fclose(tsFile);
		}
		else if (ABL_MediaServerPort.nHLSCutType == 2)
		{
			if (ABL_MediaServerPort.nH265CutType == 2)
			{
				if (nTsFileNameOrder == 0)//请求的是 SPS \ PPS 的 0.mp4 文件 
				{
					if (pushClient->nFmp4SPSPPSLength > 0)
						memcpy(pTsFileBuffer, pushClient->pFmp4SPSPPSBuffer, pushClient->nFmp4SPSPPSLength);
					else
					{
						WriteLog(Log_Debug, "CNetServerHLS=%X, fmp4 切片没有生成 0.mp4 文件 szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

						sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
						nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

						pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
						return -1;
					}
				}
				else
					pushClient->CopyTsFileBuffer(nTsFileNameOrder, pTsFileBuffer);
			}
			else
				pushClient->CopyTsFileBuffer(nTsFileNameOrder, pTsFileBuffer);
		}

		//发送http头
		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: video/mp2t; charset=utf-8\r\nDate: %s\r\nkeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin,
			szConnectionType,
			fFileByteCount,
			szDateTime1,
			MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

		//发送TS码流 
		int             nPos = 0;
		while (fFileByteCount > 0 && pTsFileBuffer != NULL)
		{
			if (fFileByteCount > Send_TsFile_MaxPacketCount)
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer + nPos, Send_TsFile_MaxPacketCount, 1);
				fFileByteCount -= Send_TsFile_MaxPacketCount;
				nPos += Send_TsFile_MaxPacketCount;
			}
			else
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer + nPos, fFileByteCount, 1);
				nPos += fFileByteCount;
				fFileByteCount = 0;
			}

			if (nWriteRet2 != 0)
			{//发送出错
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				break;
			}
		}
		WriteLog(Log_Debug, "CNetServerHLS=%X, 发送完毕TS、FMP4 文件 szRequestFileName = %s, nClient = %llu ,文件字节大小 %d", this, szRequestFileName, nClient, nPos);
	}
	else
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, 请求 http 文件类型有误 szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}
}

int CNetServerHLS::SendRecordHLS()
{
 	if (strstr(szRequestFileName, ".m3u8") != NULL)
	{//请求M3U8文件
		string  strTemp = szRequestFileName;
 
#ifdef USE_BOOST
		replace_all(strTemp, RecordFileReplaySplitter, "/");
#else
		ABL::replace_all(strTemp, RecordFileReplaySplitter, "/");
#endif
		sprintf(szRequestFileName, "%s%s", ABL_MediaServerPort.recordPath, strTemp.c_str()+1);
 
		FILE* fReadM3u8 = NULL;
		fReadM3u8 = fopen(szRequestFileName, "rb");
		
 		if (fReadM3u8  == NULL)
		{//不存在m3u8文件  
			WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu , 不存在文件 %s  ", this, nClient, szRequestFileName);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		memset(szM3u8Content, 0x00, sizeof(szM3u8Content));
		fread(szM3u8Content, 1, sizeof(szM3u8Content), fReadM3u8);
		fclose(fReadM3u8);

		if (bRequestHeadFlag == true)
		{//HEAD 请求
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: close\r\nContent-Length: 0\r\nDate: %s\r\nServer: %s\r\n\r\n", szOrigin, szDateTime1, MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
			WriteLog(Log_Debug, "CNetServerHLS=%X, 回复HEAD请求 httpResponseData = %s, nClient = %llu ", this, httpResponseData, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return 0;
		}
		else
		{
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: application/vnd.apple.mpegurl; charset=utf-8\r\nDate: %s\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\nSet-Cookie: AB_COOKIE=%s;expires=%s;path=%s/\r\n\r\n", szOrigin, szConnectionType,
				strlen(szM3u8Content), szDateTime1, MediaServerVerson, szCookieNumber, szDateTime2, szPushName);
		}

		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
		nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)szM3u8Content, strlen(szM3u8Content), 1);
		if (nWriteRet != 0 || nWriteRet2 != 0)
		{
			WriteLog(Log_Debug, "CNetServerHLS=%X, 回复http失败 szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		} 

		WriteLog(Log_Debug, "CNetServerHLS=%X, 发送完毕m3u8文件 szRequestFileName = %s, nClient = %llu , 文件字节大小 %d ", this, szRequestFileName, nClient, strlen(szM3u8Content));
 	}
	else if (strstr(szRequestFileName, ".ts") != NULL || strstr(szRequestFileName, ".mp4") != NULL)
	{//请求TS文件 
		string  strTemp = szRequestFileName;

#ifdef USE_BOOST
		replace_all(strTemp, RecordFileReplaySplitter, "/");
#else
		ABL::replace_all(strTemp, RecordFileReplaySplitter, "/");
#endif
	
		sprintf(szRequestFileName, "%s%s", ABL_MediaServerPort.recordPath, strTemp.c_str() + 1);

		FILE* fReadMP4 = NULL;
		fReadMP4 = fopen(szRequestFileName, "rb");

		if (fReadMP4 == NULL)
		{//不存在 mp4 文件 
			WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu , 不存在文件 %s  ", this, nClient, szRequestFileName);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
	
		//获取文件大小 
#ifdef OS_System_Windows 
		struct _stat64 fileBuf;
		int error = _stat64(szRequestFileName, &fileBuf);
		if (error == 0)
			fFileByteCount = fileBuf.st_size;
#else 
		struct stat fileBuf;
		int error = stat(szRequestFileName, &fileBuf);
		if (error == 0)
			fFileByteCount = fileBuf.st_size;
#endif

		//发送http头
		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: video/mp2t; charset=utf-8\r\nDate: %s\r\nkeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin,
			szConnectionType,
			fFileByteCount,
			szDateTime1,
			MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

		//发送TS码流 
		int             nRead = 0;
		while (true)
		{
			nRead = fread(pTsFileBuffer, 1, 1024 * 1024 * 1, fReadMP4);
			if (nRead > 0)
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer, nRead, 1);
 			}
			else
				break;
 
			if (nWriteRet2 != 0)
			{//发送出错
		        if(fReadMP4)
				{
				  fclose(fReadMP4);
				  fReadMP4 = NULL ;
				}
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				break;
			}
		}
		if(fReadMP4)
		{
		   fclose(fReadMP4) ;
		   fReadMP4 = NULL ;
		}
		WriteLog(Log_Debug, "CNetServerHLS=%X, 发送完毕TS、FMP4 文件 szRequestFileName = %s, nClient = %llu ,文件字节大小 %d", this, szRequestFileName, nClient, fFileByteCount);
 	}
	else
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, 请求 http 文件类型有误 szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}
}
 
//根据TS文件名字 ，获取文件序号 
int64_t  CNetServerHLS::GetTsFileNameOrder(char* szTsFileName)
{
	string strTsFileName = szTsFileName;
	int    nPos1, nPos2;
	char   szTemp[128] = { 0 };

	nPos1 = strTsFileName.rfind("/", strlen(szTsFileName));
	nPos2 = strTsFileName.find(".ts", 0);
	if (nPos1 > 0 && nPos2 > 0)
	{//ts 
		memcpy(szTemp, szTsFileName + nPos1 + 1, nPos2 - nPos1 - 1);
		return atoi(szTemp);
	}
	else
	{//mp4 
		nPos2 = strTsFileName.find(".mp4", 0);
		if (nPos1 > 0 && nPos2 > 0)
		{
		  memcpy(szTemp, szTsFileName + nPos1 + 1, nPos2 - nPos1 - 1);
		  return atoi(szTemp);
 		}
	}
	return  -1 ;
}

//发送第一个请求
int CNetServerHLS::SendFirstRequst()
{
	return 0;
}

//请求m3u8文件
bool  CNetServerHLS::RequestM3u8File()
{
	return true;
}