/*
功能：
    负责发送 GB28181 Rtp 码流，包括UDP、TCP模式  
 	增加 国标接收  （即国标发送的同时也支持国标接收）    2023-05-19
	增加支持1078码流以 2016\2019版本发送                 2023-12-23
	日期    2021-08-15
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181RtpClient.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle); 
extern MediaServerPort                       ABL_MediaServerPort;
extern int                                   SampleRateArray[];
extern CMediaFifo                            pDisconnectMediaSource;      //清理断裂媒体源 

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle); 
extern MediaServerPort                       ABL_MediaServerPort;
extern int                                   SampleRateArray[];
extern CMediaFifo                            pDisconnectMediaSource;      //清理断裂媒体源 

#endif
void PS_MUX_CALL_METHOD GB28181_Send_mux_callback(_ps_mux_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag.load())
		return;
 
	pThis->GB28181PsToRtPacket(cb->data, cb->datasize);

#ifdef  WriteGB28181PSFileFlag
	fwrite(cb->data,1,cb->datasize,pThis->writePsFile);
	fflush(pThis->writePsFile);
#endif
}

static void* ps_alloc(void* param, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;
	 
	return pThis->s_buffer;
}

static void ps_free(void* param, void* /*packet*/)
{
	return;
}

static int ps_write(void* param, int stream, void* packet, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;

	if(pThis->bRunFlag.load())
	  pThis->GB28181PsToRtPacket((unsigned char*)packet, bytes);

	return true;
}

//rtp打包回调视频
void GB28181_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag.load())
		return;

	if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//udp 直接发送 
		pThis->SendGBRtpPacketUDP(cb->data, cb->datasize);
	}
	else if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{//TCP 需要拼接 包头
		pThis->GB28181SentRtpVideoData(cb->data, cb->datasize);
	}
}

//PS 数据打包成rtp 
void  CNetGB28181RtpClient::GB28181PsToRtPacket(unsigned char* pPsData, int nLength)
{
	if(hRtpPS > 0 && bRunFlag.load())
	{
		inputPS.data = pPsData;
		inputPS.datasize = nLength;
		rtp_packet_input(&inputPS);
	}
}

//国标28181PS码流TCP方式发送 
void  CNetGB28181RtpClient::GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength)
{
	if (bRunFlag.load() == false)
		return;
	
	if ((nMaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0)
	{//剩余空间不够存储 ,防止出错 
 		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, ABL_MediaServerPort.nSyncWritePacket);
		if (nSendRet != 0)
		{
			bRunFlag.exchange(false);
 			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标RTP码流出错 ，Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nStartVideoTimestamp != GB28181VideoStartTimestampFlag &&  nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//产生一帧新的视频 
		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, ABL_MediaServerPort.nSyncWritePacket);
		if (nSendRet != 0)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标RTP码流出错 ，Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			bRunFlag.exchange(false);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	if (ABL_MediaServerPort.nGBRtpTCPHeadType == 1)
	{//国标 TCP发送 4个字节方式
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 0] = '$';
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 1] = 0;
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 4), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 4;
	}
	else if (ABL_MediaServerPort.nGBRtpTCPHeadType == 2)
	{//国标 TCP发送 2 个字节方式
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + nSendRtpVideoMediaBufferLength, (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 2;
	}
	else
	{
		bRunFlag.exchange(false);
		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 非法的国标TCP包头发送方式(必须为 1、2 )nGBRtpTCPHeadType = %d ", this, ABL_MediaServerPort.nGBRtpTCPHeadType);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
	}
}

CNetGB28181RtpClient::CNetGB28181RtpClient(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
 	nVideoPT = nAudioPT = -1;
	p1078VideoFrameBuffer = NULL;
	n1078CacheBufferLength = n1078VideoFrameBufferLength = 0;;
	nVdeoFrameNumber = 0;
	hRtpHandle = 0;
	nRecvSampleRate =  nRecvChannels = 0;
	nPFrameCount = 0;
	memset((char*)&gbDstAddr, 0x00, sizeof(gbDstAddr));
	memset((char*)&jt1078VideoHead, 0x00, sizeof(jt1078VideoHead));
	memset((char*)&jt1078AudioHead, 0x00, sizeof(jt1078AudioHead));
	memset((char*)&jt1078OtherHead, 0x00, sizeof(jt1078OtherHead));
	memset((char*)&jt1078VideoHead2019, 0x00, sizeof(jt1078VideoHead2019));
	memset((char*)&jt1078AudioHead2019, 0x00, sizeof(jt1078AudioHead2019));
	memset((char*)&jt1078OtherHead2019, 0x00, sizeof(jt1078OtherHead2019));
	jt1078VideoHead.head[0] = 0x30;
	jt1078VideoHead.head[1] = 0x31;
	jt1078VideoHead.head[2] = 0x63;
	jt1078VideoHead.head[3] = 0x64;
	memcpy(jt1078AudioHead.head, jt1078VideoHead.head,4);
	memcpy(jt1078OtherHead.head, jt1078VideoHead.head, 4);

 	memcpy(jt1078VideoHead2019.head, jt1078VideoHead.head, 4);
	memcpy(jt1078AudioHead2019.head, jt1078VideoHead.head, 4);
	memcpy(jt1078OtherHead2019.head, jt1078VideoHead.head, 4);

	jt1078VideoHead.v = jt1078AudioHead.v = jt1078OtherHead.v = jt1078VideoHead2019.v = jt1078AudioHead2019.v = jt1078OtherHead2019.v = 2;
	jt1078VideoHead.cc = jt1078AudioHead.cc = jt1078OtherHead.cc = jt1078VideoHead2019.cc = jt1078AudioHead2019.cc = jt1078OtherHead2019.cc = 1;
	jt1078VideoHead.m = jt1078AudioHead.m = jt1078OtherHead.m = jt1078VideoHead2019.m = jt1078AudioHead2019.m = jt1078OtherHead2019.m = 1;
	jt1078VideoHead.ch = jt1078AudioHead.ch = jt1078OtherHead.ch = jt1078VideoHead2019.ch = jt1078AudioHead2019.ch = jt1078OtherHead2019.ch = 1;

	nSendRtcpTime = GetTickCount64();
	memset(m_recvMediaSource, 0x00, sizeof(m_recvMediaSource));
	pRecvMediaSource = NULL;
	psBeiJingLaoChenDemuxer = NULL;
	netDataCache = NULL; //网络数据缓存
	netDataCacheLength = 0;//网络数据缓存大小
	nNetStart = nNetEnd = 0; //网络数据起始位置\结束位置
	MaxNetDataCacheCount = 1024*1024*2;
	nRtpRtcpPacketType = 0;

	nMaxRtpSendVideoMediaBufferLength = 640;//默认累计640
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	nServer = hServer;
	psMuxHandle = 0;

	nVideoStreamID = nAudioStreamID = -1;
	handler.alloc = ps_alloc;
	handler.write = ps_write;
	handler.free = ps_free;
    videoPTS = audioPTS = 0;
	s_buffer = NULL;
	psBeiJingLaoChen = NULL;
	if (ABL_MediaServerPort.gb28181LibraryUse == 2)
	{
		s_buffer = new  char[IDRFrameMaxBufferLength];
		psBeiJingLaoChen = ps_muxer_create(&handler, this);
	}
	hRtpPS = 0;
	bRunFlag.exchange(true);

	nSendRtpVideoMediaBufferLength = 0; //已经积累的长度  视频
	nStartVideoTimestamp           = GB28181VideoStartTimestampFlag ; //上一帧视频初始时间戳 ，
	nCurrentVideoTimestamp         = 0;// 当前帧时间戳

	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

#ifdef  WriteGB28181PSFileFlag
	char    szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.ps", ABL_MediaSeverRunPath,this);
	writePsFile = fopen(szFileName,"wb");
#endif
#ifdef WriteRecvPSDataFlag
	fWritePSDataFile = fopen("E:\\recv_app_recv.ps","wb");
#endif
#ifdef WriteJtt1078SrcVideoFlag
	char    szFileName[256] = { 0 };
	sprintf(szFileName, "%s%1078_X.264", ABL_MediaSeverRunPath, this);
	fWrite1078SrcFile = fopen(szFileName, "wb");
#endif

 	WriteLog(Log_Debug, "CNetGB28181RtpClient 构造 = %X  nClient = %llu ", this, nClient);
}

CNetGB28181RtpClient::~CNetGB28181RtpClient()
{
	bRunFlag.exchange(false);
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{
		XHNetSDK_DestoryUdp(nClient);
		XHNetSDK_DestoryUdp(nClientRtcp);
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect )
	{
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));
	}
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
	ps_mux_stop(psDeMuxHandle);
	rtp_packet_stop(hRtpPS);
	if(psBeiJingLaoChen != NULL )
	  ps_muxer_destroy(psBeiJingLaoChen);
	if (psBeiJingLaoChenDemuxer != NULL)
		ps_demuxer_destroy(psBeiJingLaoChenDemuxer);
	SAFE_ARRAY_DELETE(s_buffer);
	SAFE_ARRAY_DELETE(netDataCache);
	SAFE_ARRAY_DELETE(p1078VideoFrameBuffer);

	//最后才删除媒体源
	if (strlen(m_recvMediaSource) > 0 && pRecvMediaSource != NULL)
 	  pDisconnectMediaSource.push((unsigned char*)m_recvMediaSource, strlen(m_recvMediaSource));

#ifdef  WriteGB28181PSFileFlag
	fclose(writePsFile);
#endif
#ifdef WriteRecvPSDataFlag
	if(fWritePSDataFile != NULL)
	  fclose(fWritePSDataFile);
#endif
#ifdef WriteJtt1078SrcVideoFlag
	fclose(fWrite1078SrcFile);
#endif

	WriteLog(Log_Debug, "CNetGB28181RtpClient 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetGB28181RtpClient::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0 ;
	if (!bRunFlag.load() || m_startSendRtpStruct.disableVideo[0] == 0x31)
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	m_videoFifo.push(pVideoData, nDataLength);
	return 0;
}

int CNetGB28181RtpClient::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;

	//当屏蔽音频时，不进行音频打包
	if (!bRunFlag.load() || m_startSendRtpStruct.disableAudio[0] == 0x31)
		return -1;

	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	if (ABL_MediaServerPort.nEnableAudio == 0)
		return -1;

	m_audioFifo.push(pVideoData, nDataLength);
	return 0;
}

void  CNetGB28181RtpClient::CreateRtpHandle()
{
	if (hRtpPS == 0)
	{
		if (strcmp(m_startSendRtpStruct.disableVideo, "1") == 0)
			nMaxRtpSendVideoMediaBufferLength = 640;
		else
			nMaxRtpSendVideoMediaBufferLength = MaxRtpSendVideoMediaBufferLength ;

		int nRet = rtp_packet_start(GB28181_rtp_packet_callback_func_send, (void*)this, &hRtpPS);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ，创建视频rtp打包失败,nClient = %llu,  nRet = %d", this, nClient, nRet);
			return ;
		}
		optionPS.handle = hRtpPS;
		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//国标PS打包
			optionPS.mediatype = e_rtppkt_mt_video;
			optionPS.payload = atoi(m_startSendRtpStruct.payload);
			optionPS.streamtype = e_rtppkt_st_gb28181;
			optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
			optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
			rtp_packet_setsessionopt(&optionPS);
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES \ XHB 打包
			if (atoi(m_startSendRtpStruct.disableAudio) == 1)
			{//只有视频
				optionPS.mediatype = e_rtppkt_mt_video;
				if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				{
					strcpy(m_startSendRtpStruct.payload, "98");
					optionPS.payload = 98;
					optionPS.streamtype = e_rtppkt_st_h264;
				}
				else  if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				{
					strcpy(m_startSendRtpStruct.payload, "99");
					optionPS.payload = 99;
					optionPS.streamtype = e_rtppkt_st_h265;
				}
				optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
				optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
 			}
			else if (atoi(m_startSendRtpStruct.disableVideo) == 1)
			{//只有音频 
				optionPS.mediatype = e_rtppkt_mt_audio;
				optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
				if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
				{
					optionPS.ttincre = 320; 
					optionPS.streamtype = e_rtppkt_st_g711a;
					optionPS.payload = 8;
				}else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
				{
					optionPS.ttincre = 320;  
					optionPS.streamtype = e_rtppkt_st_g711u;
					optionPS.payload = 0;
				}
				else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0) 
				{
					optionPS.ttincre = 1024;  
					optionPS.streamtype = e_rtppkt_st_aac_have_adts;
					optionPS.payload = 97;
				}
 			}
			rtp_packet_setsessionopt(&optionPS);
		}

		inputPS.handle = hRtpPS;
		inputPS.ssrc = optionPS.ssrc;

		memset((char*)&gbDstAddr, 0x00, sizeof(gbDstAddr));
		gbDstAddr.sin_family = AF_INET;
		gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
		memset((char*)&gbDstAddrRTCP, 0x00, sizeof(gbDstAddrRTCP));
		gbDstAddrRTCP.sin_family = AF_INET;
		gbDstAddrRTCP.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddrRTCP.sin_port = htons(atoi(m_startSendRtpStruct.dst_port)+1);//rtcp端口

		//记下媒体源
		SplitterAppStream(m_szShareMediaURL);
		sprintf(m_addStreamProxyStruct.url, "rtp://%s:%s/%s/%s", m_startSendRtpStruct.dst_url, m_startSendRtpStruct.dst_port,m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
	}
}

int CNetGB28181RtpClient::SendVideo()
{
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (!bRunFlag.load() )
		return -1;

	unsigned char* pData = NULL;
	int            nLength = 0;

	//代表交互成功，连接成功
	if (!bUpdateVideoFrameSpeedFlag)
		bUpdateVideoFrameSpeedFlag = true;

	//jtt1078 
	if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x34)
	{
		if (gbDstAddr.sin_port == 0 && netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
		{
			gbDstAddr.sin_family = AF_INET;
			gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
			gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
		}

		if(strcmp(m_startSendRtpStruct.jtt1078_version,"2013") == 0 || strcmp(m_startSendRtpStruct.jtt1078_version, "2016") == 0)
		   SendJtt1078VideoPacket();
		else if(strcmp(m_startSendRtpStruct.jtt1078_version, "2019") == 0)
		   SendJtt1078VideoPacket2019();
		return 0;//别格式直接返回
	}

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//自研
		if (psMuxHandle == 0)
		{
			memset(&init, 0, sizeof(init));
			init.cb = (void*)GB28181_Send_mux_callback;
			init.userdata = this;
			init.alignmode = e_psmux_am_4octet;
			init.ttmode = 0;
			init.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
			init.h = &psMuxHandle;
			int32_t ret = ps_mux_start(&init);

			input.handle = psMuxHandle;

			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ，创建 ps 打包成功  ,nClient = %llu,  nRet = %d", this, nClient, ret);
		}
	}
	else
	{//北京老陈
		if (nVideoStreamID == -1 && psBeiJingLaoChen != NULL )
		{
			if (strcmp(mediaCodecInfo.szVideoName,"H264") == 0 )
			  nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_H264, NULL, 0);
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			  nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_H265, NULL, 0);
		}
	}

	//创建rtp句柄
	CreateRtpHandle();

	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		input.mediatype = e_psmux_mt_video;
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			input.streamtype = e_psmux_st_h264;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			input.streamtype = e_psmux_st_h265;
		else
		{
			m_videoFifo.pop_front();
			return 0;
		}

		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//PS 打包
			//自研PS打包
			if (ABL_MediaServerPort.gb28181LibraryUse == 1)
			{
	  		  input.data = pData;
			  input.datasize = nLength;
			  ps_mux_input(&input);
			}
			else
			{//北京老陈PS打包
				if (nVideoStreamID != -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szVideoName) > 0 )
				{
					if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
						nflags = CheckVideoIsIFrame("H264", pData, nLength);
					else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
						nflags = CheckVideoIsIFrame("H265", pData, nLength);

					if (nMediaSourceType == MediaSourceType_LiveMedia)
					{
						ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nVideoStreamID, nflags, videoPTS, videoPTS, pData, nLength);
						videoPTS += (90000 / mediaCodecInfo.nVideoFrameRate);
					}
					else
					{
						memcpy((char*)&nVdeoFrameNumber, pData, sizeof(uint32_t));
						ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nVideoStreamID, nflags, videoPTS, videoPTS, pData + 4 , nLength - 4);
						videoPTS = nVdeoFrameNumber * (90000 / mediaCodecInfo.nVideoFrameRate);
					}
				}
			}
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES 打包 \ XHB 打包
			if (hRtpPS > 0 && bRunFlag.load())
			{
				inputPS.data = pData;
				inputPS.datasize = nLength;
				rtp_packet_input(&inputPS);
			}
		}
 
		m_videoFifo.pop_front();
	}
	return 0;
}

int CNetGB28181RtpClient::SendAudio()
{
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if ( ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag.load() || m_startSendRtpStruct.disableAudio[0] == 0x31 )
		return 0;

	//jtt1078 
	if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x34)
	{
		if (gbDstAddr.sin_port == 0 && netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
		{
			gbDstAddr.sin_family = AF_INET;
			gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
			gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
		}
		if (strcmp(m_startSendRtpStruct.jtt1078_version, "2013") == 0 || strcmp(m_startSendRtpStruct.jtt1078_version, "2016") == 0)
			SendJtt1078AduioPacket();
		else if (strcmp(m_startSendRtpStruct.jtt1078_version, "2019") == 0)
			SendJtt1078AduioPacket2019();

		return 0;
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		input.mediatype = e_psmux_mt_audio;
		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			input.streamtype = e_psmux_st_aac;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			input.streamtype = e_psmux_st_g711a;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			input.streamtype = e_psmux_st_g711u;
		else
		{
			m_audioFifo.pop_front();
			return 0;
		}

		//创建rtp句柄
		CreateRtpHandle();

		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//国标PS打包时,可以把音频和视频打包在一起,但是ES打包,音频\视频 不能打包在一起
			//自研PS打包
			if (ABL_MediaServerPort.gb28181LibraryUse == 1)
			{
			   input.data = pData;
			   input.datasize = nLength;
 			   ps_mux_input(&input);
			}
			else
			{//北京老陈PS打包
				if (nAudioStreamID == -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szAudioName) > 0 )
				{
					if ( strcmp(mediaCodecInfo.szAudioName,"AAC") == 0 )
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AAC, NULL, 0);
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AUDIO_G711A, NULL, 0);
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AUDIO_G711U, NULL, 0);
				}

				if (nAudioStreamID != -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szAudioName) > 0 )
				{
					if (nMediaSourceType == MediaSourceType_LiveMedia)
 						ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nAudioStreamID, 0, audioPTS, audioPTS, pData, nLength);
 					else
 						ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nAudioStreamID, 0, audioPTS, audioPTS, pData + 4, nLength - 4);

 					if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
						audioPTS += mediaCodecInfo.nBaseAddAudioTimeStamp;
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
						audioPTS += nLength / 8;
 				}
			}
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES\XHB 打包
 			inputPS.data = pData;
			inputPS.datasize = nLength;
 		    rtp_packet_input(&inputPS);
 		}
 
		m_audioFifo.pop_front();
	}
	return 0;
}

//udp方式发送rtp包
void  CNetGB28181RtpClient::SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength)
{
	XHNetSDK_Sendto(nClient, pRtpData, nLength, (void*)&gbDstAddr);

 	if (GetTickCount64() - nSendRtcpTime >= 5 * 1000 ) 
	{//主动发送rtcp包
		nSendRtcpTime = GetTickCount64();

		memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
		rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
		rtcpSR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

		XHNetSDK_Sendto(nClientRtcp, szRtcpSRBuffer, rtcpSRBufferLength, (void*)&gbDstAddrRTCP);
	}
}

int CNetGB28181RtpClient::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
 	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag.load() || nDataLength <= 0 )
		return -1;
	if (!(strlen(m_startSendRtpStruct.recv_app) > 0 && strlen(m_startSendRtpStruct.recv_stream) > 0))
		return -1;

	if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x34)
	{//jt 1078 码流，不是标准的 rtp 流 
		if (pRecvMediaSource == NULL)
		{
			sprintf(m_recvMediaSource, "/%s/%s", m_startSendRtpStruct.recv_app, m_startSendRtpStruct.recv_stream);
			pRecvMediaSource = CreateMediaStreamSource(m_recvMediaSource, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
			if (pRecvMediaSource != NULL)
			{
				pRecvMediaSource->netBaseNetType = netBaseNetType;
				if (strlen(szClientIP) > 0)
					sprintf(pRecvMediaSource->sourceURL, "rtp://%s:%d/%s/%s", szClientIP, nClientPort, m_startSendRtpStruct.recv_app, m_startSendRtpStruct.recv_stream);
				else
  				    sprintf(pRecvMediaSource->sourceURL, "rtp://%s:%s/%s/%s", m_startSendRtpStruct.dst_url, m_startSendRtpStruct.dst_port, m_startSendRtpStruct.recv_app, m_startSendRtpStruct.recv_stream);
 			}
		}
		if (p1078VideoFrameBuffer == NULL)
			p1078VideoFrameBuffer = new unsigned char[Ma1078CacheBufferLength];
		if (netDataCache == NULL)
			netDataCache = new unsigned char[Ma1078CacheBufferLength];
  
		if (Ma1078CacheBufferLength - n1078CacheBufferLength >= nDataLength)
		{
			memcpy(netDataCache + n1078CacheBufferLength, pData, nDataLength);
			n1078CacheBufferLength += nDataLength;
		}
		else
			n1078CacheBufferLength = 0;

		return 0;
	}

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//UDP
	   //获取rtp包的包头
		rtpHeadPtr = (_rtp_header*)(pData);

		//长度合法 并且是rtp包 （rtpHeadPtr->v == 2) ,防止rtcp数据执行rtp解包
		if (nDataLength > 0 && nDataLength < 1500  && rtpHeadPtr->v == 2)
 		  RtpDepacket(pData, nDataLength);
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect  || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{//TCP 
		if (netDataCache == NULL)
			netDataCache = new unsigned char[MaxNetDataCacheCount];

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
					WriteLog(Log_Debug, "CNetGB28181RtpClient = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return 0;
				}
			}
			else
			{//没有剩余，那么 首，尾指针都要复位 
				nNetStart = nNetEnd = netDataCacheLength = 0;
			}
			memcpy(netDataCache + nNetEnd, pData, nDataLength);
			netDataCacheLength += nDataLength;
			nNetEnd += nDataLength;
		}
	}

    return 0;
}

int CNetGB28181RtpClient::ProcessNetData()
{
 	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag.load())
		return -1;
	if (!(strlen(m_startSendRtpStruct.recv_app) > 0 && strlen(m_startSendRtpStruct.recv_stream) > 0))
		return -1;

	if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x34)
	{//jt1078
		if (strcmp(m_startSendRtpStruct.jtt1078_version, "2013") == 0 || strcmp(m_startSendRtpStruct.jtt1078_version, "2016") == 0)
			SplitterJt1078CacheBuffer();
		else if (strcmp(m_startSendRtpStruct.jtt1078_version, "2019") == 0)
			SplitterJt1078CacheBuffer2019();

		return 0;
	}

	unsigned char* pData = NULL;
	int            nLength;

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//UDP
 
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect  || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{//TCP 方式的rtp包读取 
		while (netDataCacheLength > 2048)
		{//不能缓存太多buffer,否则造成接收国标流中只有音频流时，会造成延时很大 2048 比较合适 
			memcpy(rtpHeadOfTCP, netDataCache + nNetStart, 2);
			if ((rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x00) || (rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x01) || (rtpHeadOfTCP[0] == 0x24))
			{
				nNetStart += 2;
				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2;
				netDataCacheLength -= 4;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 2;//4个字节方式
			}
			else
			{
				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2;
				netDataCacheLength -= 2;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 1;//2个字节方式的
			}

			//获取rtp包的包头
			rtpHeadPtr = (_rtp_header*)(netDataCache + nNetStart);

			nRtpLength = ntohs(nRtpLength);
			if (nRtpLength > 65535)
			{
				WriteLog(Log_Debug, "CNetGB28181RtpServer = %X rtp包头长度有误  nClient = %llu ,nRtpLength = %llu", this, nClient, nRtpLength);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
				return -1;
			}

			//长度合法 并且是rtp包 （rtpHeadPtr->v == 2) ,防止rtcp数据执行rtp解包
			if (nRtpLength > 0 && rtpHeadPtr->v == 2)
			{
				//采用rtp头里面的payload进行解包,有效防止用户填写错
				if (hRtpHandle == 0)
					m_gbPayload = rtpHeadPtr->payload;

				RtpDepacket(netDataCache + nNetStart, nRtpLength);
			}

			nNetStart += nRtpLength;
			netDataCacheLength -= nRtpLength;
		}
	}

 	return 0;
}

//发送第一个请求
int CNetGB28181RtpClient::SendFirstRequst()
{//当 gb28181 为tcp时，触发该函数 
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
	{//回复http请求，连接成功，
		sprintf(szResponseBody, "{\"code\":0,\"port\":%d,\"memo\":\"success\",\"key\":%llu}", nReturnPort, nClient);
		ResponseHttp(nClient_http, szResponseBody, false);
	}

	auto pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
	if (pMediaSource != NULL)
	{
		memcpy((char*)&mediaCodecInfo, (char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));
		pMediaSource->AddClientToMap(nClient);
	}

	return 0;
}

//rtp解包回调
void RTP_DEPACKET_CALL_METHOD NetGB28181RtpClient_rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (!pThis->bRunFlag.load())
		return;

	if (pThis->m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
	{//国标PS解包
		if (pThis->psBeiJingLaoChenDemuxer)
			ps_demuxer_input(pThis->psBeiJingLaoChenDemuxer, cb->data, cb->datasize);
	}
	else if (pThis->m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32)
	{//RTP 解包
		if (pThis->pRecvMediaSource == NULL)
		{//rtp解包 
			uint64_t nClientTemp = 1;
			if (pThis->hParent == 0)
				nClientTemp = pThis->nClient;
			else
				nClientTemp = pThis->hParent ;
			pThis->pRecvMediaSource = CreateMediaStreamSource(pThis->m_recvMediaSource, nClientTemp, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pRecvMediaSource != NULL)
			{
				if(strlen(pThis->szClientIP) > 0 )
					sprintf(pThis->pRecvMediaSource->sourceURL, "rtp://%s:%d/%s/%s", pThis->szClientIP,pThis->nClientPort, pThis->m_startSendRtpStruct.recv_app, pThis->m_startSendRtpStruct.recv_stream);
				else
				    sprintf(pThis->pRecvMediaSource->sourceURL, "rtp://%s:%s/%s/%s", pThis->m_startSendRtpStruct.dst_url, pThis->m_startSendRtpStruct.dst_port, pThis->m_startSendRtpStruct.recv_app, pThis->m_startSendRtpStruct.recv_stream);
 				pThis->pRecvMediaSource->enable_mp4 = atoi(pThis->m_startSendRtpStruct.enable_mp4);
				pThis->pRecvMediaSource->enable_hls = atoi(pThis->m_startSendRtpStruct.enable_hls);
				pThis->pRecvMediaSource->fileKeepMaxTime = atoi(pThis->m_startSendRtpStruct.fileKeepMaxTime);
				pThis->pRecvMediaSource->netBaseNetType = pThis->netBaseNetType;
				WriteLog(Log_Debug, "NetGB28181RtpClient_rtppacket_callback_recv 创建媒体源 %s 成功  ", pThis->m_recvMediaSource);
			}
		}

		if (pThis->pRecvMediaSource != NULL)
		{
			if (cb->payload == 98)
				pThis->pRecvMediaSource->PushVideo(cb->data, cb->datasize, "H264");
			else if (cb->payload == 99)
				pThis->pRecvMediaSource->PushVideo(cb->data, cb->datasize, "H265");
			else if (cb->payload == 0)//g711u 
				pThis->pRecvMediaSource->PushAudio(cb->data, cb->datasize, "G711_U", 1, 8000);
			else if (cb->payload == 8)//g711a
				pThis->pRecvMediaSource->PushAudio(cb->data, cb->datasize, "G711_A", 1, 8000);
			else if (cb->payload == 97)//aac
			{
				//获取AAC媒体信息
				if(pThis->nRecvSampleRate == 0 && pThis->nRecvChannels == 0)
				   pThis->GetAACAudioInfo2(cb->data, cb->datasize, &pThis->nRecvSampleRate , &pThis->nRecvChannels);
				if (cb->datasize > 0 && cb->datasize < 2048)
					pThis->pRecvMediaSource->PushAudio((unsigned char*)cb->data, cb->datasize, "AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
			}
		}
	}
}

static void mpeg_ps_dec_NetGB28181RtpClient(void* param, int stream, int codecid, const void* extra, int bytes, int finish)
{
	printf("stream %d, codecid: %d, finish: %s\n", stream, codecid, finish ? "true" : "false");
}
//rtp 解包
struct ps_demuxer_notify_t notify_CNetGB28181RtpClient = { mpeg_ps_dec_NetGB28181RtpClient, };

//国标PS解包回调
static int NetGB28181RtpClient_on_gb28181_unpacket(void* param, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;
	if (!pThis->bRunFlag.load())
		return -1;

	if (pThis->pRecvMediaSource == NULL)
	{//优先创建媒体源 
		uint64_t nClientTemp = 1;
		if (pThis->hParent == 0)
			nClientTemp = pThis->nClient;
		else
			nClientTemp = pThis->hParent;
 		pThis->pRecvMediaSource = CreateMediaStreamSource(pThis->m_recvMediaSource, nClientTemp, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
		if (pThis->pRecvMediaSource != NULL)
		{
			if (strlen(pThis->szClientIP) > 0)//被动方式
				sprintf(pThis->pRecvMediaSource->sourceURL, "rtp://%s:%d/%s/%s", pThis->szClientIP, pThis->nClientPort, pThis->m_startSendRtpStruct.recv_app, pThis->m_startSendRtpStruct.recv_stream);
			else//主动方式 
				sprintf(pThis->pRecvMediaSource->sourceURL, "rtp://%s:%s/%s/%s", pThis->m_startSendRtpStruct.dst_url, pThis->m_startSendRtpStruct.dst_port, pThis->m_startSendRtpStruct.recv_app, pThis->m_startSendRtpStruct.recv_stream);
 			pThis->pRecvMediaSource->enable_mp4 = atoi(pThis->m_startSendRtpStruct.enable_mp4);
			pThis->pRecvMediaSource->enable_hls = atoi(pThis->m_startSendRtpStruct.enable_hls);
			pThis->pRecvMediaSource->fileKeepMaxTime = atoi(pThis->m_startSendRtpStruct.fileKeepMaxTime);
			pThis->pRecvMediaSource->netBaseNetType = pThis->netBaseNetType;
			WriteLog(Log_Debug, "NetGB28181RtpClient_on_gb28181_unpacket 创建媒体源 %s 成功  ", pThis->m_recvMediaSource);
		}
	}

	if (pThis->pRecvMediaSource == NULL)
		return -1;

	if (!pThis->pRecvMediaSource->bUpdateVideoSpeed)
	{//需要更新媒体源的帧速度
		pThis->pRecvMediaSource->UpdateVideoFrameSpeed(25, pThis->netBaseNetType);
		pThis->pRecvMediaSource->bUpdateVideoSpeed = true;
	}

	if (pThis->m_startSendRtpStruct.recv_disableAudio[0] == 0x30  && (PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_G711A == avtype || PSI_STREAM_AUDIO_G711U == avtype))
	{
		if (PSI_STREAM_AAC == avtype)
		{//aac
		    //获取AAC媒体信息
			if (pThis->nRecvSampleRate == 0 && pThis->nRecvChannels == 0)
  				pThis->GetAACAudioInfo2((unsigned char*)data, bytes, &pThis->nRecvSampleRate, &pThis->nRecvChannels);

 			pThis->pRecvMediaSource->PushAudio((unsigned char*)data, bytes, "AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
		}
		else if (PSI_STREAM_AUDIO_G711A == avtype)
		{// G711A  
			pThis->pRecvMediaSource->PushAudio((unsigned char*)data, bytes, "G711_A", 1, 8000);
		}
		else if (PSI_STREAM_AUDIO_G711U == avtype)
		{// G711U  
			pThis->pRecvMediaSource->PushAudio((unsigned char*)data, bytes, "G711_U", 1, 8000);
		}
	}
	else if (pThis->m_startSendRtpStruct.recv_disableVideo[0] == 0x30 && (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype || PSI_STREAM_VIDEO_SVAC == avtype))
	{
#ifdef WriteRecvPSDataFlag
		if (pThis->fWritePSDataFile != NULL )
		{
			fwrite(data, 1, bytes, pThis->fWritePSDataFile);
			fflush(pThis->fWritePSDataFile);
		}
#endif	
 		if (PSI_STREAM_H264 == avtype)
			pThis->pRecvMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
		else if (PSI_STREAM_H265 == avtype)
			pThis->pRecvMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
 	}
	return 0;
}

//rtp解包 
bool  CNetGB28181RtpClient::RtpDepacket(unsigned char* pData, int nDataLength)
{
	if (pData == NULL || nDataLength > 65536 || !bRunFlag.load() || nDataLength < 12)
		return false;

	//创建rtp解包
	if (hRtpHandle == 0)
	{
		rtp_depacket_start(NetGB28181RtpClient_rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle);
 
		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//rtp + PS
			rtp_depacket_setpayload(hRtpHandle, 96, e_rtpdepkt_st_gbps);
  		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 && rtpHeadPtr != NULL)
		{//rtp + ES
			if (rtpHeadPtr->payload == 98)
				rtp_depacket_setpayload(hRtpHandle, rtpHeadPtr->payload, e_rtpdepkt_st_h264);
			else if (rtpHeadPtr->payload == 99)
				rtp_depacket_setpayload(hRtpHandle, rtpHeadPtr->payload, e_rtpdepkt_st_h265);
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//rtp + xhb
			rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_xhb);
		}
		strcpy(m_addStreamProxyStruct.app, m_startSendRtpStruct.recv_app);
		strcpy(m_addStreamProxyStruct.stream, m_startSendRtpStruct.recv_stream);
		sprintf(m_recvMediaSource, "/%s/%s", m_startSendRtpStruct.recv_app, m_startSendRtpStruct.recv_stream);
		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ,创建rtp解包成功 nClient = %llu ,hRtpHandle = %d", this, nClient, hRtpHandle);
	}

	if (psBeiJingLaoChenDemuxer == NULL)
	{
		psBeiJingLaoChenDemuxer = ps_demuxer_create(NetGB28181RtpClient_on_gb28181_unpacket, this);
		if (psBeiJingLaoChenDemuxer != NULL)
		{
			ps_demuxer_set_notify(psBeiJingLaoChenDemuxer, &notify_CNetGB28181RtpClient, this);
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ,创建国标PS解包成功 nClent = %llu ,psBeiJingLaoChenDemuxer = %X", this, nClient, psBeiJingLaoChenDemuxer);
		}
	}

	if (hRtpHandle > 0)
	{//rtp解包
		rtp_depacket_input(hRtpHandle, pData, nDataLength);
	}

	return true;
}

//请求m3u8文件
bool  CNetGB28181RtpClient::RequestM3u8File()
{
	return true;
}

//发送1078视频码流 
void  CNetGB28181RtpClient::SendJtt1078VideoPacket()
{
	unsigned char* pData = NULL;
	int            nLength = 0;
 
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			jt1078VideoHead.pt = 98;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			jt1078VideoHead.pt = 99;
		else
		{
			m_videoFifo.pop_front();
			return ;
		}

		//确定帧类型 
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, pData, nLength))
		{
			jt1078VideoHead.frame_type = 0x00;
			nPFrameCount = 1;
		}
		else
		{
			jt1078VideoHead.frame_type = 0x01;
			nPFrameCount ++;
		}

		//计算总包数量 
		pPacketCount = nLength / JTT1078_MaxPacketLength;
		if (nLength % JTT1078_MaxPacketLength != 0)
			pPacketCount += 1;

		jt1078SendPacketLenth = 0;
		nPacketOrder = 0;
		nSrcVideoPos = 0;
		while (nLength > 0)
		{
			nPacketOrder ++;

			//总长度小于等于950 
			if (pPacketCount == 1)
				jt1078VideoHead.packet_type = 0;
			else
			{//多包 
				if (nPacketOrder == 1)
					jt1078VideoHead.packet_type = 1;//第一包
				else if (nPacketOrder > 1 && nPacketOrder < pPacketCount)
					jt1078VideoHead.packet_type = 3;//中间包
				else
					jt1078VideoHead.packet_type = 2;//最后包
			}

			//计算时间间隔 
			jt1078VideoHead.frame_interval = htons(1000 / mediaCodecInfo.nVideoFrameRate);
			jt1078VideoHead.i_frame_interval = htons(nPFrameCount * (1000 / mediaCodecInfo.nVideoFrameRate));
			//WriteLog(Log_Debug, "i_frame_interval = %d , frame_interval = %d", ntohs(jt1078VideoHead.i_frame_interval), ntohs(jt1078VideoHead.frame_interval));

			if (nLength >= JTT1078_MaxPacketLength)
			{
				jt1078VideoHead.payload_size = htons(JTT1078_MaxPacketLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead, sizeof(jt1078VideoHead));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, JTT1078_MaxPacketLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, JTT1078_MaxPacketLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += JTT1078_MaxPacketLength;
				nSrcVideoPos += JTT1078_MaxPacketLength;
 
				nLength -= JTT1078_MaxPacketLength;
			}
			else
			{
				jt1078VideoHead.payload_size = htons(nLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead, sizeof(jt1078VideoHead));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, nLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, nLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += nLength;
				nSrcVideoPos += nLength;
 
				nLength = 0;
			}

			if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
			{//udp 
				XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, (void*)&gbDstAddr);
				jt1078SendPacketLenth = 0;
			}
			else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
			{//tcp 
				if ((jt1078SendPacketLenth > (MaxGB28181RtpSendVideoMediaBufferLength - 4096)) || nLength == 0 )
				{
					nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, ABL_MediaServerPort.nSyncWritePacket);
					if (nSendRet != 0)
					{
						WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标1078码流出错 ，Length = %d ,nSendRet = %d", this, nLength, nSendRet);
						bRunFlag.exchange(false);
						pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
						return;
					}
					jt1078SendPacketLenth = 0;
				}
			}

			jt1078VideoHead.seq++;
			if (jt1078VideoHead.seq >= 0xFFFF)
				jt1078VideoHead.seq = 0;
 		}

		m_videoFifo.pop_front();
	}
}
//发送1078视频码流 
void  CNetGB28181RtpClient::SendJtt1078VideoPacket2019()
{
	unsigned char* pData = NULL;
	int            nLength = 0;

	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			jt1078VideoHead2019.pt = 98;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			jt1078VideoHead2019.pt = 99;
		else
		{
			m_videoFifo.pop_front();
			return;
		}

		//确定帧类型 
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, pData, nLength))
		{
			jt1078VideoHead2019.frame_type = 0x00;
			nPFrameCount = 1;
		}
		else
		{
			jt1078VideoHead2019.frame_type = 0x01;
			nPFrameCount++;
		}

		//计算总包数量 
		pPacketCount = nLength / JTT1078_MaxPacketLength;
		if (nLength % JTT1078_MaxPacketLength != 0)
			pPacketCount += 1;

		jt1078SendPacketLenth = 0;
		nPacketOrder = 0;
		nSrcVideoPos = 0;
		while (nLength > 0)
		{
			nPacketOrder++;

			//总长度小于等于950 
			if (pPacketCount == 1)
				jt1078VideoHead2019.packet_type = 0;
			else
			{//多包 
				if (nPacketOrder == 1)
					jt1078VideoHead2019.packet_type = 1;//第一包
				else if (nPacketOrder > 1 && nPacketOrder < pPacketCount)
					jt1078VideoHead2019.packet_type = 3;//中间包
				else
					jt1078VideoHead2019.packet_type = 2;//最后包
			}

			//计算时间间隔 
			jt1078VideoHead2019.frame_interval = htons(1000 / mediaCodecInfo.nVideoFrameRate);
			jt1078VideoHead2019.i_frame_interval = htons(nPFrameCount * (1000 / mediaCodecInfo.nVideoFrameRate));
 
			if (nLength >= JTT1078_MaxPacketLength)
			{
				jt1078VideoHead2019.payload_size = htons(JTT1078_MaxPacketLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead2019, sizeof(jt1078VideoHead2019));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead2019);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, JTT1078_MaxPacketLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, JTT1078_MaxPacketLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += JTT1078_MaxPacketLength;
				nSrcVideoPos += JTT1078_MaxPacketLength;

				nLength -= JTT1078_MaxPacketLength;
			}
			else
			{
				jt1078VideoHead2019.payload_size = htons(nLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead2019, sizeof(jt1078VideoHead2019));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead2019);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, nLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, nLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += nLength;
				nSrcVideoPos += nLength;

				nLength = 0;
			}

			if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
			{//udp 
				XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, (void*)&gbDstAddr);
				jt1078SendPacketLenth = 0;
			}
			else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
			{//tcp 
				if ((jt1078SendPacketLenth > (MaxGB28181RtpSendVideoMediaBufferLength - 4096)) || nLength == 0)
				{
					nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, ABL_MediaServerPort.nSyncWritePacket);
					if (nSendRet != 0)
					{
						WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标1078码流出错 ，Length = %d ,nSendRet = %d", this, nLength, nSendRet);
						bRunFlag.exchange(false);
						pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
						return;
					}
					jt1078SendPacketLenth = 0;
				}
			}

			jt1078VideoHead2019.seq++;
			if (jt1078VideoHead2019.seq >= 0xFFFF)
				jt1078VideoHead2019.seq = 0;
		}

		m_videoFifo.pop_front();
	}
}

//发送1078音频码流 
void  CNetGB28181RtpClient::SendJtt1078AduioPacket()
{
	unsigned char* pData = NULL;
	int            nLength = 0;

	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			jt1078AudioHead.pt = 6;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			jt1078AudioHead.pt = 7;
		else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			jt1078AudioHead.pt = 19;
		else
		{
			m_audioFifo.pop_front();
			return;
		}

		jt1078AudioHead.frame_type = 0x03;
		jt1078AudioHead.packet_type = 0x00;
  
		jt1078AudioHead.payload_size = htons(nLength);

		memcpy(szSendRtpVideoMediaBuffer, (char*)&jt1078AudioHead, sizeof(jt1078AudioHead));
 		memcpy(szSendRtpVideoMediaBuffer + sizeof(jt1078AudioHead), pData, nLength);
    
		if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
		{//udp 
			XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead), (void*)&gbDstAddr);
 		}
		else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
		{//tcp 
			nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead), ABL_MediaServerPort.nSyncWritePacket);
			if (nSendRet != 0)
			{
				WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标1078码流出错 ，Length = %d ,nSendRet = %d", this, nLength, nSendRet);
				bRunFlag.exchange(false);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return;
			}
  		}

		jt1078AudioHead.seq++;
		if (jt1078AudioHead.seq >= 0xFFFF)
			jt1078AudioHead.seq = 0;
 
		m_audioFifo.pop_front();
	}
}
//发送1078音频码流 
void  CNetGB28181RtpClient::SendJtt1078AduioPacket2019()
{
	unsigned char* pData = NULL;
	int            nLength = 0;

	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			jt1078AudioHead2019.pt = 6;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			jt1078AudioHead2019.pt = 7;
		else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			jt1078AudioHead2019.pt = 19;
		else
		{
			m_audioFifo.pop_front();
			return;
		}

		jt1078AudioHead2019.frame_type = 0x03;
		jt1078AudioHead2019.packet_type = 0x00;

		jt1078AudioHead2019.payload_size = htons(nLength);

		memcpy(szSendRtpVideoMediaBuffer, (char*)&jt1078AudioHead2019, sizeof(jt1078AudioHead2019));
		memcpy(szSendRtpVideoMediaBuffer + sizeof(jt1078AudioHead2019), pData, nLength);

		if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
		{//udp 
			XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead2019), (void*)&gbDstAddr);
		}
		else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
		{//tcp 
			nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead2019), ABL_MediaServerPort.nSyncWritePacket);
			if (nSendRet != 0)
			{
				WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, 发送国标1078码流出错 ，Length = %d ,nSendRet = %d", this, nLength, nSendRet);
				bRunFlag.exchange(false);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return;
			}
		}

		jt1078AudioHead2019.seq++;
		if (jt1078AudioHead2019.seq >= 0xFFFF)
			jt1078AudioHead2019.seq = 0;

		m_audioFifo.pop_front();
	}
}

//对1078数据进行切割 
void  CNetGB28181RtpClient::SplitterJt1078CacheBuffer()
{
	Jt1078VideoRtpPacket_T*      p1078VideoHead;
	Jt1078AudioRtpPacket_T*      p1078AudioHead;
	Jt1078OtherRtpPacket_T*      pOtherHead;

	n1078Pos = 0;
	n1078CurrentProcCountLength = n1078CacheBufferLength;//当前处理的总长度 
	nStartProcessJtt1078Time = GetTickCount64();
	while (n1078CacheBufferLength >= 4096 && pRecvMediaSource != NULL && (GetTickCount64() - nStartProcessJtt1078Time) <= 1000 * 3)
	{
		p1078VideoHead = (Jt1078VideoRtpPacket_T*)(netDataCache + n1078Pos);

		//判断标志头
		if (!(netDataCache[n1078Pos] == 0x30 && netDataCache[n1078Pos + 1] == 0x31 && netDataCache[n1078Pos + 2] == 0x63 && netDataCache[n1078Pos + 3] == 0x64))
		{
			nFind1078FlagPos = Find1078HeadFromCacheBuffer(netDataCache + n1078Pos, n1078CurrentProcCountLength - n1078Pos);
			if (nFind1078FlagPos < 0)
			{//数据有错误，丢弃所有数据 
				n1078CacheBufferLength = 0;
				return;
			}
			else
			{//找到标志位置，重新定位
				n1078NewPosition = n1078Pos + nFind1078FlagPos;// n1078Pos 为已经消费的数据位置、nFind1078FlagPos 因为丢包造成一些错乱数据 从而丢弃的数据 的新位置 
				memmove(netDataCache, netDataCache + n1078NewPosition, n1078CurrentProcCountLength - n1078NewPosition);
				return; //等待下一次对1078数据进行解包 
			}
		}

		if (p1078VideoHead->frame_type == 0 || p1078VideoHead->frame_type == 1 || p1078VideoHead->frame_type == 2)
		{//视频
			nPayloadSize = ntohs(p1078VideoHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
#ifdef WriteJt1078VideoFlag
				fwrite((unsigned char*)(netDataCache + (sizeof(Jt1078VideoRtpPacket_T) + n1078Pos)), 1, nPayloadSize, fWrite1078File);
				fflush(fWrite1078File);
#endif
				if (Ma1078CacheBufferLength - n1078VideoFrameBufferLength >= nPayloadSize)
				{
					memcpy(p1078VideoFrameBuffer + n1078VideoFrameBufferLength, netDataCache + (sizeof(Jt1078VideoRtpPacket_T) + n1078Pos), nPayloadSize);
					n1078VideoFrameBufferLength += nPayloadSize;

					if (nVideoPT == -1)
						nVideoPT = p1078VideoHead->pt;

					if (p1078VideoHead->packet_type == 0 && m_startSendRtpStruct.recv_disableVideo[0] == 0x30)
					{//独立一包，不可分隔
						if (nVideoPT == 98)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0;
					}
					else if (p1078VideoHead->packet_type == 2 && m_startSendRtpStruct.recv_disableVideo[0] == 0x30)
					{//最后1包
						if (nVideoPT == 98)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0;
					}

					if (!pRecvMediaSource->bUpdateVideoSpeed)
					{//更新视频源的帧速度
						if (ntohs(p1078VideoHead->frame_interval) > 0 && ntohs(p1078VideoHead->frame_interval) <= 1000)
						{//修改为只要分析出视频编码名字即可进行更新视频帧速度
							bUpdateVideoFrameSpeedFlag = true;
							sprintf(pRecvMediaSource->sim, "%02X%02X%02X%02X%02X%02X", p1078VideoHead->sim[0], p1078VideoHead->sim[1], p1078VideoHead->sim[2], p1078VideoHead->sim[3], p1078VideoHead->sim[4], p1078VideoHead->sim[5]);
							UpdateSim(pRecvMediaSource->sim);
  							pRecvMediaSource->UpdateVideoFrameSpeed(1000 / ntohs(p1078VideoHead->frame_interval), netBaseNetType);
							auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
						if (GetTickCount64() - nCreateDateTime >= 5000)
						{//如果计算失败，立即设置一个默认的帧速度 
							bUpdateVideoFrameSpeedFlag = true;
							pRecvMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
							auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
					}
				}
				else
					n1078VideoFrameBufferLength = 0;

				n1078CacheBufferLength -= sizeof(Jt1078VideoRtpPacket_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078VideoRtpPacket_T) + nPayloadSize;
			}
		}
		else if (p1078VideoHead->frame_type == 3)
		{//音频 
			Jt1078AudioRtpPacket_T* p1078AudioHead = (Jt1078AudioRtpPacket_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(p1078AudioHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
				if (nAudioPT == -1)
					nAudioPT = p1078AudioHead->pt;

				if (nAudioPT == 6 && m_startSendRtpStruct.recv_disableAudio[0] == 0x30)
				{
					if (nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead, 4) == 0)
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos + 4)), nPayloadSize - 4, "G711_A", 1, 8000);
					else
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, "G711_A", 1, 8000);
				}
				else if (nAudioPT == 7 && m_startSendRtpStruct.recv_disableAudio[0] == 0x30)
				{
					if (nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead, 4) == 0)
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos + 4)), nPayloadSize - 4, "G711_U", 1, 8000);
					else
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, "G711_U", 1, 8000);
				}
				else if (nAudioPT == 19 && m_startSendRtpStruct.recv_disableAudio[0] == 0x30)
				{//准确计算aac音频格式 

				 //获取AAC格式
					if (nRecvChannels == 0 && nRecvSampleRate == 0)
						GetAACAudioInfo2((netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, &nRecvSampleRate, &nRecvChannels);

					pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, "AAC", nRecvChannels, nRecvSampleRate);
				}

				n1078CacheBufferLength -= sizeof(Jt1078AudioRtpPacket_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078AudioRtpPacket_T) + nPayloadSize;
			}

			if (bUpdateVideoFrameSpeedFlag == false && (GetTickCount64() - nCreateDateTime >= 5000))
			{//如果计算失败，立即设置一个默认的帧速度 
				bUpdateVideoFrameSpeedFlag = true;
 				pRecvMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
				auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
			}
		}
		else if (p1078VideoHead->frame_type == 4)
		{//透传数据
			Jt1078OtherRtpPacket_T* pOtherHead = (Jt1078OtherRtpPacket_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(pOtherHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{

				n1078CacheBufferLength -= sizeof(Jt1078OtherRtpPacket_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078OtherRtpPacket_T) + nPayloadSize;
			}
		}
	}

	//把剩余的数据移动过来
	if (n1078CacheBufferLength > 0 && n1078Pos > 0)
		memmove(netDataCache, netDataCache + n1078Pos, n1078CacheBufferLength);
}

//对1078数据进行切割 
void  CNetGB28181RtpClient::SplitterJt1078CacheBuffer2019()
{
	Jt1078VideoRtpPacket2019_T*      p1078VideoHead;
	Jt1078AudioRtpPacket2019_T*      p1078AudioHead;
	Jt1078OtherRtpPacket2019_T*      pOtherHead;

	n1078Pos = 0;
	n1078CurrentProcCountLength = n1078CacheBufferLength;//当前处理的总长度 
	nStartProcessJtt1078Time = GetTickCount64();
	while (n1078CacheBufferLength >= 4096 && pRecvMediaSource != NULL && (GetTickCount64() - nStartProcessJtt1078Time) <= 1000 * 3)
	{
		p1078VideoHead = (Jt1078VideoRtpPacket2019_T*)(netDataCache + n1078Pos);

		//判断标志头
		if (!(netDataCache[n1078Pos] == 0x30 && netDataCache[n1078Pos + 1] == 0x31 && netDataCache[n1078Pos + 2] == 0x63 && netDataCache[n1078Pos + 3] == 0x64))
		{
			nFind1078FlagPos = Find1078HeadFromCacheBuffer(netDataCache + n1078Pos, n1078CurrentProcCountLength - n1078Pos);
			if (nFind1078FlagPos < 0)
			{//数据有错误，丢弃所有数据 
				n1078CacheBufferLength = 0;
				return;
			}
			else
			{//找到标志位置，重新定位
				n1078NewPosition = n1078Pos + nFind1078FlagPos;// n1078Pos 为已经消费的数据位置、nFind1078FlagPos 因为丢包造成一些错乱数据 从而丢弃的数据 的新位置 
				memmove(netDataCache, netDataCache + n1078NewPosition, n1078CurrentProcCountLength - n1078NewPosition);
				return; //等待下一次对1078数据进行解包 
			}
		}

		if (p1078VideoHead->frame_type == 0 || p1078VideoHead->frame_type == 1 || p1078VideoHead->frame_type == 2)
		{//视频
			nPayloadSize = ntohs(p1078VideoHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
#ifdef WriteJt1078VideoFlag
				fwrite((unsigned char*)(netDataCache + (sizeof(Jt1078VideoRtpPacket2019_T) + n1078Pos)), 1, nPayloadSize, fWrite1078File);
				fflush(fWrite1078File);
#endif
				if (Ma1078CacheBufferLength - n1078VideoFrameBufferLength >= nPayloadSize)
				{
					memcpy(p1078VideoFrameBuffer + n1078VideoFrameBufferLength, netDataCache + (sizeof(Jt1078VideoRtpPacket2019_T) + n1078Pos), nPayloadSize);
					n1078VideoFrameBufferLength += nPayloadSize;

					if (nVideoPT == -1)
						nVideoPT = p1078VideoHead->pt;

					if (p1078VideoHead->packet_type == 0 && m_startSendRtpStruct.recv_disableVideo[0] == 0x30)
					{//独立一包，不可分隔
						if (nVideoPT == 98)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0;
					}
					else if (p1078VideoHead->packet_type == 2 && m_startSendRtpStruct.recv_disableVideo[0] == 0x30)
					{//最后1包
						if (nVideoPT == 98)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pRecvMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0;
					}

					if (!pRecvMediaSource->bUpdateVideoSpeed)
					{//更新视频源的帧速度
						if (ntohs(p1078VideoHead->frame_interval) > 0 && ntohs(p1078VideoHead->frame_interval) <= 1000)
						{
							bUpdateVideoFrameSpeedFlag = true;
							sprintf(pRecvMediaSource->sim, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", p1078VideoHead->sim[0], p1078VideoHead->sim[1], p1078VideoHead->sim[2], p1078VideoHead->sim[3], p1078VideoHead->sim[4], p1078VideoHead->sim[5], p1078VideoHead->sim[6], p1078VideoHead->sim[7], p1078VideoHead->sim[8], p1078VideoHead->sim[9]);
							UpdateSim(pRecvMediaSource->sim);
							pRecvMediaSource->UpdateVideoFrameSpeed(1000 / ntohs(p1078VideoHead->frame_interval), netBaseNetType);
							auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
						if (GetTickCount64() - nCreateDateTime >= 5000)
						{//如果计算失败，立即设置一个默认的帧速度 
							bUpdateVideoFrameSpeedFlag = true;
							pRecvMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
							auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
					}
				}
				else
					n1078VideoFrameBufferLength = 0;

				n1078CacheBufferLength -= sizeof(Jt1078VideoRtpPacket2019_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078VideoRtpPacket2019_T) + nPayloadSize;
			}
		}
		else if (p1078VideoHead->frame_type == 3)
		{//音频 
			Jt1078AudioRtpPacket2019_T* p1078AudioHead = (Jt1078AudioRtpPacket2019_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(p1078AudioHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
				if (nAudioPT == -1)
					nAudioPT = p1078AudioHead->pt;

				if (nAudioPT == 6 && m_startSendRtpStruct.recv_disableAudio[0] == 0x30)
				{
					if (nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead, 4) == 0)
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos + 4)), nPayloadSize - 4, "G711_A", 1, 8000);
					else
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, "G711_A", 1, 8000);
				}
				else if (nAudioPT == 7 && m_startSendRtpStruct.recv_disableAudio[0] == 0x30)
				{
					if (nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead, 4) == 0)
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos + 4)), nPayloadSize - 4, "G711_U", 1, 8000);
					else
						pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, "G711_U", 1, 8000);
				}
				else if (nAudioPT == 19 && m_startSendRtpStruct.recv_disableAudio[0] == 0x30)
				{//准确计算aac音频格式 
				 //获取AAC格式
					if (nRecvChannels == 0 && nRecvSampleRate == 0)
						GetAACAudioInfo2((netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, &nRecvSampleRate, &nRecvChannels);

					pRecvMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, "AAC", nRecvChannels, nRecvSampleRate);
				}

				n1078CacheBufferLength -= sizeof(Jt1078AudioRtpPacket2019_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078AudioRtpPacket2019_T) + nPayloadSize;
			}

			if (bUpdateVideoFrameSpeedFlag == false && (GetTickCount64() - nCreateDateTime >= 5000))
			{//如果计算失败，立即设置一个默认的帧速度 
				bUpdateVideoFrameSpeedFlag = true;
				pRecvMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
				auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
			}
		}
		else if (p1078VideoHead->frame_type == 4)
		{//透传数据
			Jt1078OtherRtpPacket2019_T* pOtherHead = (Jt1078OtherRtpPacket2019_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(pOtherHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{

				n1078CacheBufferLength -= sizeof(Jt1078OtherRtpPacket2019_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078OtherRtpPacket2019_T) + nPayloadSize;
			}
		}
	}

	//把剩余的数据移动过来
	if (n1078CacheBufferLength > 0 && n1078Pos > 0)
		memmove(netDataCache, netDataCache + n1078Pos, n1078CacheBufferLength);
}

//在缓存中查找1078标志 0x30 ,0x31 ,0x63, 0x64 
int   CNetGB28181RtpClient::Find1078HeadFromCacheBuffer(unsigned char* pData, int nLength)
{
	if (nLength <= 4 || pData == NULL)
		return -1;

	for (int i = 0; i < nLength - 4; i++)
	{
		if (pData[i] == 0x30 && pData[i + 1] == 0x31 && pData[i + 2] == 0x63 && pData[i + 3] == 0x64)
		{
			return i;
		}
	}
	return -2;
}
