/*
功能：
    负责http-mp4 媒体数据发送
	 
日期    2021-12-18
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerHTTP_MP4.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern CNetBaseThreadPool*                   RecordReplayThreadPool;
extern CMediaFifo                            pMessageNoticeFifo;  //消息通知FIFO
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern MediaServerPort                       ABL_MediaServerPort;
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern CNetBaseThreadPool* RecordReplayThreadPool;
extern CMediaFifo                            pMessageNoticeFifo;  //消息通知FIFO
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
#endif
static int fmp4_hls_segment(void* param, const void* data, size_t bytes, int64_t pts, int64_t dts, int64_t duration)
{
	CNetServerHTTP_MP4* pNetServerHttpMp4 = (CNetServerHTTP_MP4*)param;
	if (pNetServerHttpMp4 == NULL)
		return 0;
	
	if(!pNetServerHttpMp4->bCheckHttpMP4Flag || !pNetServerHttpMp4->bRunFlag)
		return -1 ;

	if (bytes > 0)
	{
		pNetServerHttpMp4->SendTSBufferData((unsigned char*)data, bytes);
	}

#ifdef WriteMp4BufferToFile
	WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X fmp4_hls_segment nClient = %llu , bytes = %d", pNetServerHttpMp4, pNetServerHttpMp4->nClient, bytes);
	if (pNetServerHttpMp4->fWriteMP4)
	{
		fwrite(data, 1, bytes, pNetServerHttpMp4->fWriteMP4);
		fflush(pNetServerHttpMp4->fWriteMP4);

		if (GetTickCount() - pNetServerHttpMp4->nCreateDateTime >= 1000 * 120)
		{
			fclose(pNetServerHttpMp4->fWriteMP4);

			char szFileName[256] = { 0 };
			sprintf(szFileName, "%s%d.mp4", ABL_MediaSeverRunPath, GetTickCount());
			pNetServerHttpMp4->fWriteMP4 = fopen(szFileName, "wb");
			if (pNetServerHttpMp4->fWriteMP4 != NULL)
			{
				fwrite(pNetServerHttpMp4->s_packet, 1, pNetServerHttpMp4->s_packetLength, pNetServerHttpMp4->fWriteMP4);
				fflush(pNetServerHttpMp4->fWriteMP4);
 			}

			pNetServerHttpMp4->nCreateDateTime = GetTickCount();
		}
	}
#endif
	return 0;
}

CNetServerHTTP_MP4::CNetServerHTTP_MP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nCurrentRecordFileOrder = 0;
	pMP4Buffer = NULL;
	bOn_playFlag = false;
	memset((char*)&avc, 0x00, sizeof(avc));
	memset((char*)&hevc, 0x00, sizeof(hevc));
	bRunFlag = true;
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_HttpMP4ServerSendPush;
	nMediaClient = 0;
	bAddSendThreadToolFlag = false;
	bWaitIFrameSuccessFlag = false;

	nClient = hClient;
	hls_init_segmentFlag = false;
	audioDts = 0;
	videoDts = 0;
	track_aac = -1;
	track_video = -1;
	hlsFMP4 = hls_fmp4_create(512, fmp4_hls_segment, this);
	strcpy(szClientIP, szIP);
	nClientPort = nPort;

	memset(netDataCache ,0x00,sizeof(netDataCache));
	MaxNetDataCacheCount = sizeof(netDataCache);
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//网络数据缓存大小
	bFindMP4NameFlag = false;
	memset(szMP4Name, 0x00, sizeof(szMP4Name));
	bCheckHttpMP4Flag = false;
	nHttpDownloadSpeed = 6;
	httpMp4Type = HttpMp4Type_Unknow;
	fFileMp4 = NULL;

#ifdef WriteMp4BufferToFile
	char szFileName[256] = { 0 };
	sprintf(szFileName,"%s%d.mp4", ABL_MediaSeverRunPath, GetTickCount());
    fWriteMP4 = fopen(szFileName,"wb") ;
#endif
	WriteLog(Log_Debug, "CNetServerHTTP_MP4 构造 = %X nClient = %llu ", this, nClient);
}

CNetServerHTTP_MP4::~CNetServerHTTP_MP4()
{
    bCheckHttpMP4Flag = bRunFlag = false ;
	std::lock_guard<std::mutex> lock(mediaMP4MapLock);
	
	//删除fmp4切片句柄
	if (hlsFMP4 != NULL)
	{
		hls_fmp4_destroy(hlsFMP4);
		hlsFMP4 = NULL;
    }
	
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();

#ifdef WriteMp4BufferToFile
	if(fWriteMP4)
		fclose(fWriteMP4);
#endif

	if (fFileMp4)
	{//关闭读取MP4文件
		fclose(fFileMp4);
		fFileMp4 = NULL;
	}
	SAFE_ARRAY_DELETE(pMP4Buffer);

	WriteLog(Log_Debug, "CNetServerHTTP_MP4 析构 = %X nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetServerHTTP_MP4::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag)
		return -1;
	nRecvDataTimerBySecond = 0;
	m_videoFifo.push(pVideoData, nDataLength);
	return 0;  
}

int CNetServerHTTP_MP4::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag)
		return -1;
	 nRecvDataTimerBySecond = 0;

	if (strcmp(mediaCodecInfo.szAudioName, "AAC") != 0)
		return 0;

	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

int CNetServerHTTP_MP4::SendVideo()
{
    std::lock_guard<std::mutex> lock(mediaMP4MapLock);
	
	if(!bCheckHttpMP4Flag || !bRunFlag)
		return -1 ;

	nRecvDataTimerBySecond = 0;

	if (!bCheckHttpMP4Flag)
		return -1;

	//只有视频，或者屏蔽音频
	if (ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (hlsFMP4)
		{
			if (nMediaSourceType == MediaSourceType_LiveMedia)
				VideoFrameToFMP4File(pData, nLength);
			else
				VideoFrameToFMP4File(pData+4, nLength-4);
		}

		m_videoFifo.pop_front();
	}

	videoDts += nVideoStampAdd;
}

int CNetServerHTTP_MP4::SendAudio()
{
    std::lock_guard<std::mutex> lock(mediaMP4MapLock);

	if(!bCheckHttpMP4Flag || !bRunFlag || ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName, "AAC") != 0)
		return -1 ;
  
	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (ABL_MediaServerPort.nEnableAudio == 1 && strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		{
			if (nAsyncAudioStamp == -1)
				nAsyncAudioStamp = GetTickCount();

			avtype = PSI_STREAM_AAC;

			if (hlsFMP4 != NULL )
			{
				if (track_aac == -1)
				{
					if (nMediaSourceType == MediaSourceType_LiveMedia)
 					   nAACLength = mpeg4_aac_adts_frame_length(pData, nLength);
					else
						nAACLength = mpeg4_aac_adts_frame_length(pData+4, nLength-4);
					if (nAACLength < 0)
					{
						m_audioFifo.pop_front();
						return false;
					}

					if (nMediaSourceType == MediaSourceType_LiveMedia)
						mpeg4_aac_adts_load(pData, nLength, &aacHandle);
					else
						mpeg4_aac_adts_load(pData+4, nLength-4, &aacHandle);
					nExtenAudioDataLength = mpeg4_aac_audio_specific_config_save(&aacHandle, szExtenAudioData, sizeof(szExtenAudioData));

					if (nExtenAudioDataLength > 0)
					{
						track_aac = hls_fmp4_add_audio(hlsFMP4, MOV_OBJECT_AAC, mediaCodecInfo.nChannels, 16, mediaCodecInfo.nSampleRate, szExtenAudioData, nExtenAudioDataLength);
					}
				}

				//必须hls_init_segment 初始化完成才能写音频段，在回调函数里面做标志 
				if (track_aac >= 0 && hls_init_segmentFlag)
				{
					if (nMediaSourceType == MediaSourceType_LiveMedia)
						hls_fmp4_input(hlsFMP4, track_aac, pData + 7, nLength - 7, audioDts, audioDts, 0);
					else
						hls_fmp4_input(hlsFMP4, track_aac, pData + (4 + 7), nLength - 4-7, audioDts, audioDts, 0);
				}
			}

			audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp;

		    //同步音视频 
		    SyncVideoAudioTimestamp();
 		}
		m_audioFifo.pop_front();
	}
	return 0;
}

int CNetServerHTTP_MP4::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    std::lock_guard<std::mutex> lock(mediaMP4MapLock);
	
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
				WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
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
	return 0;
}

int CNetServerHTTP_MP4::ProcessNetData()
{
    std::lock_guard<std::mutex> lock(mediaMP4MapLock);
	nRecvDataTimerBySecond = 0 ;

	if (!bFindMP4NameFlag)
	{
		if (netDataCacheLength > string_length_4096 || strstr((char*)netDataCache, "%") != NULL)
		{
			WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X , nClient = %llu ,netDataCacheLength = %d, 发送过来的url数据长度非法 ,立即删除 ", this, nClient, netDataCacheLength);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		if (strstr((char*)netDataCache, "\r\n\r\n") == NULL)
		{
			WriteLog(Log_Debug, "数据尚未接收完整 ");
			if (memcmp(netDataCache, "GET ", 4) != 0)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X , nClient = %llu , 接收的数据非法 ", this, nClient);
				DeleteNetRevcBaseClient(nClient);
			}
			return -1;
		}
	}

	if (!bCheckHttpMP4Flag)
	{	
 		//把请求的FLV文件读取出来　
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
					WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X,请求文件名称长度非法 nClient = %llu ", this, nClient);
					DeleteNetRevcBaseClient(nClient);
					return -1;
				}
				bFindMP4NameFlag = true;
				memset(szMP4Name, 0x00, sizeof(szMP4Name));
				memcpy(szMP4Name, netDataCache + nPos1 + 4, nPos2 - nPos1 - 4);

				WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X ,nClient = %llu ,拷贝出 mp4 文件名字 %s ", this, nClient, szMP4Name);
			}
			else
				return -1;
		}
		else
			return -1;

		if (!bFindMP4NameFlag)
		{
			WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, 检测出 非法的 Http-mp4 协议数据包 nClient = %llu ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
		WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, setup -1  nClient = %llu ", this, nClient);

		char szOrigin[string_length_4096] = { 0 };
		mp4Parse.ParseSipString((char*)netDataCache);
		mp4Parse.GetFieldValue("Origin", szOrigin);
		if (strlen(szOrigin) == 0)
			strcpy(szOrigin, "*");
		WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, setup -2  nClient = %llu ", this, nClient);

		//去掉？后面参数字符串
		string strMP4Name = szMP4Name;
		int    nPos = strMP4Name.find("?", 0);
		if (nPos > 0 && nPos != string::npos && strlen(szMP4Name) > 0)
		{
			if (strlen(szPlayParams) == 0)//拷贝鉴权参数
				memcpy(szPlayParams, szMP4Name + (nPos + 1), strlen(szMP4Name) - nPos - 1);
			 szMP4Name[nPos] = 0x00 ;
  		}
		WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, setup -3  nClient = %llu ", this, nClient);

		//根据mp4文件，进行查找推流对象 
		if (strMP4Name.find("?download_speed=",0) == string::npos && strMP4Name.find("__ReplayFMP4RecordFile__", 0) == string::npos)
		{
			httpMp4Type = HttpMp4Type_Play;
		}
		else
		{
 			int nPos = strMP4Name.find("?download_speed=", 0);
			if (nPos != string::npos)
			{//去掉 ?后面
 				strcpy(szMP4Name, strMP4Name.c_str());
				char szDownLoadSpeed[string_length_4096] = { 0 };
				memcpy(szDownLoadSpeed, szMP4Name + nPos + strlen("?download_speed="), strlen(szMP4Name) - nPos - strlen("?download_speed="));
				szMP4Name[nPos] = 0x00;
				nHttpDownloadSpeed = atoi(szDownLoadSpeed);
			}
			httpMp4Type = HttpMp4Type_Download;
 		}
 
		//根据mp4文件，进行简单判断是否合法
		if (!(strstr(szMP4Name, ".mp4") != NULL || strstr(szMP4Name, ".MP4") != NULL))
		{
			WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X,  nClient = %llu ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
		WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, setup -4  nClient = %llu ", this, nClient);

		if (strstr(szMP4Name, ".mp4") != NULL || strstr(szMP4Name, ".MP4") != NULL)
			szMP4Name[strlen(szMP4Name) - 4] = 0x00;

	#ifdef USE_BOOST
			boost::shared_ptr<CMediaStreamSource> pushClient = NULL ;
#else
		std::shared_ptr<CMediaStreamSource> pushClient=NULL;
#endif
		strcpy(szMediaSourceURL, szMP4Name);
		if (httpMp4Type == HttpMp4Type_Play)
		{
			 pushClient = GetMediaStreamSource(szMP4Name, true);

			if (pushClient == NULL)
			{
 				return  ResponseError("没有推流对象的地址");
			}
		}
		else
		{//录像点播
			if (strstr(szMP4Name, RecordFileReplaySplitter) != NULL)
			{//单个录像文件下载 
 				if (QueryRecordFileIsExiting(szMediaSourceURL) == false)
  					return ResponseError("没有录像文件");

#ifdef OS_System_Windows
				sprintf(szRequestReplayRecordFile, "%s%s\\%s\\%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#else
				sprintf(szRequestReplayRecordFile, "%s%s/%s/%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#endif				 
				mutliRecordPlayNameList.push_back(szRequestReplayRecordFile);
			}
			else
			{//多个录像文件下载 
				pushClient = GetMediaStreamSource(szMP4Name);

				if (pushClient == NULL)
				{
					return  ResponseError("没有推流对象的地址");
				}

				auto  pRecordMediaSource = GetNetRevcBaseClient(pushClient->nClient);
				if (pRecordMediaSource != NULL && pRecordMediaSource->mutliRecordPlayNameList.size() > 0)
				{//拷贝多个文件
					for(int i=0;i<pRecordMediaSource->mutliRecordPlayNameList.size();i++)
					  mutliRecordPlayNameList.push_back(pRecordMediaSource->mutliRecordPlayNameList[i]);
				}else 
					return  ResponseError("没有推流对象的地址");
			}
 
			//发送播放事件通知，用于播放鉴权
			if (ABL_MediaServerPort.hook_enable == 1 && bOn_playFlag == false && mutliRecordPlayNameList.size() >  0 )
			{
				bOn_playFlag = true;
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = NetBaseNetType_HttpClient_on_play;
				sprintf(msgNotice.szMsg, "{\"eventName\":\"on_play\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu,\"ip\":\"%s\" ,\"port\":%d,\"params\":\"%s\"}", szSplliterApp, szSplliterStream, ABL_MediaServerPort.mediaServerID, netBaseNetType, nClient, szClientIP, nClientPort, szPlayParams);
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			}

			//录像下载
			if (httpMp4Type == HttpMp4Type_Download)
			{
				bCheckHttpMP4Flag = true;
				nCurrentRecordFileOrder = 0;

				fFileMp4 = fopen(mutliRecordPlayNameList[nCurrentRecordFileOrder].c_str(), "rb");
				if(fFileMp4 == NULL)
					return ResponseError("读取下载文件失败");
			    
				nRecordFileSize =  0 ;
#ifdef OS_System_Windows
				for (int i = 0; i < mutliRecordPlayNameList.size(); i++)
				{//统计所有文件的大小 
					struct _stat64 fileBuf;
					int error = _stat64(mutliRecordPlayNameList[i].c_str(), &fileBuf);
					if (error == 0)
						nRecordFileSize += fileBuf.st_size;
				}
#else 
				for (int i = 0; i < mutliRecordPlayNameList.size(); i++)
				{
			     	struct stat fileBuf;
				    int error = stat(mutliRecordPlayNameList[i].c_str(), &fileBuf);
				    if (error == 0)
					  nRecordFileSize += fileBuf.st_size;
 				}
#endif
				sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: close\r\nContent-Type: video/mp4\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=15\r\nContent-Length: %d\r\nServer: %s\r\nContent-Disposition: attachment;\r\n\r\n", szOrigin, nRecordFileSize, MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

				RecordReplayThreadPool->InsertIntoTask(nClient); //投递任务
				return 0;
			}

			//创建录像文件点播
			pushClient = CreateReplayClient(szMediaSourceURL, &nReplayClient);
			if (pushClient == NULL)
			{
 				return ResponseError("创建录像点播失败");
			}
		}
		WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, setup -5  nClient = %llu ", this, nClient);

		//记下媒体源
		SplitterAppStream(szMP4Name);
		sprintf(m_addStreamProxyStruct.url, "http://%s:%d/%s/%s.mp4", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);

		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: keep-alive\r\nContent-Type: video/mp4; charset=utf-8\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin, MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
		if (nWriteRet != 0)
		{
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		pMP4Buffer = new unsigned char[MediaStreamSource_VideoFifoLength];

		m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
		m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);
 
		//把客户端 加入源流媒体拷贝队列 
		pushClient->AddClientToMap(nClient);

		bCheckHttpMP4Flag = true;
	}
	else
	{
		if (httpMp4Type == HttpMp4Type_Download && bCheckHttpMP4Flag == true && fFileMp4 != NULL)
		{//录像下载
 			if ( (GetTickCount64() - nCreateDateTime) >= (200 / nHttpDownloadSpeed) )
			{
				nCreateDateTime = GetTickCount64();

				nReadLength = fread(pFmp4SPSPPSBuffer, 1, Send_DownloadFile_MaxPacketCount, fFileMp4);
				if (nReadLength > 0)
				{
					nWriteRet = XHNetSDK_Write(nClient, pFmp4SPSPPSBuffer, nReadLength, 1);
					if (nWriteRet != 0)
					{
						WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, 下载录像时，文件发送失败 %s nClient = %llu ", this, szMP4Name, nClient);
						DeleteNetRevcBaseClient(nClient);
						return -1;
					}
				}
				else
				{
					nCurrentRecordFileOrder ++;
					if (nCurrentRecordFileOrder >= mutliRecordPlayNameList.size())
					{
 						WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, 录像文件下载完毕 %s nClient = %llu , nCurrentRecordFileOrder = %d , mutliRecordPlayNameList.size() = %d ", this, szMP4Name, nClient, nCurrentRecordFileOrder, mutliRecordPlayNameList.size());
						DeleteNetRevcBaseClient(nClient);
						return -1;
					}
					else
					{//读取下一个文件
						if (fFileMp4 != NULL)
						{
							fclose(fFileMp4);
							fFileMp4 = fopen(mutliRecordPlayNameList[nCurrentRecordFileOrder].c_str(), "rb");
							if (fFileMp4 == NULL)
							{//读取文件失败 
								WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X,  nClient = %llu , 下载文件失败  %s ", this,  nClient, mutliRecordPlayNameList[nCurrentRecordFileOrder].c_str());
								DeleteNetRevcBaseClient(nClient);
								return -1;
							}
							else
							{
								WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X,  nClient = %llu , 正在下载文件 %d / %d  %s ", this, nClient, nCurrentRecordFileOrder + 1, mutliRecordPlayNameList.size(),mutliRecordPlayNameList[nCurrentRecordFileOrder].c_str());
								RecordReplayThreadPool->InsertIntoTask(nClient); //投递任务
							}
						}
					}
				}
			}
			else
			{
				if (nHttpDownloadSpeed != 10)
				{
				 //  Sleep(1);
				   std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
 			}
  		   RecordReplayThreadPool->InsertIntoTask(nClient); //投递任务
		}
    }

	return 0;
}

//回复错误信息
bool   CNetServerHTTP_MP4::ResponseError(char* szErrorMsg)
{
	WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X, %s %s nClient = %llu ", this, szErrorMsg,szMP4Name, nClient);

	sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
	nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

	DeleteNetRevcBaseClient(nClient);
	return true ;

}

//发送第一个请求
int CNetServerHTTP_MP4::SendFirstRequst()
{

	 return 0;
}

//请求m3u8文件
bool  CNetServerHTTP_MP4::RequestM3u8File()
{
	return true;
}

static int fmp4_hls_init_segment(hls_fmp4_t* hls, void* param)
{
	CNetServerHTTP_MP4* pNetServerHttpMp4 = (CNetServerHTTP_MP4*)param;
	if (pNetServerHttpMp4 == NULL)
		return 0;

	int bytes = hls_fmp4_init_segment(hls, pNetServerHttpMp4->s_packet, sizeof(pNetServerHttpMp4->s_packet));

	pNetServerHttpMp4->fTSFileWriteByteCount = pNetServerHttpMp4->nFmp4SPSPPSLength = bytes;
	if (bytes > 0 && bytes < 1024 * 128)
	{
		memcpy(pNetServerHttpMp4->pFmp4SPSPPSBuffer, pNetServerHttpMp4->s_packet, bytes);

		pNetServerHttpMp4->nWriteRet = XHNetSDK_Write(pNetServerHttpMp4->nClient, pNetServerHttpMp4->s_packet, bytes, 1);
		if (pNetServerHttpMp4->nWriteRet != 0)
		{
			WriteLog(Log_Debug, "fmp4_hls_segment = %X 发送出错，准备删除 nClient = %llu ", pNetServerHttpMp4, pNetServerHttpMp4->nClient);
			DeleteNetRevcBaseClient(pNetServerHttpMp4->nClient);
			return 0 ;
		}
	}
#ifdef WriteMp4BufferToFile
	if (pNetServerHttpMp4->fWriteMP4)
	{
		fwrite(pNetServerHttpMp4->s_packet, 1, bytes, pNetServerHttpMp4->fWriteMP4);
		fflush(pNetServerHttpMp4->fWriteMP4);
		pNetServerHttpMp4->s_packetLength = bytes;
	}
#endif
	//必须hls_init_segment 初始化完成才能写视频、音频段，在回调函数里面做标志
	pNetServerHttpMp4->hls_init_segmentFlag = true;

	return 0;
}

bool  CNetServerHTTP_MP4::VideoFrameToFMP4File(unsigned char* szVideoData, int nLength)
{
	if (track_video < 0 && hlsFMP4 != NULL )
	{
		int n;
		//vcl 、 update 都要赋值为 0 ，否则容易崩溃 
		vcl = 0;
		update = 0;        
		if (memcmp(mediaCodecInfo.szVideoName, "H264", 4) == 0)
		{
			n = h264_annexbtomp4(&avc, szVideoData, nLength, pMP4Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		}
		else if (memcmp(mediaCodecInfo.szVideoName, "H265", 4) == 0)
		{
			n = h265_annexbtomp4(&hevc, szVideoData, nLength, pMP4Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		}
		else
			return false;

		if (track_video < 0)
		{
			memset(szExtenVideoData, 0x00, sizeof(szExtenVideoData));
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			{//H264 等待 SPS、PPS 的方法 
				if (avc.nb_sps < 1 || avc.nb_pps < 1)
				{
 					return false  ; // waiting for sps/pps
				}
				extra_data_size = mpeg4_avc_decoder_configuration_record_save(&avc, szExtenVideoData, sizeof(szExtenVideoData));
			}
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			{//H265 等待SPS、PPS的方法 
				if (hevc.numOfArrays < 1)
				{
 					return false ; // waiting for vps/sps/pps
				}
			    extra_data_size = mpeg4_hevc_decoder_configuration_record_save(&hevc, szExtenVideoData, sizeof(szExtenVideoData));
			}
			else
				return false;

			if (extra_data_size <= 0)
			{
				return false;
			}

			if (extra_data_size > 0)
			{
				if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				  track_video = hls_fmp4_add_video(hlsFMP4, MOV_OBJECT_H264, mediaCodecInfo.nWidth, mediaCodecInfo.nHeight, szExtenVideoData, extra_data_size);
				else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				  track_video = hls_fmp4_add_video(hlsFMP4, MOV_OBJECT_HEVC, mediaCodecInfo.nWidth, mediaCodecInfo.nHeight, szExtenVideoData, extra_data_size);
			}
		}
	}

	if (track_video >= 0 && hlsFMP4 != NULL )
	{
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, szVideoData, nLength) == true)
		{
			if (!bWaitIFrameSuccessFlag)
				bWaitIFrameSuccessFlag = true;
			flags = 1;
		}
		else
			flags = 0;

		if (!bWaitIFrameSuccessFlag)
			return false ;

		vcl = 0;
		update = 0;        

		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			nMp4BufferLength = h264_annexbtomp4(&avc, szVideoData, nLength, pMP4Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			nMp4BufferLength = h265_annexbtomp4(&hevc, szVideoData, nLength, pMP4Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		else
			return false;

		//有音频轨道 ，或者 等待视频超过30帧时，还没产生音频轨道证明该码流没有音频 
		if (nMp4BufferLength > 0 && (ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName,"AAC") != 0 || (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0  && track_aac >= 0 )))
		{
 			if (hls_init_segmentFlag == false)
			{
				fmp4_hls_init_segment(hlsFMP4, this);
			}

			//必须hls_init_segment 初始化完成才能写视频段，在回调函数里面做标志 
			if (hls_init_segmentFlag == true)
				hls_fmp4_input(hlsFMP4, track_video, pMP4Buffer, nMp4BufferLength, videoDts, videoDts, (flags == 1) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
	}

	return true;
}

bool  CNetServerHTTP_MP4::SendTSBufferData(unsigned char* pTSData, int nLength)
{
	if ( bCheckHttpMP4Flag == false || pTSData == NULL || nLength <= 0 )
		return false;
	 
	//发送MP4码流 
	nSendErrorCount = 0;
	nPos = 0;
	while (nLength > 0 && pTSData != NULL)
	{
		if (nLength > Send_MP4File_MaxPacketCount)
		{
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)pTSData + nPos, Send_MP4File_MaxPacketCount, 1);
			nLength -= Send_MP4File_MaxPacketCount;
			nPos += Send_MP4File_MaxPacketCount;
		}
		else
		{
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)pTSData + nPos, nLength, 1);
			nPos += nLength;
			nLength = 0;
		}

		if (nWriteRet != 0)
		{//发送出错
			nSendErrorCount ++;
			if (nSendErrorCount >= 5)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_MP4 = %X nClient = %llu 发送次数超过 %d 次 ，准备删除 ", this, nClient, nSendErrorCount);
				DeleteNetRevcBaseClient(nClient);
				return -1;
			}
		}
		else
			nSendErrorCount = 0;
	}
	return true;
}
