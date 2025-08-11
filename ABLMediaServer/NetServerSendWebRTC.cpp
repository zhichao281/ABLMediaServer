///*
//功能：
// 	 实现whep方式的webrtc播放 　
//
//日期    2025-06-12
//作者    罗家兄弟
//QQ      79941308
//E-Mail  79941308@qq.com
//*/
//
//#include "stdafx.h"
//#include "NetServerSendWebRTC.h"
//#include <random>
//
//extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
//extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
//extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
//extern bool                                  DeleteMediaStreamSource(char* szURL);
//extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
//extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
//
//extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
//extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
//extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
//extern MediaServerPort                       ABL_MediaServerPort;
//extern CNetBaseThreadPool*                   MessageSendThreadPool;//消息发送线程池
//extern CMediaFifo                            pMessageNoticeFifo; //消息通知FIFO
//extern char                                  ABL_MediaSeverRunPath[256];
//
//extern void LIBNET_CALLMETHOD	             onconnect(NETHANDLE clihandle,uint8_t result, uint16_t nLocalPort);
//extern void LIBNET_CALLMETHOD                onread(NETHANDLE srvhandle,NETHANDLE clihandle,uint8_t* data,uint32_t datasize,void* address);
//extern void LIBNET_CALLMETHOD	             onclose(NETHANDLE srvhandle,NETHANDLE clihandle);
//SSL_CTX*  CNetServerSendWebRTC::sm_dtlsCtx   = nullptr;
//int	      CNetServerSendWebRTC::m_nVideoSSRC = 8235;
//
////rtp打包回调视频
//void WebRTC_Video_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
//{
//	CNetServerSendWebRTC* pWebRtcClient = (CNetServerSendWebRTC*)cb->userdata;
//	
//	if(pWebRtcClient->bRunFlag.load() && pWebRtcClient->m_srtp != NULL )
//	{
//#ifdef  WriteRtpPacketFlag
//      if(pWebRtcClient->writeRtpFile)
//	  {
//		pWebRtcClient->rtspHead.head = '$';
//		pWebRtcClient->rtspHead.chan = 0 ;
//		pWebRtcClient->rtspHead.Length = htons(cb->datasize);
//		fwrite((unsigned char*)&pWebRtcClient->rtspHead,1,sizeof(pWebRtcClient->rtspHead),pWebRtcClient->writeRtpFile);
//		fwrite((unsigned char*)cb->data,1,cb->datasize,pWebRtcClient->writeRtpFile);
//	  }
//#endif 			
//		
//		memcpy(pWebRtcClient->srtpData,cb->data,cb->datasize);
//        pWebRtcClient->srtLength = cb->datasize  ;
//   
//		int32_t r = srtp_protect(pWebRtcClient->m_srtp, (void*)pWebRtcClient->srtpData, &pWebRtcClient->srtLength);
//
// 		if (srtp_err_status_t::srtp_err_status_ok == r && pWebRtcClient->srtLength > 0)
//		{
//			if (static_cast<gint>(pWebRtcClient->srtLength) != nice_agent_send(pWebRtcClient->m_iceAgent, pWebRtcClient->m_vStreamId, 1, static_cast<guint>(pWebRtcClient->srtLength),(const gchar*)pWebRtcClient->srtpData))
//			{
//				WriteLog(Log_Debug, "nice_agent_send error ,pullhandle=%llu", pWebRtcClient->nClient);
//				return ;
//			}
//		}
//		else
//		{
// 			WriteLog(Log_Debug, "srtp_protect error.ec:%d ,pullhandle=%llu",  r, pWebRtcClient->nClient);
//		}   
//	   // WriteLog(Log_Debug, "WebRTC_Video_rtp_packet_callback_func_send 回调 nClient = %llu ,Length =%d, r= %d , srtLength = %d ", pWebRtcClient->nClient,cb->datasize,r,pWebRtcClient->srtLength);
//	}
//}
//
//CNetServerSendWebRTC::CNetServerSendWebRTC(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
//{
//#ifdef  WriteRtpPacketFlag
//    writeRtpFile = fopen("./sendVideo.rtp","wb") ;
//#endif   	
//#ifdef WriteVidedDataFlag
//    writeVideoFile = fopen("./sendVideo.data","wb");
//#endif  	
//    nVdeoFrameNumber  = 0 ;
//    nAudioFrameNumber = 0 ;
//
//	hRtpVideo = 0;
//	hRtpAudio = 0;	
//	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
//	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);
//	
//	m_srtp = nullptr;
//	m_ssl = nullptr;
//	m_inBio = nullptr;
//	m_outBio = nullptr;
//	m_localUfrag = generateUfrag();
//	m_localPwd = generatePwd();
//	m_localCandidateFlag = 0;
//	m_vStreamId = 0;
//	m_aStreamId = 0;
//    m_nVideoSSRC ++ ;
//	m_nAudioRate = 8000;
//	m_nAudioChannel = 1;
//	
//	m_iceAgent = NULL ;
//	nHttpHeadEndLength = 0;
//	nWebRTC_Comm_State = WebRTC_Comm_State_ConnectSuccess;
//	nCreateDateTime = GetTickCount64();
//	strcpy(m_szShareMediaURL,szShareMediaURL);
// 	netBaseNetType = NetBaseNetType_WebRtcServerWhepPlayer;
//	nServer = hServer;
//	nClient = hClient;
//	strcpy(szClientIP, szIP);
//	nClientPort = nPort;
//	nNetStart = nNetEnd = netDataCacheLength = 0;
//	bRunFlag = true;
//
//	nMediaClient = 0;
//	WriteLog(Log_Debug, "CNetServerSendWebRTC 构造 = %X nClient = %llu ", this, nClient );
//}
//
//std::string CNetServerSendWebRTC::generateUfrag()
//{
//	thread_local std::default_random_engine e(time(0));
//	std::uniform_int_distribution<int> dist(97, 122);
//
//	char str[5] = { 0 };
//
//	for (int32_t i = 0; i < 4; ++i)
//	{
//		str[i] = (char)dist(e);
//	}
//
//	str[4] = '\0';
//
//	return str;
//}
//
//std::string CNetServerSendWebRTC::generatePwd()
//{
//	thread_local std::default_random_engine e(time(0));
//	std::uniform_int_distribution<int> dist(97, 122);
//
//	char str[23] = { 0 };
//
//	for (int32_t i = 0; i < 22; ++i)
//	{
//		str[i] = (char)dist(e);
//	}
//
//	str[22] = '\0';
//
//	return str;
//}
//
//CNetServerSendWebRTC::~CNetServerSendWebRTC()
//{
//	bRunFlag = false;
//	std::lock_guard<std::mutex> lock(NetClientHTTPLock);
//
//	if (hRtpVideo != 0)
//	{
//		rtp_packet_stop(hRtpVideo);
//		hRtpVideo = 0;
//	}
//	if (hRtpAudio != 0)
//	{
//		rtp_packet_stop(hRtpAudio);
//		hRtpAudio = 0;
//	}
//	m_videoFifo.FreeFifo();
//	m_audioFifo.FreeFifo();
//	
//	cleanICE();
//	cleanDtls();
//	cleanSrtp();
//#ifdef  WriteRtpPacketFlag
//    fclose(writeRtpFile ) ;
//#endif 	
//#ifdef WriteVidedDataFlag
//    fclose(writeVideoFile);
//#endif  	
//  	WriteLog(Log_Debug, "CNetServerSendWebRTC 析构 = %X nClient = %llu , nMediaClient = %llu ", this, nClient, nMediaClient);
//	malloc_trim(0);
//}
//
//int CNetServerSendWebRTC::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
//{
//	//WriteLog(Log_Debug, "PushVideo = %X ，视频资源 ,nClient = %llu，szVideoCodec = %s, nDataLength = %d ", this, nClient,szVideoCodec,nDataLength);
//	
//	if (strlen(mediaCodecInfo.szVideoName) == 0)
//		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);
//
//	if (!bRunFlag || nWebRTC_Comm_State != WebRTC_Comm_State_StartPlay)
//		return -1;
//	
// #ifdef WriteVidedDataFlag
//   if(writeVideoFile)
//  	   fwrite(pVideoData,1,nDataLength,writeVideoFile);
//#endif 		
//	if(hRtpVideo == 0)
//	{
//		memset((char*)&optionVideo, 0x00, sizeof(optionVideo));
//		if (strcmp(mediaCodecInfo.szVideoName,"H264") == 0)
//		{
//			optionVideo.streamtype = e_rtppkt_st_h264;
//   		}
//		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
//		{
//			optionVideo.streamtype = e_rtppkt_st_h265;
//   		}
//		else if (strcmp(mediaCodecInfo.szVideoName, "") == 0)
//		{
//			WriteLog(Log_Debug, "CNetServerSendWebRTC = %X ，没有视频资源 ,nClient = %llu", this, nClient);
//		}
//
//		int nRet = 0;
//		if (strlen(mediaCodecInfo.szVideoName) > 0)
//		{
//			nRet = rtp_packet_start(WebRTC_Video_rtp_packet_callback_func_send, (void*)this, &hRtpVideo);
//			if (nRet != e_rtppkt_err_noerror)
//			{
//				WriteLog(Log_Debug, "CNetServerSendWebRTC = %X ，创建视频rtp打包失败,nClient = %llu,  nRet = %d", this,nClient, nRet);
//				return false;
//			}
//			optionVideo.handle = hRtpVideo;
//			optionVideo.ssrc = m_nVideoSSRC ;
//			optionVideo.mediatype = e_rtppkt_mt_video;
//			optionVideo.payload = nVideoPayload;
//			optionVideo.ttincre = 90000 / mediaCodecInfo.nVideoFrameRate; //视频时间戳增量
//
//			inputVideo.handle = hRtpVideo;
//			inputVideo.ssrc = optionVideo.ssrc;
//
//			nRet = rtp_packet_setsessionopt(&optionVideo);
//			if (nRet != e_rtppkt_err_noerror)
//			{
//				WriteLog(Log_Debug, "CNetServerSendWebRTC = %X ，rtp_packet_setsessionopt 设置视频rtp打包失败 ,nClient = %llu nRet = %d", this, nClient, nRet);
//				return false;
//			}
//			
//	       WriteLog(Log_Debug, "CNetServerSendWebRTC = %X ，rtp_packet_setsessionopt 设置视频rtp打包成功 ,nClient = %llu nRet = %d， m_nVideoSSRC = %d , nVideoFrameRate = %d ", this, nClient, nRet,optionVideo.ssrc,mediaCodecInfo.nVideoFrameRate);		
//		}	
//	}
//	std::lock_guard<std::mutex> lock(businessProcMutex);
//
//	m_videoFifo.push(pVideoData, nDataLength);
//	
//    return 0;
//}
//
//int CNetServerSendWebRTC::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
//{
//	if (!bRunFlag || nWebRTC_Comm_State != WebRTC_Comm_State_StartPlay)
//		return -1;
//	
//	return 0;
//}
//
//int CNetServerSendWebRTC::SendVideo()
//{
//  	nVideoStampAdd = 90000 / mediaCodecInfo.nVideoFrameRate;
//
//	unsigned char* pData = NULL;
//	int            nLength = 0;
//	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
//	{
// 		inputVideo.data = pData;
//		inputVideo.datasize = nLength;
// 
//		if (nMediaSourceType == MediaSourceType_ReplayMedia)
//		{//如果是录像回放，前面4个字节是视频帧序号，根据这个帧序号生成时间戳直接送入rtp打包
//			inputVideo.data = pData + 4;
//			inputVideo.datasize = nLength - 4;
//			memcpy((char*)&nVdeoFrameNumber, pData, sizeof(uint32_t));
//			inputVideo.timestamp = nVdeoFrameNumber * nVideoStampAdd;
//		}
//		else
//		{//实时流
//			nVdeoFrameNumber ++;
//			inputVideo.timestamp = nVdeoFrameNumber * nVideoStampAdd;
//		}
//		
//		rtp_packet_input(&inputVideo);
//		m_videoFifo.pop_front();
//	}
//
//	return 0;
//}
//
//int CNetServerSendWebRTC::SendAudio()
//{
//
//	return 0;
//}
//
//int CNetServerSendWebRTC::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
//{
//	if (!bRunFlag)
//		return -1;
//	std::lock_guard<std::mutex> lock(NetClientHTTPLock);
//
//	if (MaxNetClientHttpBuffer - nNetEnd >= nDataLength)
//	{//剩余空间足够
//		memcpy(netDataCache + nNetEnd, pData, nDataLength);
//		netDataCacheLength += nDataLength;
//		nNetEnd += nDataLength;
//	}
//	else
//	{//剩余空间不够，需要把剩余的buffer往前移动
//		if (netDataCacheLength > 0)
//		{//如果有少量剩余
//			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
//			nNetStart = 0;
//			nNetEnd = netDataCacheLength;
//
//			//把空余的buffer清空 
//			memset(netDataCache + nNetEnd, 0x00, MaxNetClientHttpBuffer - nNetEnd);
//
//			if (MaxNetClientHttpBuffer - nNetEnd < nDataLength)
//			{
//				nNetStart = nNetEnd = netDataCacheLength = 0;
//				memset(netDataCache, 0x00, MaxNetClientHttpBuffer);
//				WriteLog(Log_Debug, "CNetServerSendWebRTC = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
//				deleteWebRTC();
//				return 0;
//			}
//		}
//		else
//		{//没有剩余，那么 首，尾指针都要复位 
//			nNetStart = nNetEnd = netDataCacheLength = 0;
//			memset(netDataCache, 0x00, MaxNetClientHttpBuffer);
//		}
//		memcpy(netDataCache + nNetEnd, pData, nDataLength);
//		netDataCacheLength += nDataLength;
//		nNetEnd += nDataLength;
//	}
//	return 0;
//}
//
////检测http头是否接收完整
//int   CNetServerSendWebRTC::CheckHttpHeadEnd()
//{
//	int       nHttpHeadEndPos = -1;
//	unsigned char szHttpHeadEnd[4] = { 0x0d,0x0a,0x0d,0x0a };
//	for (int i = nNetStart; i < netDataCacheLength; i++)
//	{
//		if (memcmp(netDataCache + i, szHttpHeadEnd, 4) == 0)
//		{
//			nHttpHeadEndPos = i;
//			nHttpHeadEndLength = i + 4;
//			break;
//		}
//	}
//	return nHttpHeadEndPos;
//}
//
//int CNetServerSendWebRTC::ProcessNetData()
//{
//	if (!bRunFlag)
//		return -1;
//	std::lock_guard<std::mutex> lock(NetClientHTTPLock);
//
//	int nHttpHeadEndPos = CheckHttpHeadEnd();
// 	netDataCache[netDataCacheLength] = 0x00;
//
//	//确保接收数据完整
//	if (nHttpHeadEndPos < 0)
//		return -1;
// 
// 	if (!(memcmp(netDataCache, "GET ", 4) == 0 || memcmp(netDataCache, "OPTIONS ", 8) == 0 || memcmp(netDataCache, "POST ", 5) == 0 || memcmp(netDataCache, "DELETE ", 7) == 0 || memcmp(netDataCache, "PATCH ", 6) == 0))
//	{
//		WriteLog(Log_Debug, "CNetServerSendWebRTC = %X , nClient = %llu , 接收的数据非法 \r\n%s", this, nClient,netDataCache);
//        deleteWebRTC();
//		return -1;
//	}
//	
//	memcpy(szHttpHead, netDataCache, nHttpHeadEndLength);
//	szHttpHead[nHttpHeadEndLength] = 0x00;
//	httpParse.ParseSipString(szHttpHead);
//	memset(szContentLength, 0x00, sizeof(szContentLength));
//	httpParse.GetFieldValue("Content-Length", szContentLength);
//	nContent_Length = atoi(szContentLength);
//
//	if (netDataCacheLength >= (nHttpHeadEndLength + nContent_Length))
//	{
//	  DeleteAllHttpKeyValue();
//	  WriteLog(Log_Debug, "CNetServerSendWebRTC = %X nClient = %llu , Recv \r\n%s", this, nClient, netDataCache);
//	
//      if(nContent_Length > 0 && nHttpHeadEndLength > 0 )
//	  {
//        memcpy(szHttpBody,netDataCache + nHttpHeadEndLength,nContent_Length);
//		szHttpBody[nContent_Length]=0x00;
//         //WriteLog(Log_Debug, "CNetServerSendWebRTC = %X , nClient = %llu , szHttpBody \r\n%s", this, nClient,szHttpBody);
//	  }
//		
//	  if ( memcmp(netDataCache, "GET ", 4) == 0)
//	  {
//	     ResponseGetRqeuset();
//	  }else if ( memcmp(netDataCache, "OPTIONS ", 8) == 0)
//	  {//回复OPTIONS
//	  	 ResponseOPTIONS();
//	  }
//	  else if (memcmp(netDataCache, "POST ",5) == 0)
//	  {//回复POST
//		  ResponsePost();
//	  }else if (memcmp(netDataCache, "DELETE ",7) == 0)
//	  {//浏览器发送删除命令
//		  deleteWebRTC();
//	  }
//
//	  //重新复位
//	  if (netDataCacheLength > (nHttpHeadEndLength + atoi(szContentLength)))
//	  {//把下一包数据移动到头部
//		  memmove(netDataCache, netDataCache + (nHttpHeadEndLength + nContent_Length), netDataCacheLength - (nHttpHeadEndLength + nContent_Length));
//		  netDataCacheLength = netDataCacheLength - (nHttpHeadEndLength + nContent_Length);
//	  }
//	  else
//	  {
//		  nNetStart = nNetEnd = netDataCacheLength = 0;
//		  memset(netDataCache, 0x00, MaxNetClientHttpBuffer);
//	  }
//	}
//
//	return 0;
//}
//
////发送第一个请求
//int CNetServerSendWebRTC::SendFirstRequst()
//{
//
// 	return 0;
//}
//
////请求m3u8文件
//bool  CNetServerSendWebRTC::RequestM3u8File()
//{
//	return true;
//}
//
////删除所有参数
//void CNetServerSendWebRTC::DeleteAllHttpKeyValue()
//{
//	RequestKeyValue* pDel;
//	RequestKeyValueMap::iterator it;
//
//	for (it = requestKeyValueMap.begin(); it != requestKeyValueMap.end();)
//	{
//		pDel = (*it).second;
//		delete pDel;
//		requestKeyValueMap.erase(it++);
//	}
//}
//
//bool  CNetServerSendWebRTC::GetKeyValue(char* key, char* value)
//{
//	RequestKeyValue* pFind;
//	RequestKeyValueMap::iterator it;
//
//	it = requestKeyValueMap.find(key);
//	if (it != requestKeyValueMap.end())
//	{
//		pFind = (*it).second;
//		strcpy(value, pFind->value);
//		return true;
//	}
//	else
//		return false;
//}
//
////对get 方式的参数进行切割
//bool CNetServerSendWebRTC::SplitterTextParam(char* szTextParam)
//{//id=1000&name=luoshenzhen&address=广东深圳&age=45
//	string strTextParam = szTextParam;
//	int    nPos1 = 0, nPos2 = 0;
//	int    nFind1, nFind2;
//	int    nKeyCount = 0;
//	char   szValue[string_length_2048] = { 0 };
//	char   szKey[string_length_2048] = { 0 };
//	char   szBlockString[string_length_8192] = { 0 };
//
//	DeleteAllHttpKeyValue();
//	string strValue;
//	string strKey;
//	string strBlockString;
//	bool   breakFlag = false;
//
//	while (true)
//	{
//		nFind1 = strTextParam.find("&", nPos1);
//		memset(szBlockString, 0x00, sizeof(szBlockString));
//		if (nFind1 > 0)
//		{
//			if (nFind1 - nPos1 > 0 && nFind1 - nPos1 < string_length_8192)
//			{
//				memcpy(szBlockString, szTextParam + nPos1, nFind1 - nPos1);
//				strBlockString = szBlockString;
//			}
//			nPos1 = nFind1 + 1;
//		}
//		else
//		{
//			if ((strlen(szTextParam) - nPos1) > 0 && (strlen(szTextParam) - nPos1) < string_length_8192)
//			{
//				memcpy(szBlockString, szTextParam + nPos1, strlen(szTextParam) - nPos1);
//				strBlockString = szBlockString;
//			}
//			breakFlag = true;
//		}
//
//		if (strlen(szBlockString) > 0)
//		{//找出字段
//			replace_all(strBlockString, "%20", "");//去掉空格
//			replace_all(strBlockString, "%26", "&");
//			memset(szBlockString, 0x00, sizeof(szBlockString));
//			strcpy(szBlockString, strBlockString.c_str());
//
//			RequestKeyValue* keyValue = NULL;
//			while (keyValue == NULL)
//				keyValue = new RequestKeyValue();
//			nFind2 = strBlockString.find("=", 0);
//			if (nFind2 > 0)
//			{
//				memcpy(keyValue->key, szBlockString, nFind2);
//				memcpy(keyValue->value, szBlockString + nFind2 + 1, strBlockString.size() - nFind2 - 1);
//			}
//			else
//			{
//				if (strlen(szBlockString) < 512)
//					strcpy(keyValue->key, szBlockString);
//			}
//			requestKeyValueMap.insert(RequestKeyValueMap::value_type(keyValue->key, keyValue));
//			nKeyCount++;
//			WriteLog(Log_Debug, "CNetServerSendWebRTC = %X  nClient = %llu 获取webrtc播放请求参数  %s = %s", this, nClient, keyValue->key, keyValue->value);
//		}
//		if (breakFlag)
//			break;
//	}
//
// 	return nKeyCount > 0 ? true : false;
//}
//
//
////响应第一个Get请求
//bool   CNetServerSendWebRTC::ResponseGetRqeuset()
//{
//	strcpy(szHttpBody, "HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Max-Age: 86440\r\nAccess-Control-Allow-Headers: Content-type,Access-Token\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET\r\nServer: XH_HTTP_SERVER\r\n\r\n");
//	int nRet = XHNetSDK_Write(nClient, (unsigned char*)szHttpBody, strlen(szHttpBody), ABL_MediaServerPort.nSyncWritePacket);
//	nWebRTC_Comm_State = WebRTC_Comm_State_ResponseGet;
//	WriteLog(Log_Debug, "CNetServerSendWebRTC = %X , nClient = %llu , ResponseGetRqeuset()\r\n%s", this, nClient,szHttpBody);
//	return true;
//}
//
//bool  CNetServerSendWebRTC::ResponseWebpage()
//{
//#ifdef OS_System_Windows
// 	sprintf(webrtcPlayerFile, "%swebrtc\\%s", ABL_MediaSeverRunPath, "webrtc.html");
//	struct _stat64 fileBuf;
//	int error = _stat64(webrtcPlayerFile, &fileBuf);
//	if (error == 0)
//		webrtcPlayerFileLength = fileBuf.st_size;
// #else 
//	sprintf(webrtcPlayerFile, "%swebrtc/%s", ABL_MediaSeverRunPath, "webrtc.html");
//
//	struct stat fileBuf;
//	int error = stat(webrtcPlayerFile, &fileBuf);
//	if (error == 0)
//		webrtcPlayerFileLength = fileBuf.st_size;
//#endif	
//	FILE* fWebRtc = fopen(webrtcPlayerFile, "rb");
//	if (fWebRtc)
//	{
//		fread(szHttpBody, 1, webrtcPlayerFileLength, fWebRtc);
//		fclose(fWebRtc);
//	}
//	else
//	{
//		WriteLog(Log_Debug, "CNetServerSendWebRTC = %X , nClient = %llu , webrtc.html 文件不存在 ", this, nClient);
//		bRunFlag = false;
//		DeleteNetRevcBaseClient(nClient);
// 	}
//
//	sprintf(szHttpHead, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: max-age=3600\r\nContent-Type: text/html\r\nServer: mediamtx\r\nDate: Wed, 07 May 2025 05:36:18 GMT\r\nContent-Length: %d\r\n\r\n", webrtcPlayerFileLength);
//	int nRet = XHNetSDK_Write(nClient, (unsigned char*)szHttpHead, strlen(szHttpHead), ABL_MediaServerPort.nSyncWritePacket);
//    nRet = XHNetSDK_Write(nClient, (unsigned char*)szHttpBody, webrtcPlayerFileLength, ABL_MediaServerPort.nSyncWritePacket);
// 
//    nWebRTC_Comm_State = WebRTC_Comm_State_ResponseWebpage;
//   return true;
//}
//
////回复OPTIONS
//bool  CNetServerSendWebRTC::ResponseOPTIONS()
//{
//	char szOptions[string_length_4096] = { 0 };
// 	int     nPos1, nPos2;
//	string  strOptions;
//	bool    bResponseFlag = false;
//	char    szURL[string_length_1024];
//
//	memset(szPlayParams, 0x00, sizeof(szPlayParams));
//	if (httpParse.GetFieldValue("OPTIONS", szOptions))
//	{
//		strcpy(szHttpModem,"OPTIONS");
//		memset(szHttpURL,0x00,sizeof(szHttpURL));
//		WriteLog(Log_Debug, "ResponseOPTIONS() = %X , nClient = %llu , szOptions = %s ", this, nClient, szOptions);
//		
//		strOptions = szOptions;
//		nPos1 = strOptions.find("?", 0);
//		nPos2 = strOptions.find(" ", 0);
//		if (nPos1 > 0 && nPos2 > nPos1)
//		{
//			memcpy(szHttpURL,szOptions,nPos2);
//			m_location = szHttpURL;
//			memmove(szOptions, szOptions + nPos1 + 1, nPos2 - nPos1 - 1);
//			szOptions[nPos2 - nPos1 - 1] = 0x00;
//			strcpy(szPlayParams,szOptions);
//
//			if (SplitterTextParam(szOptions))
//			{
//				GetKeyValue("app", app);
//				GetKeyValue("stream", stream);
// 				sprintf(szURL, "/%s/%s", app, stream);
//				boost::shared_ptr<CMediaStreamSource> pMediaSource = GetMediaStreamSource(szURL);
//				if (pMediaSource)
//				{
//					sprintf(szHttpBody, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Allow-Methods: *\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Credentials: false\r\nAccess-Control-Request-Private-Network: true\r\nContent-Length: 0\r\nServer: %s\r\n\r\n",MediaServerVerson);
//
//					int nRet = XHNetSDK_Write(nClient, (unsigned char*)szHttpBody, strlen(szHttpBody), ABL_MediaServerPort.nSyncWritePacket);
//					nWebRTC_Comm_State = WebRTC_Comm_State_ResponseOPTIONS;
//
//					WriteLog(Log_Debug, "ResponseOPTIONS() = %X , nClient = %llu ,szHttpURL = %s, 回复\r\n%s", this, nClient, szHttpURL , szHttpBody);
// 					pMediaSource->AddClientToMap(nClient);
// 					bResponseFlag = true;
//					return true;
//				}else
//					bResponseFlag = false;
//			}
//			else
//				bResponseFlag = false;
//		}
//		else
//			bResponseFlag = false;
//	}
//
//	if(bResponseFlag == false)
//	{
//		sprintf(szHttpBody, "HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Max-Age: 86440\r\nAccess-Control-Allow-Headers: Content-type,Access-Token\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET\r\nServer: %s\r\n\r\n",MediaServerVerson);
//		int nRet = XHNetSDK_Write(nClient, (unsigned char*)szHttpBody, strlen(szHttpBody), ABL_MediaServerPort.nSyncWritePacket);
//		
//		WriteLog(Log_Debug, "ResponseOPTIONS() = %X , nClient = %llu , 命令错误 或者 媒体源 /%s/%s 不存在 , 执行删除 ", this, nClient,app ,stream );
//		deleteWebRTC();
//	}
//}
//
//GMainLoop* CNetServerSendWebRTC::getMainLoop()
//{
//	static GMainLoop* mainLoop = nullptr;
//	static GMainContext* context = nullptr;
//	static std::shared_ptr<std::thread> mainLoopThread;
//	static std::once_flag of;
//	std::call_once(of, []()
//		{ mainLoopThread = std::make_shared<std::thread>([]()
//			{
//				g_networking_init();
//				context = g_main_context_new();
//				mainLoop = g_main_loop_new(context, FALSE);
//				g_main_context_push_thread_default(context);
//				g_main_loop_run(mainLoop);
//				g_main_context_pop_thread_default(context);
//				g_main_loop_unref(mainLoop);
//				g_main_context_unref(context); });
//	std::this_thread::sleep_for(std::chrono::milliseconds(1000)); });
//
//	return mainLoop;
//}
//
//GMainContext* CNetServerSendWebRTC::getMainContext()
//{
//	return g_main_loop_get_context(getMainLoop());
//}
//
//bool CNetServerSendWebRTC::SrtpInit()
//{
//	static std::once_flag of;
//	std::call_once(of, []()
//		{ srtp_init(); });
//	if (!m_srtp)
//	{
//		srtp_policy_t policy;
//		memset(&policy, 0, sizeof(policy));
//		// srtp_crypto_policy_set_rtp_default(&policy.rtp);
//		// srtp_crypto_policy_set_rtcp_default(&policy.rtcp);
//		srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
//		srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
//		policy.ssrc.type = ssrc_any_inbound;
//		policy.key = m_srvSrtpKey;  // 设置密钥
//		policy.window_size = 128;   // 重放保护窗口大小
//		policy.allow_repeat_tx = 0; // 是否允许重复发送
//		policy.next = NULL;         // 单流场景
//
//		srtp_err_status_t status = srtp_create(&m_srtp, &policy);
//		if (status != srtp_err_status_t::srtp_err_status_ok || !m_srtp)
//		{
//			WriteLog(Log_Debug, "SrtpInit failed to create srtp session ,pullhandle=%llu", nClient);
//			return false;
//		}
//		else
//		{
//			nWebRTC_Comm_State = WebRTC_Comm_State_StartPlay;
//			WriteLog(Log_Debug, "SrtpInit create srtp session successfully ,pullhandle=%llu", nClient);
//		}
//	}
//	return true;
//}
//
//void CNetServerSendWebRTC::onCandidateGatheringDone(NiceAgent* agent, guint streamId, gpointer data)
//{
//	WriteLog(Log_Debug, "**********onCandidateGatheringDone**********");
//
//	boost::shared_ptr<CNetRevcBase> pullHandle = GetNetRevcBaseClient(static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//	if (pullHandle == NULL)
//	{
//		WriteLog(Log_Debug, "can not find handle, pullhandle=%llu", static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//		return;
//	}
//	CNetServerSendWebRTC* pNetServerSendWebRTC = (CNetServerSendWebRTC*)pullHandle.get();
//	pNetServerSendWebRTC->handleCandidateGatheringDone(streamId);
//}
//
//void CNetServerSendWebRTC::onNewSelectedPair(NiceAgent* agent, guint streamId, guint componentId, gchar* lfoundation, gchar* rfoundation, gpointer data)
//{
//	WriteLog(Log_Debug, "**********onNewSelectedPair**********");
//
//	boost::shared_ptr<CNetRevcBase> pullHandle = GetNetRevcBaseClient(static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//	if (pullHandle == NULL)
//	{
//		WriteLog(Log_Debug, "can not find handle, pullhandle=%llu",  static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//		return;
//	}
//	CNetServerSendWebRTC* pNetServerSendWebRTC = (CNetServerSendWebRTC*)pullHandle.get();
// 	pNetServerSendWebRTC->handleNewSelectedPair(streamId, componentId, lfoundation, rfoundation);	
//}
//
//void CNetServerSendWebRTC::onComponentStateChanged(NiceAgent* agent, guint streamId, guint componentId, guint state, gpointer data)
//{
//	WriteLog(Log_Debug, "**********onComponentStateChanged**********");
//
//	boost::shared_ptr<CNetRevcBase> pullHandle = GetNetRevcBaseClient(static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//	if (pullHandle == NULL)
//	{
//		WriteLog(Log_Debug, "can not find handle, pullhandle=%llu",  static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//		return;
//	}
//	CNetServerSendWebRTC* pNetServerSendWebRTC = (CNetServerSendWebRTC*)pullHandle.get();
// 	pNetServerSendWebRTC->handleComponentStateChanged(streamId, componentId, state);
//}
//
//void CNetServerSendWebRTC::onNiceRecv(NiceAgent* agent, guint streamId, guint componentId, guint len, gchar* buf, gpointer data)
//{
//	//WriteLog(Log_Debug, "**********onNiceRecv()**********");
//	boost::shared_ptr<CNetRevcBase> pullHandle = GetNetRevcBaseClient(static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//	if (pullHandle == NULL)
//	{
//		WriteLog(Log_Debug, "onNiceRecv can not find handle, pullhandle=%llu", static_cast<uint32_t>(reinterpret_cast<uint64_t>(data)));
//		return;
//	}
//	CNetServerSendWebRTC* pNetServerSendWebRTC = (CNetServerSendWebRTC*)pullHandle.get();
// 	pNetServerSendWebRTC->handleNiceRecv(streamId, componentId, len, buf);
//}
//
//void  CNetServerSendWebRTC::handleCandidateGatheringDone(guint streamId)
//{
//	WriteLog(Log_Debug, "handleCandidateGatheringDone. streamId: %d ,pullhandle=%llu" ,streamId, nClient);
//
//	if (0 == streamId)
//	{
//		WriteLog(Log_Debug, "handleCandidateGatheringDone invalid streamId: %d,pullhandle=%llu" , streamId, nClient);
//		deleteWebRTC();
//		return;
//	}
//
//	m_localCandidateFlag |= (m_vStreamId == streamId ? 1 : (m_aStreamId == streamId ? 2 : 0));
//	// if (3 == m_localCandidateFlag)
//	if (1 == m_localCandidateFlag)
//	{
//		//WriteLog(Log_Debug, "handleCandidateGatheringDone done. start to parse remote sdp,handle= %ld , sdp:\n%s\n", nClient, szHttpBody);
//
//		gchar* ufrag = nullptr, * pwd = nullptr;
//		GSList* candidates = nice_agent_parse_remote_stream_sdp(m_iceAgent, m_vStreamId, szHttpBody, &ufrag, &pwd);
//		if (!ufrag || !pwd )
//		{
//			deleteWebRTC();
//			WriteLog(Log_Debug, "handleCandidateGatheringDone failed to parse remote stream sdp. streamId: %d,pullhandle=%llu" , m_vStreamId, nClient);
//			if (ufrag)
//				g_free(ufrag);
//			if (pwd)
//				g_free(pwd);
//		}
//		else
//		{
//			std::string strufrag = ufrag;
//			std::string strpwd = pwd;
//
//			if (strufrag[strufrag.size() - 1] == '\n' || strufrag[strufrag.size() - 1] == '\r')
//				strufrag = strufrag.substr(0, strufrag.size() - 1);
//
//			if (strpwd[strpwd.size() - 1] == '\n' || strpwd[strpwd.size() - 1] == '\r')
//				strpwd = strpwd.substr(0, strpwd.size() - 1);
//
//			WriteLog(Log_Debug, "handleCandidateGatheringDone parse remote stream sdp successfully. streamId: %d. ufrag: %s. pwd: %s,pullhandle=%llu" , m_vStreamId , strufrag.c_str(), strpwd.c_str(), nClient);
//			if (!nice_agent_set_remote_credentials(m_iceAgent, m_vStreamId, strufrag.c_str(), strpwd.c_str()))
//			{
//				WriteLog(Log_Debug, "failed to set remote credentials. streamId:%d ,pullhandle=%llu" ,m_vStreamId, nClient);
//				deleteWebRTC();
//				g_free(ufrag);
//				g_free(pwd);
//				//g_slist_free(candidates);
//				return;
//			}
//			else
//			{
//				WriteLog(Log_Debug, "set remote credentials successfully. streamId: %d,pullhandle=%llu" ,m_vStreamId, nClient);
//			}
//
//			g_free(ufrag);
//			g_free(pwd);
//			//g_slist_free(candidates);
//			//nice_agent_peer_candidate_gathering_done(m_iceAgent, m_vStreamId);
//			
//			GSList* candidateList= nice_agent_get_local_candidates(m_iceAgent, m_vStreamId,1);
//			if (candidateList)
//			{
//				for (GSList* it = candidateList; it; it = it->next)
//				{
//					NiceCandidate* candidate = (NiceCandidate*)it->data;
//
//					ip =  nice_address_dup_string(&candidate->addr);
//					port = nice_address_get_port(&candidate->addr);
//
//					priority = candidate->priority;
//					ResponseHttpMessageToClient(true);
//				
//					WriteLog(Log_Debug, " nice_agent_get_local_candidatess, stream_id = %d,component_id = %d",  candidate->stream_id, candidate->component_id);
//					WriteLog(Log_Debug, "deal ice nice_agent_get_local_candidatess, type = %d,transport= %d ,priority = %d", candidate->type, candidate->transport, candidate->priority);
//					WriteLog(Log_Debug, "deal ice nice_agent_get_local_candidatess, stream_id = %d,username= %s ,password = %s", candidate->stream_id, candidate->username, candidate->password);
//					
//					//std::string ip1 = nice_address_dup_string(&candidate->base_addr);
//					//int port1 = nice_address_get_port(&candidate->base_addr);
//					 WriteLog(Log_Debug, " nice_agent_get_local_candidatess, ip = %s,port = %d", ip.c_str(), port);
//					//WriteLog(Log_Debug, " nice_agent_get_local_candidatess, ip1 = %s,port1 = %d", ip1.c_str(), port1);
//					break;
//				}
//				
//			}
//			else
//			{
//				ResponseHttpMessageToClient(false);
//			}
//			
//		}
//	}
//}
//
//static const char* sm_testSdp = "v=0\r\n"
//"o=%s 88273680 2 IN IP4 0.0.0.0\r\n"
//"s=%s\r\n"
//"t=0 0\r\n"
//"a=ice-lite\r\n"
//"a=group:BUNDLE 0 1\r\n"
//"a=msid-semantic: WMS \r\n"
//"m=audio 9 UDP/TLS/RTP/SAVPF 111 9 0 8\r\n"
//"c=IN IP4 0.0.0.0\r\n"
//"a=ice-ufrag:%s\r\n"
//"a=ice-pwd:%s\r\n"
//"a=fingerprint:sha-256 %s\r\n"
//"a=setup:passive\r\n"
//"a=mid:0\r\n"
//"a=rtcp-mux\r\n"
//"a=rtcp-rsize\r\n"
//"a=rtpmap:9 G722/8000\r\n"
//"a=rtcp-fb:9 transport-cc\r\n"
//"a=rtpmap:0 PCMU/8000\r\n"
//"a=rtcp-fb:0 transport-cc\r\n"
//"a=rtpmap:8 PCMA/8000\r\n"
//"a=rtcp-fb:8 transport-cc\r\n"
//"a=rtpmap:111 opus/48000/2\r\n"
//"a=rtcp-fb:111 transport-cc\r\n"
//"a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
//"a=sendonly\r\n"
//"a=ssrc:%d cname:2s2y4w39v2o64c6a\r\n"
//"a=ssrc:%d label:audio-m3e496p3\r\n"
//"a=candidate:0 1 udp %ld %s %d typ host generation 0\r\n"
//"m=video 9 UDP/TLS/RTP/SAVPF %d\r\n"
//"c=IN IP4 0.0.0.0\r\n"
//"a=mid:1\r\n"
//"a=ice-ufrag:%s\r\n"
//"a=ice-pwd:%s\r\n"
//"a=fingerprint:sha-256 %s\r\n"
//"a=setup:passive\r\n"
//"a=rtcp-mux\r\n"
//"a=rtcp-rsize\r\n"
//"a=rtpmap:%d H264/90000\r\n"
//"a=fmtp:%d level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=%s\r\n"
//"a=rtcp-fb:%d transport-cc\r\n"
//"a=rtcp-fb:%d nack\r\n"
//"a=rtcp-fb:%d nack pli\r\n"
//"a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
//"a=ssrc:%d cname:2s2y4w39v2o64c6a\r\n"
//"a=ssrc:%d label:video-m3e496p3\r\n"
//"a=sendonly\r\n"
//"a=candidate:0 1 udp %ld %s %d typ host generation 0\r\n";
//
//std::string CNetServerSendWebRTC::XHServerCreateSdp(std::string localUfrag, std::string localPwd, int videoId, std::string levelStrId, std::string ip, int port, uint64_t priority,std::string fingerprint, int ssrc,int nAudioRate,int nAudioChannel)
//{
//	WriteLog(Log_Debug, "XHServerCreateSdp() nClient = %llu \r\nlocalUfrag = %s,  localPwd = %s , videoId = %d , levelStrId = %s,  ip = %s,  port = %d, priority = %llu , fingerprint = %s, ssrc = %d, nAudioRate = %d , nAudioChannel = %d ", nClient,
//	localUfrag.c_str(), localPwd.c_str(), videoId, levelStrId.c_str(), ip.c_str(),  port,  priority, fingerprint.c_str(),  ssrc, nAudioRate, nAudioChannel);
//	
//	std::string responseContent;
//	char strbuf[10240] = { 0 };
//	sprintf(strbuf, sm_testSdp, MediaServerVerson,MediaServerVerson,localUfrag.c_str(), localPwd.c_str(), fingerprint.c_str(), ssrc-1, ssrc-1, priority, ip.c_str(), port, videoId, localUfrag.c_str(), localPwd.c_str(), fingerprint.c_str(), videoId,  videoId, levelStrId.c_str(), videoId, videoId, videoId, ssrc, ssrc, priority, ip.c_str(), port);
//	responseContent = strbuf;
//	
//	WriteLog(Log_Debug,"strbuf = \r\n%s",strbuf);
//	return responseContent;
//}
//
//void CNetServerSendWebRTC::ResponseHttpMessageToClient(bool bSuccess)
//{
//	char ABL_HttpRsp[10240]={0};
//	if (bSuccess)
//	{
//		std::string responseContent = XHServerCreateSdp(m_localUfrag, m_localPwd, nVideoPayload, VIDEO_LEVEL_ID_H264, ip, port, priority, DTLS_FINGER_PRINT,m_nVideoSSRC, m_nAudioRate, m_nAudioChannel);
//		sprintf(ABL_HttpRsp, "HTTP/1.1 201 Created\r\nConnection: Close\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Allow-Methods: *\r\nAccess-Control-Expose-Headers: *\r\nAccess-Control-Allow-Credentials: false\r\nAccess-Control-Request-Private-Network: true\r\nContent-Type: application/sdp\r\nLocation: %s\r\nContent-Length: %d\r\nServer: %s\r\n\r\n%s",szHttpURL, responseContent.size(),MediaServerVerson, responseContent.c_str());
//		XHNetSDK_Write(nClient, (unsigned char*)ABL_HttpRsp, strlen(ABL_HttpRsp),ABL_MediaServerPort.nSyncWritePacket);
// 		WriteLog(Log_Debug, "ICEsuccess,return 201 Created.  pullhandle=%llu, ABL_HttpRsp = \r\n%s", nClient,ABL_HttpRsp);
//	}
//	else
//	{
//		sprintf(ABL_HttpRsp, "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Max-Age: 86440\r\nAccess-Control-Allow-Headers: Content-type,Access-Token\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET\r\nServer: XH_HTTP_SERVER\r\n\r\n");
//		XHNetSDK_Write(nClient, (unsigned char*)ABL_HttpRsp, strlen(ABL_HttpRsp),ABL_MediaServerPort.nSyncWritePacket);
// 		WriteLog(Log_Debug, "ICE error,return 500 error.  pullhandle=%llu", nClient);
//		deleteWebRTC();
//	}
//}
//
//void  CNetServerSendWebRTC::handleNewSelectedPair(guint streamId, guint componentId, gchar* lfoundation, gchar* rfoundation)
//{
//	WriteLog(Log_Debug, "NewSelectedPair. streamId: %d. componentId: %d. lfoundation: %ld. rfoundation: %ld ,pullhandle=%llu" , streamId ,componentId, lfoundation , rfoundation, nClient);
//}
//
//void  CNetServerSendWebRTC::handleComponentStateChanged(guint streamId, guint componentId, guint state)
//{
//	WriteLog(Log_Debug, "handleComponentStateChanged. streamId: %d. componentId: %d. state: %d ,pullhandle=%llu" , streamId ,componentId,state, nClient);
//
//	//if (NICE_COMPONENT_STATE_READY == state || NICE_COMPONENT_STATE_CONNECTING == state)
//	if (NICE_COMPONENT_STATE_READY == state)
//	{
//		int32_t ec = DoDtls();
//		if (0 != ec)
//		{
//			//abl_HeartbeatDisconnectList.push((unsigned char*)&nClient, sizeof(unsigned long long));
//			WriteLog(Log_Debug, "handleComponentStateChanged DoDtls error, pullhandle push to delete,pullhandle=%llu", nClient);
//		}
//	}
//	else if (NICE_COMPONENT_STATE_FAILED == state)
//	{
//		//abl_HeartbeatDisconnectList.push((unsigned char*)&nClient, sizeof(unsigned long long));
//		WriteLog(Log_Debug, "handleComponentStateChanged NICE_COMPONENT_STATE_FAILED error, pullhandle=%llu", nClient);
//	}
//}
//
//int32_t CNetServerSendWebRTC::DoDtls()
//{
//	static std::string certPath ;
//	static std::string keyPath ;
//	//static const std::string certPath = TC_File::extractFilePath(TC_File::getExePath()) + "cacert.pem";
//	//static const std::string keyPath = TC_File::extractFilePath(TC_File::getExePath()) + "privkey.pem";
//	static std::once_flag of;
//	std::call_once(of, []
//		{
//			char  path[1024] = { 0 };
//			std::string temp = ABL_MediaSeverRunPath;
//			std::size_t pos_last = temp.find_last_of('/');
//			path[pos_last + 1] = '\0';
//			//strcat(path, "server.pem");
//			certPath =  std::string("./cacert.pem");
//			keyPath =   std::string("./privkey.pem");
//			// SSL_Library_init();
//			SSL_load_error_strings();
//			OpenSSL_add_ssl_algorithms();
//
//			WriteLog(Log_Debug, "DoDtls()  certPath = %s ", certPath.c_str());
//			WriteLog(Log_Debug, "DoDtls()  keyPath = %s ",  keyPath.c_str());
//			
//			const SSL_METHOD* method = DTLS_method();
//			sm_dtlsCtx = SSL_CTX_new(method);
//			if (!sm_dtlsCtx)
//			{
//				WriteLog(Log_Debug, "DoDtls SSL_CTX_new error");
//			}
//			else
//			{
//				WriteLog(Log_Debug, "DoDtls SSL_CTX_new success");
//				int32_t ec = SSL_CTX_use_certificate_file(sm_dtlsCtx, certPath.c_str(), SSL_FILETYPE_PEM);
//				if (ec <= 0)
//				{
//					int32_t sslec = ERR_get_error();
//					char buf[1024] = { 0 };
//					ERR_error_string(sslec, buf);
//					WriteLog(Log_Debug, "DoDtls SSL_CTX_use_certificate_file error,ec:%d,ERR_get_error:%d, ERR_error_string:%s,certPath:%s ", ec, sslec, buf, certPath.c_str());
//					
//					SSL_CTX_free(sm_dtlsCtx);
//					sm_dtlsCtx = nullptr;
// 				}
//				else
//				{
//					WriteLog(Log_Debug, "DoDtls SSL_CTX_use_certificate_file success");
//					ec = SSL_CTX_use_PrivateKey_file(sm_dtlsCtx, keyPath.c_str(), SSL_FILETYPE_PEM);
//					if (ec <= 0)
//					{
//						int32_t sslec = ERR_get_error();
//						char buf[1024] = { 0 };
//						ERR_error_string(sslec, buf);
//						WriteLog(Log_Debug, "DoDtls SSL_CTX_use_PrivateKey_file error,ec:%d,ERR_get_error:%d, ERR_error_string:%s,keyPath:%s ", ec, sslec, buf, keyPath.c_str());
//
//						SSL_CTX_free(sm_dtlsCtx);
//						sm_dtlsCtx = nullptr;
// 						return;
//					}
//					else if ((ec = SSL_CTX_check_private_key(sm_dtlsCtx)) <= 0)
//					{
//						int32_t sslec = ERR_get_error();
//						char buf[1024] = { 0 };
//						ERR_error_string(sslec, buf);
//
//						WriteLog(Log_Debug, "DoDtls SSL_CTX_check_private_key error,ec:%d,ERR_get_error:%d, ERR_error_string:%s", ec, sslec, buf);
//
//						SSL_CTX_free(sm_dtlsCtx);
//						sm_dtlsCtx = nullptr;
// 						return ;
//					}
//
//					WriteLog(Log_Debug, "DoDtls SSL_CTX_use_PrivateKey_file success");
//					//const char *profiles = "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32";
//					const char* profiles = "SRTP_AES128_CM_SHA1_80";
//					int ret = SSL_CTX_set_tlsext_use_srtp(sm_dtlsCtx, profiles);
//					if (ret != 0)
//					{
//						int32_t sslec = ERR_get_error();
//						char buf[1024] = { 0 };
//						ERR_error_string(sslec, buf);
//						WriteLog(Log_Debug, "DoDtls SSL_CTX_set_tlsext_use_srtp error,ec:%d,ERR_get_error:%d, ERR_error_string:%s", ec, sslec, buf);
//
//						SSL_CTX_free(sm_dtlsCtx);
//						sm_dtlsCtx = nullptr;
// 						return;
//					}
//				}
//			}
//			WriteLog(Log_Debug, "DoDtls initialize SSL module success"); });
//
//	if (!sm_dtlsCtx)
//	{
//		WriteLog(Log_Debug, "DoDtls dtls_invalid_context ,pullhandle=%llu ", nClient);
//        deleteWebRTC();
//		return -1;
//	}
//		
//
//	if (!m_ssl && !(m_ssl = SSL_new(sm_dtlsCtx)))
//	{
//		WriteLog(Log_Debug, "DoDtls dtls_invalid_ssl ,pullhandle=%llu ", nClient);
//        deleteWebRTC();
//		return -1;
//	}
//		
//	SSL_set_options(m_ssl, SSL_OP_NO_QUERY_MTU);
//	SSL_set_mtu(m_ssl, 1500);
//
//	m_inBio = BIO_new(BIO_s_mem());
//	m_outBio = BIO_new(BIO_s_mem());
//	if (!m_inBio || !m_outBio)
//	{
//		if (m_inBio)
//			BIO_free(m_inBio);
//		WriteLog(Log_Debug, "dtls_malloc_bio_error ,pullhandle=%llu ", nClient);
//        deleteWebRTC();
//		return -1;
//	}
//
//	SSL_set_bio(m_ssl, m_inBio, m_outBio);
//
//	SSL_set_accept_state(m_ssl); // 设置为服务端模式
//   
//    WriteLog(Log_Debug, "DoDtls() success pullhandle=%llu ", nClient);
//
//	return 0;
//}
//
//int64_t CNetServerSendWebRTC::currentTimestampMs()
//{
//	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
//}
//
//void  CNetServerSendWebRTC::handleNiceRecv(guint streamId, guint componentId, guint len, gchar* buf)
//{
//	//这里收到rtcp包回应，通过这里监控webrtc播放是否结束 
//	nRecvDataTimerBySecond = 0;
//
//	if (m_vStreamId == streamId && buf && len > 0 && m_ssl!=NULL)
//	{
//		if (!SSL_is_init_finished(m_ssl))
//		{
//			WriteLog(Log_Debug, "handleNiceRecv SSL_do_handshake . streamId:%d,componentId:%d,len:%d, pullhandle=%llu", streamId, componentId, len, nClient);
//			if (BIO_write(m_inBio, buf, len) == static_cast<int32_t>(len))
//			{
//				WriteLog(Log_Debug, "handleNiceRecv SSL_do_handshake continue...... . state:%s, pullhandle=%llu", SSL_state_string(m_ssl), nClient);
//				int32_t ec = SSL_do_handshake(m_ssl);
//
//				if (ec <= 0)
//				{
//					int32_t sslec = SSL_get_error(m_ssl, ec);
//					if (SSL_ERROR_WANT_READ != sslec /*&& SSL_ERROR_WANT_WRITE != sslec*/)
//					{
//						deleteWebRTC();
//						WriteLog(Log_Debug, "handleNiceRecv SSL_do_handshake error. ec::%d, pullhandle=%llu", sslec, nClient);
//						return;
//					}
//
//					size_t s = BIO_ctrl_pending(m_outBio);
//					if (s > 0 && (s = BIO_read(m_outBio, m_bioBuff, 8192)) > 0)
//					{
//						if (static_cast<gint>(s) != nice_agent_send(m_iceAgent, m_vStreamId, 1, static_cast<guint>(s), m_bioBuff))
//						{
//							deleteWebRTC();
//							WriteLog(Log_Debug, "handleNiceRecv nice_agent_send error, pullhandle=%llu",  nClient);
//							return;
//						}
//					}
//					else
//					{
//						WriteLog(Log_Debug, "handleNiceRecv wbio is empty. SSL_is_init_finished:%d,state:%d, pullhandle=%llu", SSL_is_init_finished(m_ssl), SSL_state_string(m_ssl), nClient);
//					}
//				}
//				else
//				{
//
//					size_t s = BIO_ctrl_pending(m_outBio);
//					if (s > 0 && (s = BIO_read(m_outBio, m_bioBuff, 8192)) > 0)
//					{
//						if (static_cast<gint>(s) != nice_agent_send(m_iceAgent, m_vStreamId, 1, static_cast<guint>(s), m_bioBuff))
//						{
//							deleteWebRTC();
//							WriteLog(Log_Debug, "handleNiceRecv nice_agent_send error, pullhandle=%llu",  nClient);
//							return;
//						}
//					}
//					else
//					{
//						deleteWebRTC();
//						WriteLog(Log_Debug, "handleNiceRecv wbio is empty. SSL_is_init_finished:%d,state:%d, pullhandle=%llu", SSL_is_init_finished(m_ssl), SSL_state_string(m_ssl), nClient);
//					}
//
//					WriteLog(Log_Debug, "handleNiceRecv SSL handleshake is success, pullhandle=%llu",  nClient);
//					// 获取协商的 SRTP 保护配置
//					const SRTP_PROTECTION_PROFILE* profile = SSL_get_selected_srtp_profile(m_ssl);
//					if (!profile)
//					{
//						deleteWebRTC();
//						WriteLog(Log_Debug, "handleNiceRecv srtp profile is empty, pullhandle=%llu",  nClient);
//						return;
//					}
//					else
//					{
//						memcpy(&m_srtpProf, profile, sizeof(m_srtpProf));
//					}
//
//					// 导出 SRTP 密钥材料
//					size_t key_len = sizeof(m_srtpKey);
//					if (SSL_export_keying_material(m_ssl, m_srtpKey, key_len, "EXTRACTOR-dtls_srtp", 19, NULL, 0, 0) <= 0)
//					{
//						deleteWebRTC();
//						WriteLog(Log_Debug, "handleNiceRecv srtp invalid key material, pullhandle=%llu", nClient);
//						return;
//					}
//					else
//					{
//						memcpy(m_srvSrtpKey, m_srtpKey + 16, 16);
//						memcpy(m_srvSrtpKey + 16, m_srtpKey + 16 + 16 + 14, 14);
//					}
//
//					//WHEP_SESSION_LOGINFO("all steps is success. whep-playing is successful");
//					WriteLog(Log_Debug, "handleNiceRecv all steps is success. whep-playing is successful, pullhandle=%llu", nClient);
//					m_succPlayTT = currentTimestampMs();
//					if (!SrtpInit())
//					{
//						deleteWebRTC();
//						WriteLog(Log_Debug, "handleNiceRecv SrtpInit error, pullhandle=%llu", nClient);
//					}			
//				}
//			}
//			else
//			{
//				//WHEP_SESSION_LOGERROR("do BIO_write with rbio error");
//				WriteLog(Log_Debug, "handleNiceRecv do BIO_write with rbio error, pullhandle=%llu", nClient);
//				//if (m_fnOp)
//				//	m_fnOp(whepEnoToInt(e_whep_eno::dtls_handshake_error), nullptr);
//                deleteWebRTC();
//				return;
//			}
//		}
//		else
//		{
//		}
//	}
//}
//
////回复post 
//bool  CNetServerSendWebRTC::ResponsePost()
//{
//	char    szPost[string_length_4096] = { 0 };
//	char    szURL[string_length_1024]={0} ;
// 	int     nPos1, nPos2;
//	string  strPost;
//	bool    bResponseFlag = false;
//   
//	if (httpParse.GetFieldValue("POST", szPost))
//	{
//		strcpy(szHttpModem,"POST");
//		memset(szHttpURL,0x00,sizeof(szHttpURL));
//		strPost = szPost ;
//		nPos1 = strPost.find(" ",0) ;
//		if(nPos1 != string::npos)
//		   memcpy(szHttpURL,szPost,nPos1);
//	   
//		WriteLog(Log_Debug, "ResponsePost() = %X , nClient = %llu , szVideoName = %s , szAudioCodec = %s, szHttpURL = %s ", this, nClient,mediaCodecInfo.szVideoName,mediaCodecInfo.szAudioName, szHttpURL);
//		
//		//回复POST之前快速的确定视频、音频编码格式 
//		sprintf(szURL, "/%s/%s", app, stream);
//		boost::shared_ptr<CMediaStreamSource> pMediaSource = GetMediaStreamSource(szURL);
//		if (pMediaSource)
//			memcpy((char*)&mediaCodecInfo,(char*)&pMediaSource->m_mediaCodecInfo,sizeof(mediaCodecInfo));
//		
//		//查找视频Paylaod 
//  		if(!GetVideoPayload(mediaCodecInfo.szVideoName))
//			return false ;
//		
//		if (m_iceAgent)
//		{
//			g_object_unref(m_iceAgent);
//			m_iceAgent = nullptr;
//		}
//
//		NiceAgentOption flags = NiceAgentOption(NICE_AGENT_OPTION_ICE_TRICKLE | NICE_AGENT_OPTION_LITE_MODE);
//		// m_iceAgent = nice_agent_new(getMainContext(), NiceCompatibility(NICE_COMPATIBILITY_RFC5245));
//		m_iceAgent = nice_agent_new_full(getMainContext(), NICE_COMPATIBILITY_RFC5245, flags);
//		if (!m_iceAgent)
//		{
//			WriteLog(Log_Debug, "deal ice ice_malloc_agent_error,pullhandle=%llu ", nClient);
//			deleteWebRTC();
//			return false ;// ice_malloc_agent_error);
//		}
//		WriteLog(Log_Debug, "ResponsePost() = %X , nClient = %llu , m_iceAgent = %X ", this, nClient, m_iceAgent);
//		
//		gboolean controlling = 1, trickle = 1;
//		g_object_set(m_iceAgent, "controlling-mode", controlling, NULL);
//		//g_object_set(m_iceAgent, "ice-trickle", trickle, NULL);
//		//g_object_set(G_OBJECT(m_iceAgent), "upnp", FALSE, NULL);
//		//g_object_set(G_OBJECT(m_iceAgent), "full-mode", FALSE, NULL);
//		//g_object_set(m_iceAgent, "ice-tcp", icetcp, NULL);
//
//		// connect to the signals
//		g_signal_connect(m_iceAgent, "candidate-gathering-done", G_CALLBACK(&CNetServerSendWebRTC::onCandidateGatheringDone), reinterpret_cast<void*>(static_cast<uint64_t>(nClient)));
//		g_signal_connect(m_iceAgent, "new-selected-pair-full", G_CALLBACK(&CNetServerSendWebRTC::onNewSelectedPair), reinterpret_cast<void*>(static_cast<uint64_t>(nClient)));
//		//g_signal_connect(m_iceAgent, "new-selected-pair", G_CALLBACK(&CNetServerSendWebRTC::onNewSelectedPair), reinterpret_cast<void*>(static_cast<uint64_t>(nClient)));
//		g_signal_connect(m_iceAgent, "component-state-changed", G_CALLBACK(&CNetServerSendWebRTC::onComponentStateChanged), reinterpret_cast<void*>(static_cast<uint64_t>(nClient)));
//		
//
//		// create a new stream with one component
//		m_vStreamId = nice_agent_add_stream(m_iceAgent, 1);
//		if (0 == m_vStreamId)
//		{
//			WriteLog(Log_Debug, "deal ice ice_add_data_stream_error, pullhandle=%llu ", nClient);
//			return -1;
//		}
//
//		if (!nice_agent_set_local_credentials(m_iceAgent, m_vStreamId, m_localUfrag.c_str(), m_localPwd.c_str()))
//		{
//			WriteLog(Log_Debug, "deal ice ice_set_local_credentials_error pullhandle=%llu ", nClient);
//			return -1;
//		}
//			
//		// Attach to the component to receive the data
//		// Without this call, candidates cannot be gathered
//		nice_agent_attach_recv(m_iceAgent, m_vStreamId, 1, getMainContext(), &CNetServerSendWebRTC::onNiceRecv, reinterpret_cast<void*>(static_cast<uint64_t>(nClient)));
//
//		// if (!nice_agent_gather_candidates(m_iceAgent, m_vStreamId) || !nice_agent_gather_candidates(m_iceAgent, m_aStreamId))
//		if (!nice_agent_gather_candidates(m_iceAgent, m_vStreamId))
//		{
//			WriteLog(Log_Debug, "deal ice ice_start_gathering_candidates_error ,pullhandle=%llu ", nClient);
//			return -1;
//		}
//
//		WriteLog(Log_Debug, "deal ice candidate end,pullhandle=%llu ", nClient);
//	
//	    bResponseFlag = true ;
//	}	
//	WriteLog(Log_Debug, "CNetServerSendWebRTC = %X , nClient = %llu , 回复post ", this, nClient);
//	return true;
//}
//
//void CNetServerSendWebRTC::cleanICE()
//{
//	WriteLog(Log_Debug, "cleanICE CNetServerSendWebRTC ,pullhandle=%llu ", nClient);
//	if (m_iceAgent)
//	{
//		WriteLog(Log_Debug, "cleanICE g_object_unref m_iceAgen ,pullhandle=%llu ", nClient);
//		// g_signal_disconnect(m_iceAgent);
//		g_object_unref(m_iceAgent);
//		m_iceAgent = nullptr;
//	}
//}
//
//void CNetServerSendWebRTC::cleanDtls()
//{
//	WriteLog(Log_Debug, "clenDtls WhepRtcSession ,pullhandle=%llu ", nClient);
//
//	if (m_ssl)
//	{
//		WriteLog(Log_Debug, "cleanICE SSL_free m_ssl ,pullhandle=%llu ", nClient);
//		SSL_free(m_ssl);
//		m_ssl = nullptr;
//	}
//}
//
//void CNetServerSendWebRTC::cleanSrtp()
//{
//	WriteLog(Log_Debug, "cleanSrtp WhepRtcSession ,pullhandle=%llu ", nClient);
//
//	if (m_srtp)
//	{
//		WriteLog(Log_Debug, "cleanSrtp srtp_dealloc ,pullhandle=%llu ", nClient);
//		srtp_dealloc(m_srtp);
//		m_srtp = nullptr;
//	}
//}
//
////获取浏览器发送过来的SDP种支持视频的payload 
//bool  CNetServerSendWebRTC::GetVideoPayload(char* videoCodecName)
//{
//	string strHttpBody = szHttpBody ;
//	int    nPos1,nPos2,nPos3 ;
//	char   szVideoPayload[string_length_512]={0} ;
//	char   szFindVideoCodec[string_length_256] = {0} ;
//	
//	sprintf(szFindVideoCodec,"%s/90000",videoCodecName) ;
//	nPos2 = strHttpBody.find(szFindVideoCodec,0);
//	if(nPos2 != string::npos)
//	{
//		nPos1 = strHttpBody.rfind("\r\n",nPos2) ;
//		if(nPos1 != string::npos)
//		{//a=rtpmap:103
//			memcpy(szVideoPayload,szHttpBody+nPos1+2,nPos2 - nPos1 - 2) ;
//			memmove(szVideoPayload,szVideoPayload+9,strlen(szVideoPayload)-9) ;
//			szVideoPayload[strlen(szVideoPayload)-9]=0x00;
//			nVideoPayload = atoi(szVideoPayload);
//		    WriteLog(Log_Debug, "nClient=%llu , 找到视频 payload = %s , nVideoPayload = %d", nClient, szVideoPayload,nVideoPayload);
//			
//			return true ;
//		}
//	}else 
//	{
//		WriteLog(Log_Debug, "nClient=%llu , 没有找到视频 %s 的 payload ，该浏览器不支持该视频编码格式播放 ", nClient,videoCodecName);
//		deleteWebRTC();
//		return false ;
//	}
//}
//
////删除WebRTC播放 
//void  CNetServerSendWebRTC::deleteWebRTC()
//{
//	bRunFlag = false;
//	nWebRTC_Comm_State = WebRTC_Comm_State_Delete ;
//	pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
//}
//
