/*
功能：
       实现
        rtsp接收 
日期    2021-07-24
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientRecvRtsp.h"
#include "LCbase64.h"

#include "netBase64.h"
#include "Base64.hh"
#ifdef USE_BOOST
uint64_t                                     CNetClientRecvRtsp::Session = 1000;
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#else
uint64_t                                     CNetClientRecvRtsp::Session = 1000;
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#endif

extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 

extern size_t base64_decode(void* target, const char *source, size_t bytes);
extern MediaServerPort                       ABL_MediaServerPort;
static const uint8_t                         start_code[4] = { 0, 0, 0, 1 };
extern CMediaFifo                            pMessageNoticeFifo;          //消息通知FIFO

//AAC采样频率序号
extern int avpriv_mpeg4audio_sample_rates[];

extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result, uint16_t nLocalPort);

extern void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);

static void* rtp_alloc(void* param, int bytes)
{
	CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)param;
	WriteLog(Log_Debug, "rtp_alloc() 分配 rtp 视频资源  ");

	return pRtsp->szCallBackVideo;
}

static void* rtp_alloc2(void* param, int bytes)
{
	CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)param;

	WriteLog(Log_Debug, "rtp_alloc2() 分配 rtp 音频资源  ");

	return pRtsp->szCallBackAudio;
}


static void rtp_free(void* param, void * packet)
{

}

static int rtp_decode_packet(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)param;
	if (pRtsp == NULL || !pRtsp->bRunFlag )
		return -1;

	//如果rtp负载PS ，需要PS解包 ，海康NVR录像回放采用这样方式 
	if (pRtsp->nRtspRtpPayloadType == RtspRtpPayloadType_PS && pRtsp->psHandle > 0)
	{
		ps_demux_input(pRtsp->psHandle,(unsigned char*)packet,bytes);
		return 0 ;
	}
  
	if (pRtsp->m_addStreamProxyStruct.disableVideo[0] == 0x30 && (0 == strcmp("H264", pRtsp->szVideoName) || 0 == strcmp("H265", pRtsp->szVideoName)))
	{//不过滤视频
		if (MaxNetDataCacheBufferLength - pRtsp->cbVideoLength > (bytes + 4))
		{
			if (pRtsp->cbVideoTimestamp != 0 && pRtsp->cbVideoTimestamp != timestamp)
			{
 				pRtsp->nRecvDataTimerBySecond = 0;//网络断线检测

				if (pRtsp->m_nSpsPPSLength > 0 && pRtsp->CheckVideoIsIFrame(pRtsp->szVideoName, pRtsp->szCallBackVideo, pRtsp->cbVideoLength))
					pRtsp->pMediaSource->PushVideo((unsigned char*)pRtsp->m_pSpsPPSBuffer, pRtsp->m_nSpsPPSLength, pRtsp->szVideoName);

				pRtsp->pMediaSource->PushVideo(pRtsp->szCallBackVideo, pRtsp->cbVideoLength, pRtsp->szVideoName);

				if (ABL_MediaServerPort.nSaveProxyRtspRtp == 1 && (GetTickCount64() - pRtsp->nCreateDateTime) < 1000 * 180)
				{
					char szVFile[string_length_1024];
					if (pRtsp->fWriteESStream == NULL)
					{
						if (0 == strcmp("H264", pRtsp->szVideoName))
							sprintf(szVFile, "%s%llu_%X.264", ABL_MediaServerPort.debugPath, pRtsp->nClient, pRtsp);
						else if ( 0 == strcmp("H265", pRtsp->szVideoName))
							sprintf(szVFile, "%s%llu_%X.265", ABL_MediaServerPort.debugPath, pRtsp->nClient, pRtsp);
						pRtsp->fWriteESStream = fopen(szVFile, "wb"); 
					}

					if (pRtsp->fWriteESStream)
					{
						fwrite(pRtsp->szCallBackVideo, 1, pRtsp->cbVideoLength, pRtsp->fWriteESStream);
						fflush(pRtsp->fWriteESStream);
					}
  				}

				pRtsp->cbVideoLength = 0;
			}

			memcpy(pRtsp->szCallBackVideo + pRtsp->cbVideoLength, start_code, sizeof(start_code));
			pRtsp->cbVideoLength += 4;
			memcpy(pRtsp->szCallBackVideo + pRtsp->cbVideoLength, packet, bytes);
			pRtsp->cbVideoLength += bytes;
 		}
		else
		{
			pRtsp->cbVideoLength = 0;
		}
		pRtsp->cbVideoTimestamp = timestamp ;
  	} 

	return  0;
}

static int rtp_decode_packetAudio(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	CNetClientRecvRtsp* pRtsp = (CNetClientRecvRtsp*)param;
	if (pRtsp == NULL || !pRtsp->bRunFlag || pRtsp->m_addStreamProxyStruct.disableAudio[0] != 0x30)
		return -1;

	pRtsp->nRecvDataTimerBySecond = 0;//网络断线检测

	size_t size = 0;
	if (0 == strcmp("mpeg4-generic", pRtsp->szSdpAudioName))
	{
		int len = bytes + 7;
		uint8_t profile = 2;
		uint8_t sampling_frequency_index = pRtsp->sample_index;
		uint8_t channel_configuration = pRtsp->nChannels;
 		pRtsp->szCallBackAudio[0] = 0xFF; /* 12-syncword */
		pRtsp->szCallBackAudio[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
		pRtsp->szCallBackAudio[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
		pRtsp->szCallBackAudio[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
		pRtsp->szCallBackAudio[4] = (uint8_t)(len >> 3);
		pRtsp->szCallBackAudio[5] = ((len & 0x07) << 5) | 0x1F;
		pRtsp->szCallBackAudio[6] = 0xFC | ((len / 1024) & 0x03);
 		size = 7;
		memcpy(pRtsp->szCallBackAudio + size, packet, bytes);
		size += bytes;

		pRtsp->pMediaSource->PushAudio(pRtsp->szCallBackAudio, size, pRtsp->szAudioName, pRtsp->nChannels, pRtsp->nSampleRate);
	}
	else // g711a,g711u 
		pRtsp->pMediaSource->PushAudio((unsigned char*)packet, bytes, pRtsp->szAudioName, pRtsp->nChannels, pRtsp->nSampleRate);

	return  0;
}

void PS_DEMUX_CALL_METHOD NetClientRtspRecv_demux_callback(_ps_demux_cb* cb)
{
	CNetClientRecvRtsp* pThis = (CNetClientRecvRtsp*)cb->userdata;
	if(!pThis->bRunFlag)
		return ;
	//WriteLog(Log_Debug, "cb->type = %d ,Length =%d ", cb->streamtype,cb->datasize);
	pThis->nRecvDataTimerBySecond = 0;//网络断线检测

	if (pThis && cb->streamtype == e_rtpdepkt_st_h264 || cb->streamtype == e_rtpdepkt_st_h265 ||
		cb->streamtype == e_rtpdepkt_st_mpeg4 || cb->streamtype == e_rtpdepkt_st_mjpeg)
	{
		if (cb->streamtype == e_rtpdepkt_st_h264)
			pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
		else if (cb->streamtype == e_rtpdepkt_st_h265)
			pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");

#ifdef   WriteHIKPsPacketData
		if (pThis->fWritePS != NULL)
		{
 			fwrite(cb->data, 1, cb->datasize, pThis->fWritePS);
			fflush(pThis->fWritePS);
		}
#endif
	}
	else if (pThis && pThis->m_addStreamProxyStruct.disableAudio[0] == 0x30)
	{
		if (strcmp(pThis->szAudioName, "AAC") == 0)
		{//aac
			pThis->SplitterRtpAACData(cb->data, cb->datasize);
		}
		else
		{// G711A 、G711U
			pThis->pMediaSource->PushAudio(cb->data, cb->datasize, pThis->szAudioName, pThis->nChannels, pThis->nSampleRate);
		}
	}
}

//对AAC的rtp包进行切割
void  CNetClientRecvRtsp::SplitterRtpAACData(unsigned char* rtpAAC, int nLength)
{
	au_header_length = (rtpAAC[0] << 8) + rtpAAC[1];
	au_header_length = (au_header_length + 7) / 8;  
	ptr = rtpAAC;

	au_size = 2;  
	au_numbers = au_header_length / au_size;

	ptr += 2;  
	pau = ptr + au_header_length;  

	for (int i = 0; i < au_numbers; i++)
	{
		SplitterSize[i] = (ptr[0] << 8) | (ptr[1] & 0xF8);
		SplitterSize[i] = SplitterSize[i] >> 3; 

 		if (SplitterSize[i] > nLength || SplitterSize[i] <= 0 )
		{
			WriteLog(Log_Debug, "CNetClientRecvRtsp=%X ,nClient = %llu, rtp 切割长度 有误 ", this, nClient);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return;
		}
	 
		 AddADTSHeadToAAC((unsigned char*)pau, SplitterSize[i]);

		if (netBaseNetType == NetBaseNetType_RtspClientRecv && pMediaSource != NULL)
		{
		   pMediaSource->PushAudio(aacData, SplitterSize[i] +7, szAudioName, nChannels,nSampleRate);

		   if (ABL_MediaServerPort.nSaveProxyRtspRtp == 1  && fWriteRtpAudio && GetTickCount64() - nCreateDateTime <= 1000 * 180)
		   {
			   fwrite(aacData, 1, SplitterSize[i] + 7, fWriteRtpAudio);
			   fflush(fWriteRtpAudio);
		   }
		}
 
		ptr += au_size;
		pau += SplitterSize[i];
	}
}

//追加adts信息头
void  CNetClientRecvRtsp::AddADTSHeadToAAC(unsigned char* szData, int nAACLength)
{
	int len = nAACLength + 7;
	uint8_t profile = 2;
	uint8_t sampling_frequency_index = sample_index;
	uint8_t channel_configuration = nChannels;
	aacData[0] = 0xFF; /* 12-syncword */
	aacData[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
	aacData[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
	aacData[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
	aacData[4] = (uint8_t)(len >> 3);
	aacData[5] = ((len & 0x07) << 5) | 0x1F;
	aacData[6] = 0xFC | ((len / 1024) & 0x03);

	memcpy(aacData + 7, szData, nAACLength);
}

CNetClientRecvRtsp::CNetClientRecvRtsp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	memset(m_szContentBaseURL, 0x00, sizeof(m_szContentBaseURL));
	nRecvRtpPacketCount = nMaxRtpLength = 0;
	nSendOptionsHeartbeatTimer = GetTickCount64();
	fWriteRtpVideo = fWriteRtpAudio = fWriteESStream = NULL;
	m_nXHRtspURLType = XHRtspURLType_Liveing;
	m_bPauseFlag = false;
	nServer = hServer;
	nClient = hClient;
	hRtpVideo = 0;
	hRtpAudio = 0;
	nSendRtpFailCount = 0;//累计发送rtp包失败次数 
	strcpy(m_szShareMediaURL, szShareMediaURL);

	nPrintCount = 0;
	pMediaSource = NULL;
	bRunFlag = true;
	bIsInvalidConnectFlag = false;

	netDataCacheLength = 0;//网络数据缓存大小
	nNetStart = nNetEnd = 0; //网络数据起始位置\结束位置
	MaxNetDataCacheCount = MaxNetDataCacheBufferLength ;
	data_Length = 0;

	nRecvLength = 0;
	memset((char*)szHttpHeadEndFlag, 0x00, sizeof(szHttpHeadEndFlag));
	strcpy((char*)szHttpHeadEndFlag, "\r\n\r\n");
	nHttpHeadEndLength = 0;
	nContentLength = 0;
	memset(szResponseHttpHead, 0x00, sizeof(szResponseHttpHead));

	m_bHaveSPSPPSFlag = false;
	m_nSpsPPSLength = 0;
	memset(s_extra_data,0x00,sizeof(s_extra_data));
	extra_data_size = 0;

	RtspProtectArrayOrder = 0;
	for (int i = 0; i < MaxRtspProtectCount; i++)
		memset((char*)&RtspProtectArray[i], 0x00, sizeof(RtspProtectArray[i]));

	memset(szCallBackAudio, 0x00, sizeof(szCallBackAudio));
	szCallBackVideo = new unsigned char[MaxNetDataCacheBufferLength];
	for (int i = 0; i < MaxRtpHandleCount; i++)
		rtpDecoder[i] = NULL;

	for (int i = 0; i < 3; i++)
		bExitProcessFlagArray[i] = true;

	m_nSpsPPSLength = 0;
	AuthenticateType = WWW_Authenticate_None;
	nSendSetupCount = 0;
	netBaseNetType = NetBaseNetType_RtspClientRecv;

	for (int i = 0; i < 16; i++)
		memset(szTrackIDArray[i], 0x00, sizeof(szTrackIDArray[i]));

	if (ParseRtspRtmpHttpURL(szIP) == true)
	  uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);

	strcpy(szClientIP, m_rtspStruct.szIP);
	nClientPort = atoi(m_rtspStruct.szPort);

	nVideoSSRC = rand();
	nCurrentTime = GetTickCount64();
	//检测是否是大华摄像机
	if (strstr(m_rtspStruct.szSrcRtspPullUrl, "realmonitor") != NULL)
	{//大华摄像头实况url地址包含  realmonitor  
		bSendRRReportFlag = true;
    }
	bSendRRReportFlag = true;
	nVideoSSRC = audioSSRC = 0;

	psHandle = 0;

	cbVideoTimestamp = 0 ;//回调时间戳
	cbVideoLength = 0 ;//回调视频累计
	nCreateDateTime = GetTickCount64();

	if (ABL_MediaServerPort.nSaveProxyRtspRtp == 1)
	{
		char szVFile[string_length_1024];
		sprintf(szVFile, "%s%llu_%X.rtp", ABL_MediaServerPort.debugPath, nClient, this);
		fWriteRtpVideo = fopen(szVFile, "wb");

		sprintf(szVFile, "%s%llu_%X.aac", ABL_MediaServerPort.debugPath, nClient, this);
		fWriteRtpAudio = fopen(szVFile, "wb");
		bStartWriteFlag = false;
	}

#ifdef           WriteHIKPsPacketData
	fWritePS = fopen("D:\\HIK_2022-02-25.264","wb");
#endif

	WriteLog(Log_Debug, "CNetClientRecvRtsp 构造 nClient = %llu ", nClient);
}

CNetClientRecvRtsp::~CNetClientRecvRtsp()
{
	bRunFlag = false;
	WriteLog(Log_Debug, "CNetClientRecvRtsp 等待任务退出 nTime = %llu, nClient = %llu , app = %s ,stream = %s ,bUpdateVideoFrameSpeedFlag = %d",GetTickCount64(), nClient,m_addStreamProxyStruct.app,m_addStreamProxyStruct.stream, bUpdateVideoFrameSpeedFlag);
	std::lock_guard<std::mutex> lock(netDataLock);

	//服务器异常断开
	if (bUpdateVideoFrameSpeedFlag == false)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"faied. Abnormal didconnection \",\"key\":%llu}", IndexApiCode_RecvRtmpFailed, 0);
		ResponseHttp2(nClient_http, szResponseBody, false);
	}

	for (int i = 0; i < 3; i++)
	{
		while (!bExitProcessFlagArray[i])
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			//Sleep(5);
	}
	WriteLog(Log_Debug, "CNetClientRecvRtsp 任务退出完毕 nTime = %llu, nClient = %llu ",GetTickCount64(), nClient);
 
	if(psHandle > 0)
		ps_demux_stop(psHandle);
 
	for (int i = 0; i < MaxRtpHandleCount; i++)
	{
		if (rtpDecoder[i] != NULL)
			rtp_payload_decode_destroy(rtpDecoder[i]);
	}

	if (ABL_MediaServerPort.nSaveProxyRtspRtp == 1 )
	{
		if (fWriteRtpVideo)
			fclose(fWriteRtpVideo);
		if (fWriteRtpAudio)
			fclose(fWriteRtpAudio);
		if (fWriteESStream)
			fclose(fWriteESStream);
	}


	SAFE_ARRAY_DELETE(szCallBackVideo);

#ifdef   WriteHIKPsPacketData
	fclose(fWritePS);
#endif

	//码流没有达到通知
	if (ABL_MediaServerPort.hook_enable == 1  && bUpdateVideoFrameSpeedFlag == false)
	{
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_not_arrive;
 		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_not_arrive\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu}", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, hParent);
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}


	//删除分发源
	if (strlen(m_szShareMediaURL) > 0 && pMediaSource != NULL )
		DeleteMediaStreamSource(m_szShareMediaURL);


	WriteLog(Log_Debug, "CNetClientRecvRtsp 析构 nClient = %llu \r\n", nClient);
	malloc_trim(0);
}
int CNetClientRecvRtsp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	return 0;
}

int CNetClientRecvRtsp::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	return 0;
}

int CNetClientRecvRtsp::SendVideo()
{

	return 0;
}

int CNetClientRecvRtsp::SendAudio()
{

	return 0;
}

//网络数据拼接 
int CNetClientRecvRtsp::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(netDataLock);

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
				WriteLog(Log_Debug, "CNetClientRecvRtsp = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
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
 	return 0 ;
}

//读取网络数据 ，模拟原来底层网络库读取函数 
int32_t  CNetClientRecvRtsp::XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain)
{
	int nWaitCount = 0;
	bExitProcessFlagArray[0] = false;
	while (!bIsInvalidConnectFlag && bRunFlag)
	{
		if (netDataCacheLength >= *buffsize)
		{
			memcpy(buffer, netDataCache + nNetStart, *buffsize);
			nNetStart += *buffsize;
			netDataCacheLength -= *buffsize;
			
			bExitProcessFlagArray[0] = true;
			return 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		
		//Sleep(20);
		nWaitCount ++;
		if (nWaitCount >= 100 * 5)
			break;
	}
	bExitProcessFlagArray[0] = true;

	return -1;  
}

bool   CNetClientRecvRtsp::ReadRtspEnd()
{
	unsigned int nReadLength = 1;
	unsigned int nRet;
	bool     bRet = true ;
	bExitProcessFlagArray[1] = false;
	while (!bIsInvalidConnectFlag && bRunFlag)
	{
		nReadLength = 1;
		nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
		if (nRet == 0 && nReadLength == 1)
		{
			data_Length += 1;
 			if (data_Length >= 4 &&  data_[data_Length - 4] == '\r' && data_[data_Length - 3] == '\n' && data_[data_Length - 2] == '\r' && data_[data_Length - 1] == '\n')
			{
				bRet = true;
				break;
			}
		}
		else
		{
			WriteLog(Log_Debug, "ReadRtspEnd() ,尚未读取到数据 ！CABLRtspClient =%X ,dwClient=%llu ", this, nClient);
			break;
		}

		if (data_Length >= RtspServerRecvDataLength)
		{
			WriteLog(Log_Debug, "ReadRtspEnd() ,找不到 rtsp 结束符号 ！CABLRtspClient =%X ,dwClient = %llu ", this, nClient);
			break;
		}
	}
	bExitProcessFlagArray[1] = true;
	return bRet;
}

//查找
int  CNetClientRecvRtsp::FindHttpHeadEndFlag()
{
	if (data_Length <= 0)
		return -1 ;

	for (int i = 0; i < data_Length; i++)
	{
		if (memcmp(data_ + i, szHttpHeadEndFlag, 4) == 0)
		{
			nHttpHeadEndLength = i + 4;
			return nHttpHeadEndLength;
		}
	}
	return -1;
}

int  CNetClientRecvRtsp::FindKeyValueFlag(char* szData)
{
	int nSize = strlen(szData);
	for (int i = 0; i < nSize; i++)
	{
		if (memcmp(szData + i, ": ", 2) == 0)
			return i;
	}
	return -1;
}

//获取HTTP方法，httpURL 
void CNetClientRecvRtsp::GetHttpModemHttpURL(char* szMedomHttpURL)
{//"POST /getUserName?userName=admin&password=123456 HTTP/1.1"
	if (RtspProtectArrayOrder >= MaxRtspProtectCount)
		return;

	int nPos1, nPos2, nPos3, nPos4;
	string strHttpURL = szMedomHttpURL;
	char   szTempRtsp[string_length_2048] = { 0 };
	string strTempRtsp;

	strcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspCmdString, szMedomHttpURL);

	nPos1 = strHttpURL.find(" ", 0);
	if (nPos1 > 0 && nPos1 != string::npos)
	{
		nPos2 = strHttpURL.find(" ", nPos1 + 1);
		if (nPos1 > 0 && nPos2 > 0 && nPos2 != string::npos)
		{
			memcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspCommand, szMedomHttpURL, nPos1);

			//memcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspURL, szMedomHttpURL + nPos1 + 1, nPos2 - nPos1 - 1);
			memcpy(szTempRtsp, szMedomHttpURL + nPos1 + 1, nPos2 - nPos1 - 1);
			strTempRtsp = szTempRtsp;
			nPos3 = strTempRtsp.find("?", 0);
			if (nPos3 > 0 && nPos3 != string::npos)
			{//去掉后面的
				szTempRtsp[nPos3] = 0x00;
			}
			strTempRtsp = szTempRtsp;

			//增加554 端口
			nPos4 = strTempRtsp.find(":", 8);
			if (nPos4 <= 0 && nPos4 != string::npos)
			{//没有554 端口
				nPos3 = strTempRtsp.find("/", 8);
				if (nPos3 > 0 && nPos3 != string::npos)
				{
					strTempRtsp.insert(nPos3, ":554");
				}
			}

			strcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspURL, strTempRtsp.c_str());
		}
	}
}

 //把http头数据填充到结构中
int  CNetClientRecvRtsp::FillHttpHeadToStruct()
{
	RtspProtectArrayOrder = 0;
	if (RtspProtectArrayOrder >= MaxRtspProtectCount)
		return true;

	int  nStart = 0;
	int  nPos = 0;
	int  nFlagLength;
	if (nHttpHeadEndLength <= 0)
		return true;
	int  nKeyCount = 0;
	char szTemp[string_length_4096] = { 0 };
	char szKey[string_length_4096] = { 0 };

	for (int i = 0; i < nHttpHeadEndLength - 2; i++)
	{
		if (memcmp(data_ + i, szHttpHeadEndFlag, 2) == 0)
		{
			memset(szTemp, 0x00, sizeof(szTemp));
			memcpy(szTemp, data_ + nPos, i - nPos);

			if ((nFlagLength = FindKeyValueFlag(szTemp)) >= 0)
			{
				memset(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey, 0x00, sizeof(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey));//要清空
				memset(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue, 0x00, sizeof(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue));//要清空 

 				memcpy(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey, szTemp, nFlagLength);
				memcpy(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue, szTemp + nFlagLength + 2, strlen(szTemp) - nFlagLength - 2);

				strcpy(szKey, RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey);
			
#ifdef USE_BOOST
				to_lower(szKey);
#else
				ABL::to_lower(szKey);
#endif
				if (strcmp(szKey, "content-length") == 0)
				{//内容长度
					nContentLength = atoi(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue);
					RtspProtectArray[RtspProtectArrayOrder].nRtspSDPLength = nContentLength;
				}

				nKeyCount++;

				//防止超出范围
				if (nKeyCount >= MaxRtspValueCount)
					return true;
			}
			else
			{//保存 http 方法、URL 
				GetHttpModemHttpURL(szTemp);
			} 

			nPos = i + 2;
		}
	}

	return true;
} 

bool CNetClientRecvRtsp::GetFieldValue(char* szFieldName, char* szFieldValue)
{
	bool bFindFlag = false;

	for (int i = 0; i < MaxRtspProtectCount; i++)
	{
		for (int j = 0; j < MaxRtspValueCount; j++)
		{
			if (strcmp(RtspProtectArray[i].rtspField[j].szKey, szFieldName) == 0)
			{
				bFindFlag = true;
				strcpy(szFieldValue, RtspProtectArray[i].rtspField[j].szValue);
				break;
			}
		}
	}

	return bFindFlag;
}

//统计rtspURL  rtsp://190.15.240.11:554/Media/Camera_00001 路径 / 的数量 
int  CNetClientRecvRtsp::GetRtspPathCount(char* szRtspURL)
{
	string strCurRtspURL = szRtspURL;
	int    nPos1, nPos2;
	int    nPathCount = 0;
 	nPos1 = strCurRtspURL.find("//", 0);
	if (nPos1 < 0)
		return 0;//地址非法 

	while (true)
	{
		nPos1 = strCurRtspURL.find("/", nPos1 + 2);
		if (nPos1 >= 0)
		{
			nPos1 += 1;
			nPathCount ++;
		}
		else
			break;
	}
	return nPathCount;
}

//处理rtsp数据
void  CNetClientRecvRtsp::InputRtspData(unsigned char* pRecvData, int nDataLength)
{
#if  0 
	 if (nDataLength < 1024)
		WriteLog(Log_Debug, "RecvData \r\n%s \r\n", pRecvData);
#endif

    if (memcmp(data_, "RTSP/1.0 401", 12) == 0 && nRtspProcessStep == RtspProcessStep_OPTIONS && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//大华摄像机,OPTIONS需要发送md5方式验证
		if (!GetWWW_Authenticate())
		{
			DeleteNetRevcBaseClient(nClient);
			return;
		}
		SendOptions(AuthenticateType);
	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_OPTIONS && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		SendDescribe(AuthenticateType);
   	}
	else if (nRtspProcessStep == RtspProcessStep_DESCRIBE && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//&& nRecvLength > 500
 		if (data_Length - nHttpHeadEndLength < nContentLength)
		{
			WriteLog(Log_Debug, "数据尚未接收完整 ");
 			return;
		}

		//打印接收到Describe 信息 
		if (nDataLength < 1024)
			WriteLog(Log_Debug, "Describe Response nClient = %llu \r\n%s \r\n",nClient, pRecvData);

		//需要MD5验证
		if (memcmp(pRecvData, "RTSP/1.0 401", 12) == 0)
		{
			if (!GetWWW_Authenticate())
			{
				DeleteNetRevcBaseClient(nClient);
			    return;
 		     }

		   SendDescribe(AuthenticateType);
		   return ;
		}

		memset(szCSeq, 0x00, sizeof(szCSeq));
		GetFieldValue("CSeq", szCSeq);

		//获取 Content-Base 字段
		GetFieldValue("Content-Base", m_szContentBaseURL);
		if (strlen(m_szContentBaseURL) == 0 || memcmp(m_szContentBaseURL, "rtsp://", 7) != 0)
			strcpy(m_szContentBaseURL, m_rtspStruct.szRtspURLTrim);
 
		currentSession = Session;
		Session++;
 
		//拷贝SDP信息
		memset(szRtspContentSDP, 0x00, sizeof(szRtspContentSDP));
		memcpy(szRtspContentSDP, data_ + nHttpHeadEndLength, nContentLength);
		nContentLength = 0; //nContentLength长度 要复位

		//从SDP中获取视频、音频格式信息 
		if (!GetMediaInfoFromRtspSDP())
		{
			sprintf(szResponseBuffer, "RTSP/1.0 415 Unsupport Media Type\r\nServer: %s\r\nCSeq: %s\r\n\r\n", MediaServerVerson, szCSeq);
			nSendRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
            
			WriteLog(Log_Debug, "RtspProcessStep_DESCRIBE 返回 \r\n%s", pRecvData);
			WriteLog(Log_Debug, "ANNOUNCE SDP 信息中没有合法的媒体数据 %s ", szRtspContentSDP);
			
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%llu}",IndexApiCode_ConnectFail,pRecvData, hParent);
		    ResponseHttp(nClient_http,szResponseBody,false );

			DeleteNetRevcBaseClient(nClient);
			return ;
		}

		//判断视频，音频是否合法
		if (!(strcmp(szVideoName, "H264") == 0 || strcmp(szVideoName, "H265") == 0 ||
			strcmp(szAudioName, "G711_A") == 0 || strcmp(szAudioName, "G711_U") == 0 ||
			strcmp(szAudioName, "AAC") == 0
			))
		{
			WriteLog(Log_Debug, "不支持的媒体格式 videoName = %s, audioName = %s ", szVideoName, szAudioName);

			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%llu}", IndexApiCode_ConnectFail, pRecvData, hParent);
			ResponseHttp(nClient_http, szResponseBody, false);

			DeleteNetRevcBaseClient(nClient);
			return;
		}

		//确定音视频轨道
		if (FindVideoAudioInSDP() == false)
		{
			WriteLog(Log_Debug, "获取音视频格式失败！，准备执行删除  nClient = %llu ", nClient);
			DeleteNetRevcBaseClient(nClient);
 		}

		//把RTSP的SDP 拷贝给媒体源 
		if(pMediaSource)
		  strcpy(pMediaSource->rtspSDPContent.szSDPContent, szRtspContentSDP);

		GetSPSPPSFromDescribeSDP();
		if (m_nSpsPPSLength > 0 && m_nSpsPPSLength <= sizeof(pMediaSource->pSPSPPSBuffer))
		{
 			memcpy(pMediaSource->pSPSPPSBuffer, m_pSpsPPSBuffer, m_nSpsPPSLength);
			pMediaSource->nSPSPPSLength = m_nSpsPPSLength;
 		}

		if (strcmp(szVideoName, "H265") == 0)
		{//获取h265的VPS、SPS、PPS 
			if (GetH265VPSSPSPPS(szRtspContentSDP, nVideoPayload))
			{
				memcpy(pMediaSource->pSPSPPSBuffer, m_pSpsPPSBuffer, m_nSpsPPSLength);
				pMediaSource->nSPSPPSLength = m_nSpsPPSLength;
			}
		}

		//确定ADTS头相关参数
		sample_index = 11;
		for (int i = 0; i < 13; i++)
		{
			if (avpriv_mpeg4audio_sample_rates[i] == nSampleRate)
			{
				sample_index = i;
				break;
			}
		} 
		SendSetup(AuthenticateType);

		//把视频，音频相关信息拷贝给媒体源
		strcpy(pMediaSource->rtspSDPContent.szVideoName, szVideoName);
		pMediaSource->rtspSDPContent.nVidePayload = nVideoPayload;
		strcpy(pMediaSource->rtspSDPContent.szAudioName, szAudioName);
		pMediaSource->rtspSDPContent.nChannels = nChannels;
		pMediaSource->rtspSDPContent.nAudioPayload = nAudioPayload;
		pMediaSource->rtspSDPContent.nSampleRate = nSampleRate;
 	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_SETUP && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		GetFieldValue("Session", szSessionID);
		if (strlen(szSessionID) == 0)
		{//有些服务器检查Session,如果不对，会立刻关闭
			string strSessionID;
			char   szTempSessionID[string_length_1024] = { 0 };
			int    nPos;
			if (GetFieldValue("session", szTempSessionID))
			{
				strSessionID = szTempSessionID;
				nPos = strSessionID.find(";", 0);
				if (nPos > 0)
				{//有超时项，需要去掉超时选项
					memcpy(szSessionID, szTempSessionID, nPos);
				}
				else
					strcpy(szSessionID, szTempSessionID);
			}
			else
				strcpy(szSessionID, "1000005");
		}

		if (nMediaCount == 1)
		{//只有1个媒体，直接发送Player
			SendPlay(AuthenticateType);
			
		    sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", hParent);
		    ResponseHttp(nClient_http,szResponseBody,false );
		}
		else if (nMediaCount == 2)
		{
			if (nSendSetupCount == 1)
			{
				SendSetup(AuthenticateType);//需要再发送
				
		        sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", hParent);
		        ResponseHttp(nClient_http,szResponseBody,false );
			}
			else
			{
				SendPlay(AuthenticateType);
			}
		}

		nRecvLength = nHttpHeadEndLength = 0;
   	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_PLAY && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "收到 Play 回复命令，rtsp交互完毕 nClient = %llu ", nClient);
		nRtspProcessStep = RtspProcessStep_PLAYSucess ;
 		auto  pParentPtr = GetNetRevcBaseClient(hParent);
		if (pParentPtr)
			bProxySuccessFlag = pParentPtr->bProxySuccessFlag = true;
	}
	else if (memcmp(pRecvData, "TEARDOWN", 8) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "收到 TEARDOWN 命令，立即执行删除 nClient = %llu ", nClient);
		bRunFlag = false;
		DeleteNetRevcBaseClient(nClient);
	}
	else if (memcmp(pRecvData, "ANNOUNCE", 8) == 0 && strstr((char*)pRecvData, "QUIT") != NULL && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//收到华为特殊命令，码流发送结束包
		WriteLog(Log_Debug, "收到华为特殊命令，码流发送结束包 nClient = %llu \r\n%s", nClient, pRecvData);

		if (m_nXHRtspURLType == XHRtspURLType_RecordPlay)//录像回放
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
	}
	else if (memcmp(pRecvData, "ANNOUNCE", 8) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//收到华为特殊命令，码流发送结束包
		 WriteLog(Log_Debug, "收到华为回放特殊命令 nClient = %llu \r\n%s", nClient, pRecvData);
  	}
	else if (memcmp(data_, "RTSP/1.0 405", 12) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "收到华为回放特殊命令 nClient = %llu \r\n%s", nClient, pRecvData);
	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 &&  strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "收到 回复命令，nClient = %llu \r\n%s", nClient, pRecvData);
	}
	else if (memcmp(pRecvData, "GET_PARAMETER", 13) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		memset(szCSeq, 0x00, sizeof(szCSeq));
		GetFieldValue("CSeq", szCSeq);

		sprintf(szResponseBuffer, "RTSP/1.0 200 OK\r\nCSeq: %s\r\nPublic: %s\r\nx-Timeshift_Range: clock=20100318T021915.84Z-20100318T031915.84Z\r\nx-Timeshift_Current: clock=20100318T031915.84Z\r\n\r\n", szCSeq, RtspServerPublic);
		nSendRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
		if (nSendRet != 0)
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
 	}
	else
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%llu}", IndexApiCode_ConnectFail, pRecvData, hParent);
		ResponseHttp(nClient_http, szResponseBody, false);

		bIsInvalidConnectFlag = true; //确认为非法连接 
		WriteLog(Log_Debug, "非法的rtsp 命令，立即执行删除 nClient = %llu, \r\n%s ",nClient, pRecvData);
		DeleteNetRevcBaseClient(nClient);
	}
}

bool CNetClientRecvRtsp::getRealmAndNonce(char* szDigestString, char* szRealm, char* szNonce)
{
	string strDigestString = szDigestString;

	int nPos1, nPos2;
	nPos1 = strDigestString.find("realm=\"", 0);
	if (nPos1 <= 0)
		return false;
	nPos2 = strDigestString.find("\"", nPos1 + strlen("realm=\""));
	if (nPos2 <= 0)
		return false;

	memcpy(szRealm, szDigestString + nPos1 + 7, nPos2 - nPos1 - 7);

	nPos1 = strDigestString.find("nonce=\"", 0);
	if (nPos1 <= 0)
		return false;
	nPos2 = strDigestString.find("\"", nPos1 + strlen("nonce=\""));
	if (nPos2 <= 0)
		return false;

	memcpy(szNonce, szDigestString + nPos1 + 7, nPos2 - nPos1 - 7);

	return true;
}


//获取认证方式
bool  CNetClientRecvRtsp::GetWWW_Authenticate()
{
	if (!GetFieldValue("WWW-Authenticate", szWww_authenticate))
	{
		WriteLog(Log_Debug, "在Describe中，获取不到 www-authenticate 信息！\r\n");
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return false;
	}

	//确定是哪种认证类型
	string strWww_authenticate = szWww_authenticate;
	int nPos1 = strWww_authenticate.find("Digest ", 0);//摘要认证
	int nPos2 = strWww_authenticate.find("Basic ", 0);//基本认证
	if (nPos1 >= 0)
		AuthenticateType = WWW_Authenticate_MD5;
	else if (nPos2 >= 0)
		AuthenticateType = WWW_Authenticate_Basic;
	else
	{//不认识的认证方式，
		WriteLog(Log_Debug, "在Describe中，找不到认识的认证方式 ！\r\n");
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return false;
	}

	if (AuthenticateType == WWW_Authenticate_MD5)
	{//摘要认证 获取realm ,nonce 
		if (getRealmAndNonce(szWww_authenticate, m_rtspStruct.szRealm, m_rtspStruct.szNonce) == false)
		{
			WriteLog(Log_Debug, "在Describe中，获取不到 realm,nonce 信息！\r\n");
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return false;
		}
	}
	return true;
}

//从sdp信息中获取视频、音频格式信息 ,根据这些信息创建rtp解包、rtp打包 
bool   CNetClientRecvRtsp::GetMediaInfoFromRtspSDP()
{
	//把sdp装入分析器
	string strSDPTemp = szRtspContentSDP;
	char   szTemp[string_length_1024] = { 0 };
	int    nPos1 = strSDPTemp.find("m=video", 0);
	int    nPos2 = strSDPTemp.find("m=audio", 0);
	int    nPos3;
	memset(szVideoSDP, 0x00, sizeof(szVideoSDP));
	memset(szAudioSDP, 0x00, sizeof(szAudioSDP));
	memset(szVideoName, 0x00, sizeof(szVideoName));
	memset(szAudioName, 0x00, sizeof(szAudioName));
	nChannels = 0;

	if (nPos1 >= 0 && nPos2 > 0 && nPos2 > nPos1)
	{//视频SDP 排在前面 ，音频SDP排在后面
		memcpy(szVideoSDP, szRtspContentSDP + nPos1, nPos2 - nPos1);
		memcpy(szAudioSDP, szRtspContentSDP + nPos2, strlen(szRtspContentSDP) - nPos2);

		sipParseV.ParseSipString(szVideoSDP);
		sipParseA.ParseSipString(szAudioSDP);
	}
	else if (nPos1 >= 0 && nPos2 > 0 && nPos2 < nPos1)
	{//视频SDP排在后面，音频SDP排在前面，ZLMediaKit 采用这样的排列方式 
		memcpy(szVideoSDP, szRtspContentSDP + nPos1, strlen(szRtspContentSDP) - nPos1);
		memcpy(szAudioSDP, szRtspContentSDP + nPos2, nPos1 - nPos2);

		sipParseV.ParseSipString(szVideoSDP);
		sipParseA.ParseSipString(szAudioSDP);
	}
	else if (nPos1 >= 0 && nPos2 < 0)
	{//只有视频
		memcpy(szVideoSDP, szRtspContentSDP + nPos1, strlen(szRtspContentSDP) - nPos1);
		sipParseV.ParseSipString(szVideoSDP);
	}
	else
		return false;

	//获取视频编码名称
	string strVideoName;
	char   szTemp2[string_length_1024] = { 0 };
	char   szTemp3[string_length_1024] = { 0 };
	if (sipParseV.GetSize() > 0)
	{
		if (sipParseV.GetFieldValue("a=rtpmap", szTemp))
		{
			strVideoName = szTemp;
			nPos1 = strVideoName.find(" ", 0);
			if (nPos1 > 0)
			{
				memcpy(szTemp2, szTemp, nPos1);
				nVideoPayload = atoi(szTemp2);
			}
			nPos2 = strVideoName.find("/", 0);
			if (nPos1 > 0 && nPos2 > 0)
			{
				memcpy(szVideoName, szTemp + nPos1 + 1, nPos2 - nPos1 - 1);

				//转为大小
				strVideoName = szVideoName;

#ifdef USE_BOOST
				to_upper(strVideoName);
#else
				ABL::to_upper(strVideoName);
#endif
				strcpy(szVideoName, strVideoName.c_str());
			}

			WriteLog(Log_Debug, "nClient = %llu ,在SDP中，获取到的视频格式为 %s , payload = %d ！",nClient, szVideoName, nVideoPayload);
		}
	}

	  //获取音频信息
	  if (sipParseA.GetSize() > 0)
	  {
		memset(szTemp2, 0x00, sizeof(szTemp2));
		memset(szTemp, 0x00, sizeof(szTemp));
		if (sipParseA.GetFieldValue("a=rtpmap", szTemp))
		{
			strVideoName = szTemp;
			nPos1 = strVideoName.find(" ", 0);
			if (nPos1 > 0)
			{
				memcpy(szTemp2, szTemp, nPos1);
				nAudioPayload = atoi(szTemp2);
			}
			nPos2 = strVideoName.find("/", 0);
			if (nPos1 > 0 && nPos2 > 0)
			{
				memcpy(szAudioName, szTemp + nPos1 + 1, nPos2 - nPos1 - 1);

				//转为大小
				string strName = szAudioName;
		
#ifdef USE_BOOST
				to_upper(strName);
#else
				ABL::to_upper(strName);
#endif
				strcpy(szAudioName, strName.c_str());

				//找出采用频率、通道数
				nPos3 = strVideoName.find("/", nPos2 + 1);
				memset(szTemp2, 0x00, sizeof(szTemp2));
				if (nPos3 > 0)
				{
					memcpy(szTemp2, szTemp + nPos2 + 1, nPos3 - nPos2 - 1);
					nSampleRate = atoi(szTemp2);

					memcpy(szTemp3, szTemp + nPos3 + 1, strlen(szTemp) - nPos3 - 1);
					nChannels = atoi(szTemp3);//通道数量
				}
				else
				{
					memcpy(szTemp2, szTemp + nPos2 + 1, strlen(szTemp) - nPos2 - 1);
					nSampleRate = atoi(szTemp2);
					nChannels = 1;
				}

				//防止音频通道数出错 
				if (nChannels > 2)
					nChannels = 1;
			}
		}
		else
		{//ffmpeg g711a\g711u m=audio 0 RTP/AVP 0 
			sipParseA.GetFieldValue("m=audio", szTemp);
			if (strstr(szTemp, "RTP/AVP 0") != NULL)
			{
				nSampleRate = 8000;
				nChannels = 1;
				strcpy(szAudioName, "PCMU");
			}
			else if (strstr(szTemp, "RTP/AVP 8") != NULL)
			{
				nSampleRate = 8000;
				nChannels = 1;
				strcpy(szAudioName, "PCMA");
			}
		}

		if (strcmp(szAudioName, "PCMA") == 0)
		{
			nSampleRate = 8000;
			nChannels = 1;
			strcpy(szAudioName, "G711_A");
		}
		else if (strcmp(szAudioName, "PCMU") == 0)
		{
			nSampleRate = 8000;
			nChannels = 1;
			strcpy(szAudioName, "G711_U");
		}
		else if (strcmp(szAudioName, "MPEG4-GENERIC") == 0)
		{
			strcpy(szAudioName, "AAC");
		}
		else if (strstr(szAudioName, "G726") != NULL)
		{
			strcpy(szAudioName, "G726LE");
		}
		else
			strcpy(szAudioName, "NONE");

		WriteLog(Log_Debug, "在SDP中，获取到的音频格式为 %s ,nChannels = %d ,SampleRate = %d , payload = %d ！", szAudioName, nChannels, nSampleRate, nAudioPayload);
	}

   if (strlen(szVideoName) == 0)
   {
		nMediaCount = 0;
		return false;
    }
   else
   {
	   if (nChannels >= 1)
		   nMediaCount = 2;
	   else
		   nMediaCount = 1;
      return true;
   }
}

//查找rtp包标志 
bool CNetClientRecvRtsp::FindRtpPacketFlag()
{
	bool bFindFlag = false;

	unsigned char szRtpFlag1[2] = { 0x24, 0x00 };
	unsigned char szRtpFlag2[2] = { 0x24, 0x01 };
	unsigned char szRtpFlag3[2] = { 0x24, 0x02 };
	unsigned char szRtpFlag4[2] = { 0x24, 0x03 };
	int  nPos = 0;
	unsigned short nFindLength;

	if (netDataCacheLength > 2)
	{
		for (int i = nNetStart; i < nNetEnd; i++)
		{
			if ((memcmp(netDataCache + i, szRtpFlag1, 2) == 0) || 
				(memcmp(netDataCache + i, szRtpFlag2, 2) == 0) || 
				(memcmp(netDataCache + i, szRtpFlag3, 2) == 0) || 
				(memcmp(netDataCache + i, szRtpFlag4, 2) == 0))
			{
				memcpy((char*)&nFindLength, netDataCache + i + 2, sizeof(nFindLength));
				nFindLength = ntohs(nFindLength);
				if (nFindLength > 0 && nFindLength <= 1500)
				{//需要判断rtp包数据长度 
				  nPos = i;
				  bFindFlag = true;
 				  break;
 				}
			}
  		}
	}

	//找到标志，重新计算起点，及长度 
	if (bFindFlag)
	{
		nNetStart = nPos;
		netDataCacheLength = nNetEnd - nNetStart;
		WriteLog(Log_Debug, "CNetClientRecvRtsp = %X ,找到RTP位置， nNetStart = %d , nNetEnd = %d , netDataCacheLength = %d ", this, nNetStart, nNetEnd, netDataCacheLength);
	}

	return bFindFlag;
}

//处理网络数据
int CNetClientRecvRtsp::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(netDataLock);

	bExitProcessFlagArray[2] = false; 
	tRtspProcessStartTime = GetTickCount64();
	int nRet;
	uint32_t nReadLength = 4;

	while (!bIsInvalidConnectFlag && bRunFlag && netDataCacheLength > 4)
	{
		data_Length = 0;
		memcpy((unsigned char*)&rtspHead, netDataCache + nNetStart, sizeof(rtspHead));
		if (true)
		{
			if (rtspHead.head == '$')
			{//rtp 数据
				nNetStart += 4;
				netDataCacheLength -= 4;

				nRtpLength = nReadLength = ntohs(rtspHead.Length);

				if (nRtpLength > netDataCacheLength)
				{//剩余rtp长度不够读取，需要退出，等待下一次读取
					bExitProcessFlagArray[2] = true;
					nNetStart -= 4;
					netDataCacheLength += 4;
 					return 0;
				}

				if (nRtpLength > 0 && nRtpLength <= 65535 )
				{
					if (ABL_MediaServerPort.nSaveProxyRtspRtp == 1 && fWriteRtpVideo != NULL && GetTickCount64() - nCreateDateTime <= 1000 * 180)
					{//只保存5分钟
					   fwrite(netDataCache + (nNetStart - 4), 1, 4, fWriteRtpVideo);
					   fwrite(netDataCache + nNetStart, 1, nRtpLength, fWriteRtpVideo);
					   fflush(fWriteRtpVideo);
 					}
 
					if (rtspHead.chan == 0x00)
					{
						//程序字段判断rtp负载类型 
						if (nRtspRtpPayloadType == RtspRtpPayloadType_Unknow)
						{
							//PS 头
							if(memcmp(netDataCache+ (nNetStart + 12), psHeadFlag,4) == 0)
 							    nRtspRtpPayloadType = RtspRtpPayloadType_PS;
							else
								nRtspRtpPayloadType = RtspRtpPayloadType_ES;

							StartRtpPsDemux();//根据rtp负载类型，启动rtp、PS解包 
						}
						if (nVideoSSRC == 0)
						{//获取SSRC 
							memcpy((char*)&rtpHead, netDataCache + nNetStart, sizeof(rtpHead));
							nVideoSSRC = rtpHead.ssrc;
						}

						//统计rtp最大包长
						if (nRecvRtpPacketCount < 1000)
						{
							nRecvRtpPacketCount ++;
							if (nRtpLength > nMaxRtpLength)
							{//记录rtp 包最大长度 
								nMaxRtpLength = nRtpLength;
								WriteLog(Log_Debug, "CNetClientRecvRtsp = %X  nClient = %llu , 获取到rtp最大长度 , nMaxRtpLength = %d ", this, nClient, nMaxRtpLength);
							}
						}
						//rtp 包长度正常才进行解包
						if (nRtpLength <= nMaxRtpLength && rtpDecoder[0] != NULL)
							rtp_payload_decode_input(rtpDecoder[0], netDataCache + nNetStart, nRtpLength);
						else
						{//rtp 包长度异常
							WriteLog(Log_Debug, "CNetClientRecvRtsp = %X rtp包头长度有误  nClient = %llu ,nRtpLength = %llu , nMaxRtpLength = %d ", this, nClient, nRtpLength, nMaxRtpLength);
							DeleteNetRevcBaseClient(nClient);
							return -1;
						}
 
						 if (!bUpdateVideoFrameSpeedFlag)
						 {//更新视频源的帧速度
							 int nVideoSpeed = CalcVideoFrameSpeed(netDataCache + nNetStart, nRtpLength);
							 if (nVideoSpeed > 0 && pMediaSource != NULL )
							 {
								 bUpdateVideoFrameSpeedFlag = true;
								 WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ",nClient, pMediaSource->m_szURL, pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
								 pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, netBaseNetType) ;
							 }
						 }
						//if(nPrintCount % 200 == 0)
 						//  printf("this =%X, Video Length = %d \r\n",this, nReadLength);
					}
					else if (rtspHead.chan == 0x02)
					{
						//只要有音频包，一定是负载 ES 流
						if (nRtspRtpPayloadType == RtspRtpPayloadType_Unknow)
						{
							nRtspRtpPayloadType = RtspRtpPayloadType_ES;
							StartRtpPsDemux();//根据rtp负载类型，启动rtp、PS解包 
 						}
						if (audioSSRC == 0)
						{//获取SSRC 
							memcpy((char*)&rtpHead, netDataCache + nNetStart, sizeof(rtpHead));
							audioSSRC = rtpHead.ssrc;
						}

						//统计rtp最大包长
						if (nRecvRtpPacketCount < 1000)
						{
							nRecvRtpPacketCount++;
							if (nRtpLength > nMaxRtpLength)
							{//记录rtp 包最大长度 
								nMaxRtpLength = nRtpLength;
								WriteLog(Log_Debug, "CNetClientRecvRtsp = %X  nClient = %llu , 获取到rtp最大长度 , nMaxRtpLength = %d ", this, nClient, nMaxRtpLength);
							}
						}

						if (strcmp(szSdpAudioName, "NONE") != 0)
						{//不支持的音频格式 
							//rtp 包长度正常才进行解包
							if (nRtpLength <= nMaxRtpLength && rtpDecoder[1] != NULL)
								rtp_payload_decode_input(rtpDecoder[1], netDataCache + nNetStart, nRtpLength);
							else
							{//rtp 包长度异常
								WriteLog(Log_Debug, "CNetClientRecvRtsp = %X rtp包头长度有误  nClient = %llu ,nRtpLength = %llu , nMaxRtpLength = %d ", this, nClient, nRtpLength, nMaxRtpLength);
								DeleteNetRevcBaseClient(nClient);
								return -1;
							}
						}
  
						//if(nPrintCount % 100 == 0 )
						//	WriteLog(Log_Debug, "this =%X ,Audio Length = %d ",this,nReadLength);

						nPrintCount ++;
					}
					else if (rtspHead.chan == 0x01)
					{//收到RTCP包，需要回复rtcp报告包
						//WriteLog(Log_Debug, "this =%X ,收到 视频 的RTCP包，需要回复rtcp报告包，netBaseNetType = %d  收到RCP包长度 = %d ", this, netBaseNetType, nReadLength);
						SendRtcpReportDataRR(nVideoSSRC, 1);
					}
					else if (rtspHead.chan == 0x03)
					{//收到RTCP包，需要回复rtcp报告包
						//WriteLog(Log_Debug, "this =%X ,收到 音频 的RTCP包，需要回复rtcp报告包，netBaseNetType = %d  收到RCP包长度 = %d ", this, netBaseNetType, nReadLength);
						SendRtcpReportDataRR(audioSSRC, 3);
					}

					bExitProcessFlagArray[2] = true;
					nNetStart          += nRtpLength;
					netDataCacheLength -= nRtpLength;
 				}
				else
				{//只有rtsp头，没有rtp包
					bExitProcessFlagArray[2] = true;
					return -1;
				}
			}
			else
			{//rtsp 数据
				if (FindRtpPacketFlag() == true)
				{//rtp 丢包 ,乱序
					bExitProcessFlagArray[2] = true;
					return 0;
				}

				if (!ReadRtspEnd())
				{
					WriteLog(Log_Debug, "ReadDataFunc() ,尚未读取到rtsp (1)数据 ! nClient = %llu ", nClient);
					bExitProcessFlagArray[2] = true;
					DeleteNetRevcBaseClient(nClient);
					return  -1;
				}
				else
				{
					if (rtpDecoder[0] == NULL || memcmp(data_, "ANNOUNCE", 8) == 0 || memcmp(data_, "RTSP/1.0 200", 12) == 0)
					{//rtsp尚未交互完毕、或者 华为发送结束通知 
					  //填充rtsp头
					  if (FindHttpHeadEndFlag() > 0)
						 FillHttpHeadToStruct();

						if (nContentLength > 0)
						{
							nReadLength = nContentLength;

							//如果有ContentLength ，需要积累了ContentLength的内容再进行读取 
							if (netDataCacheLength < nContentLength)
							{//剩下的数据不够 ContentLength ,需要重新移动已经读取的字节数 data_Length  
								nNetStart -= data_Length;
								netDataCacheLength += data_Length;
								bExitProcessFlagArray[2] = true;
								WriteLog(Log_Debug, "ReadDataFunc (), RTSP 的 Content-Length 的数据尚未接收完整  nClient = %llu", nClient);
								return 0;
							}
 
							nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
							if (nRet != 0 || nReadLength != nContentLength)
							{
								WriteLog(Log_Debug, "ReadDataFunc() ,尚未读取到rtsp (2)数据 ! nClient = %llu", nClient);
								bExitProcessFlagArray[2] = true;
								DeleteNetRevcBaseClient(nClient);
								return -1;
							}
							else
							{
								data_Length += nContentLength;
							}
						}

					   data_[data_Length] = 0x00;
					   InputRtspData(data_, data_Length);
				    }
				    break; //rtsp 都是一 一交互的，执行完毕立即退出 
				}
			}
		}
		else
		{
			WriteLog(Log_Debug, "CNetClientRecvRtsp= %X  , ProcessNetData() ,尚未读取到数据 ! , nClient = %llu", this, nClient);
			bExitProcessFlagArray[2] = true;
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		if (GetTickCount64() - tRtspProcessStartTime > 16000)
		{
			WriteLog(Log_Debug, "CNetClientRecvRtsp= %X  , ProcessNetData() ,RTSP 网络处理超时 ! , nClient = %llu", this, nClient);
			bExitProcessFlagArray[2] = true;
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
	}

	bExitProcessFlagArray[2] = true;
	return 0;
}

//发送rtcp 报告包
void  CNetClientRecvRtsp::SendRtcpReportData()
{
	memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
	rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
	rtcpSR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nVideoSSRC);

	ProcessRtcpData((char*)szRtcpSRBuffer, rtcpSRBufferLength, 1);
}

//发送rtcp 报告包 接收端
void  CNetClientRecvRtsp::SendRtcpReportDataRR(unsigned int nSSRC, int nChan)
{
	memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
	rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
	rtcpRR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

	ProcessRtcpData((char*)szRtcpSRBuffer, rtcpSRBufferLength, nChan);
}

void  CNetClientRecvRtsp::ProcessRtcpData(char* szRtcpData, int nDataLength, int nChan)
{
	std::lock_guard<std::mutex> lock(MediaSumRtpMutex);

	szRtcpDataOverTCP[0] = '$';
	szRtcpDataOverTCP[1] = nChan;
	unsigned short nRtpLen = htons(nDataLength);
	memcpy(szRtcpDataOverTCP + 2, (unsigned char*)&nRtpLen, sizeof(nRtpLen));

	memcpy(szRtcpDataOverTCP + 4, szRtcpData, nDataLength);
	XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 4, 1);
}

//发送第一个请求
int CNetClientRecvRtsp::SendFirstRequst()
{
	if (strlen(m_szShareMediaURL) > 0)
	{
		pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hParent, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
		if (pMediaSource)
		{
			pMediaSource->netBaseNetType = netBaseNetType;
			pMediaSource->enable_mp4 = (strcmp(m_addStreamProxyStruct.enable_mp4, "1") == 0) ? true : false;
			pMediaSource->enable_hls = (strcmp(m_addStreamProxyStruct.enable_hls, "1") == 0) ? true : false;
		}
		else 
		{
			bRunFlag = false;
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
 	}
	SendOptions(AuthenticateType);
	m_nXHRtspURLType = atoi(m_addStreamProxyStruct.isRtspRecordURL);
 	return 0;
}

//请求m3u8文件
bool  CNetClientRecvRtsp::RequestM3u8File()
{
	return true;
}

//从 SDP中获取  SPS，PPS 信息
bool  CNetClientRecvRtsp::GetSPSPPSFromDescribeSDP()
{
	m_bHaveSPSPPSFlag = false;
	int  nPos1, nPos2;
	char  szSprop_Parameter_Sets[string_length_2048] = { 0 };

	m_nSpsPPSLength = 0;
	string strSDPTemp = szRtspContentSDP;
	nPos1 = strSDPTemp.find("sprop-parameter-sets=", 0);
	memset(m_szSPSPPSBuffer, 0x00, sizeof(m_szSPSPPSBuffer));
	nPos2 = strSDPTemp.find("\r\n", nPos1 + 1);

	if (nPos1 > 0 && nPos2 > 0)
	{
		memcpy(szSprop_Parameter_Sets, szRtspContentSDP + nPos1, nPos2 - nPos1 + 2);
		strSDPTemp = szSprop_Parameter_Sets;
		nPos1 = strSDPTemp.find("sprop-parameter-sets=", 0);

		nPos2 = strSDPTemp.find(";", nPos1 + 1);
		if (nPos2 > nPos1)
		{//后面还有别的项
			memcpy(m_szSPSPPSBuffer, szSprop_Parameter_Sets + nPos1 + strlen("sprop-parameter-sets="), nPos2 - nPos1 - strlen("sprop-parameter-sets="));
		}
		else
		{//后面没有别的项
			nPos2 = strSDPTemp.find("\r\n", nPos1 + 1);
			if (nPos2 > nPos1)
				memcpy(m_szSPSPPSBuffer, szSprop_Parameter_Sets + nPos1 + strlen("sprop-parameter-sets="), nPos2 - nPos1 - strlen("sprop-parameter-sets="));
		}
	}

	if (strlen(m_szSPSPPSBuffer) > 0)
	{
		m_nSpsPPSLength = sdp_h264_load((unsigned char*)m_pSpsPPSBuffer, sizeof(m_pSpsPPSBuffer), m_szSPSPPSBuffer);
		m_bHaveSPSPPSFlag = true;
	}

	return m_bHaveSPSPPSFlag;
}

void  CNetClientRecvRtsp::UserPasswordBase64(char* szUserPwdBase64)
{
	char szTemp[string_length_1024] = { 0 };
	sprintf(szTemp, "%s:%s", m_rtspStruct.szUser, m_rtspStruct.szPwd);
	Base64Encode((unsigned char*)szUserPwdBase64, (unsigned char*)szTemp, strlen(szTemp));
}

//确定SDP里面视频，音频的总媒体数量, 从Describe中找到 trackID，大华的从0开始，海康，华为的从1开始
bool  CNetClientRecvRtsp::FindVideoAudioInSDP()
{
	char szTemp[string_length_2048] = { 0 };
	WriteLog(Log_Debug, "开始获取 TrackID  nClient = %llu \r%s", nClient, szRtspContentSDP);

	nMediaCount = 0;
	if (strlen(szRtspContentSDP) <= 0)
		return false ;

	strcpy(szTemp, szRtspContentSDP);
#ifdef USE_BOOST
	to_lower(szTemp);
#else
	ABL::to_lower(szTemp);
#endif
	string strSDP = szTemp;
	string strTraceID;
	char   szTempTraceID[string_length_2048] = { 0 };
	int nPos, nPos2, nPos3, nPos4;

	nPos = strSDP.find("m=video");
	if (nPos >= 0)
	{
		nPos2 = strSDP.find("a=control:", nPos);
		if (nPos2 > 0)
		{
			nPos3 = strSDP.find("\r\n", nPos2);
			if (nPos3 > 0)
			{
				nMediaCount++;
				if (nPos2 + 10 > 0 && nPos2 + 10 < strlen(szRtspContentSDP) && (nPos3 - nPos2 - 10) > 0 && (nPos3 - nPos2 - 10) < strlen(szRtspContentSDP))
					memcpy(szTempTraceID, szRtspContentSDP + nPos2 + 10, nPos3 - nPos2 - 10);
				else
					return false;

				strTraceID = szTempTraceID;
				nPos4 = strTraceID.rfind("/", strlen(szTempTraceID));
				if (nPos4 <= 0)
				{//没有rtsp类似的地址，比如 trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//有rtsp类似的地址，比如海康的摄像头 a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
					if (nPos4 + 1 > 0 && nPos4 + 1 < strlen(szTempTraceID) && (strlen(szTempTraceID) - nPos4 - 1) > 0 && (strlen(szTempTraceID) - nPos4 - 1) < strlen(szTempTraceID))
						memcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID + nPos4 + 1, strlen(szTempTraceID) - nPos4 - 1);
					else
						return false;
				}
				nTrackIDOrer++;
			}
		}
	}

	nPos = strSDP.find("m=audio");
	if (nPos >= 0)
	{
		nPos2 = strSDP.find("a=control:", nPos);
		if (nPos2 > 0)
		{
			nPos3 = strSDP.find("\r\n", nPos2);
			if (nPos3 > 0)
			{
				nMediaCount++;
				if (nPos2 + 10 > 0 && nPos2 + 10 < strlen(szRtspContentSDP) && (nPos3 - nPos2 - 10) > 0 && (nPos3 - nPos2 - 10) < strlen(szRtspContentSDP))
					memcpy(szTempTraceID, szRtspContentSDP + nPos2 + 10, nPos3 - nPos2 - 10);
				else
					return false;
				strTraceID = szTempTraceID;
				nPos4 = strTraceID.rfind("/", strlen(szTempTraceID));
				if (nPos4 <= 0)
				{//没有rtsp类似的地址，比如 trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//有rtsp类似的地址，比如海康的摄像头 a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
					if (nPos4 + 1 > 0 && nPos4 + 1 < strlen(szTempTraceID) && (strlen(szTempTraceID) - nPos4 - 1) > 0 && (strlen(szTempTraceID) - nPos4 - 1) < strlen(szTempTraceID))
						memcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID + nPos4 + 1, strlen(szTempTraceID) - nPos4 - 1);
					else
						return false;
				}
				nTrackIDOrer ++;
			}
		}
	}

	return true;
}

void  CNetClientRecvRtsp::SendOptions(WWW_AuthenticateType wwwType)
{
	//确定类型
 	nSendSetupCount = 0;
	nMediaCount = 0;
	nTrackIDOrer = 1;//从1开始，不从0开始
	CSeq = 1;

	if (wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("OPTIONS", m_rtspStruct.szSrcRtspPullUrl); //要注意 uri ,有时候没有最后的 斜杠 /

		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nAuthorization: Basic %s\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szBasic);
	}

	unsigned int nRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	if (nRet != 0)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
	nRtspProcessStep = RtspProcessStep_OPTIONS;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

void  CNetClientRecvRtsp::SendDescribe(WWW_AuthenticateType wwwType)
{
	if (wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("DESCRIBE", m_rtspStruct.szSrcRtspPullUrl); //要注意 uri ,有时候没有最后的 斜杠 /

		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);

	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Basic %s\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szBasic);
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	nRtspProcessStep = RtspProcessStep_DESCRIBE;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

/*
要优化，有些摄像机 {trackID=1 、 trackID=2} ，有些摄像机是，比如大华 {trackID=0、trackID=1}
*/
void  CNetClientRecvRtsp::SendSetup(WWW_AuthenticateType wwwType)
{
	nSendSetupCount++;
	if (nSendSetupCount == 2)
	{
		string strSession = szSessionID;
		int    nPos2 = strSession.find(";", 0);
		if (nPos2 > 0)
		{
			szSessionID[nPos2] = 0x00;
			WriteLog(Log_Debug, "SendPlay() ，nClient = %llu ,strSessionID = %s , szSessionID = %s ", nClient, strSession.c_str(), szSessionID);
		}
	}
	if (wwwType == WWW_Authenticate_None)
	{
		if (nSendSetupCount == 1)
		{
			if (m_szContentBaseURL[strlen(m_szContentBaseURL) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_szContentBaseURL[strlen(m_szContentBaseURL) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
		}
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("SETUP", m_szContentBaseURL); //要注意 uri ,有时候没有最后的 斜杠 /

		if (nSendSetupCount == 1)
		{
			if (m_szContentBaseURL[strlen(m_szContentBaseURL) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_szContentBaseURL[strlen(m_szContentBaseURL) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse, szSessionID);
		}

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);

		if (nSendSetupCount == 1)
		{
			if (m_szContentBaseURL[strlen(m_szContentBaseURL) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_szContentBaseURL[strlen(m_szContentBaseURL) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic, szSessionID);
		}
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);

	nRtspProcessStep = RtspProcessStep_SETUP;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

void  CNetClientRecvRtsp::SendPlay(WWW_AuthenticateType wwwType)
{//\r\nScale: 255

    //把 session 里面; 后面字符串去掉  
	string strSession = szSessionID;
	int    nPos2 = strSession.find(";", 0);
	if (nPos2 > 0)
	{
		szSessionID[nPos2] = 0x00;
		WriteLog(Log_Debug, "SendPlay() ，nClient = %llu ,strSessionID = %s , szSessionID = %s ", nClient, strSession.c_str(), szSessionID);
	}

	if (wwwType == WWW_Authenticate_None)
	{
 		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nRange: npt=0.000-\r\n\r\n", m_szContentBaseURL, CSeq, szSessionID);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("PLAY", m_szContentBaseURL); //要注意 uri ,有时候没有最后的 斜杠 /

 		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nRange: npt=0.000-\r\n\r\n", m_szContentBaseURL, CSeq, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);

 		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nAuthorization: Basic %s\r\nRange: npt=0.000-\r\n\r\n", m_szContentBaseURL, CSeq, szSessionID, szBasic);
	}
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);

	nRtspProcessStep = RtspProcessStep_PLAY;
	m_wwwType = wwwType;
	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

//暂停
bool CNetClientRecvRtsp::RtspPause()
{
	if (m_bPauseFlag || nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0  || m_nXHRtspURLType != XHRtspURLType_RecordPlay )
  	return false;

	if (m_wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "PAUSE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_szContentBaseURL, CSeq, MediaServerVerson, szSessionID);
	}
	else if (m_wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("PAUSE", m_szContentBaseURL); //要注意 uri ,有时候没有最后的 斜杠 /

		sprintf(szResponseBuffer, "PAUSE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n\r\n", m_szContentBaseURL, CSeq, MediaServerVerson, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (m_wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "PAUSE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Basic %s\r\n\r\n", m_szContentBaseURL, CSeq, MediaServerVerson, szSessionID, szBasic);
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	m_bPauseFlag = true;
	WriteLog(Log_Debug, "CNetClientRecvRtsp = %X,nClient = %d ,RtspPause() \r\n%s", this, nClient, szResponseBuffer);

	//设置媒体源里面所有分发对象暂停 
	pMediaSource->SetPause(true);

	return true;
}

//继续 
bool CNetClientRecvRtsp::RtspResume()
{
	if (!m_bPauseFlag || nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0 || m_nXHRtspURLType != XHRtspURLType_RecordPlay)
	  return false;

	if (m_wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_szContentBaseURL, CSeq, MediaServerVerson, szSessionID);
	}
	else if (m_wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("PLAY", m_szContentBaseURL); //要注意 uri ,有时候没有最后的 斜杠 /

		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n\r\n", m_szContentBaseURL, CSeq, MediaServerVerson, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (m_wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Basic %s\r\n\r\n", m_szContentBaseURL, CSeq, MediaServerVerson, szSessionID, szBasic);
	}
	nRecvDataTimerBySecond = 0;

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	m_bPauseFlag = false;

	//设置媒体源里面所有分发对象继续 
	pMediaSource->SetPause(false);

	WriteLog(Log_Debug, "CNetClientRecvRtsp = %X,nClient = %d ,RtspResume() \r\n%s", this, nClient, szResponseBuffer);
	return true;
}

//倍速播放
bool  CNetClientRecvRtsp::RtspSpeed(char* nSpeed)
{
	if (nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0 || m_nXHRtspURLType != XHRtspURLType_RecordPlay)
		return false;

	if (m_wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nScale: %s\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_szContentBaseURL, nSpeed, CSeq, MediaServerVerson, szSessionID);
	}
	else if (m_wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("PLAY", m_szContentBaseURL); //要注意 uri ,有时候没有最后的 斜杠 /

		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nScale: %s\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n\r\n", m_szContentBaseURL, nSpeed, CSeq, MediaServerVerson, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (m_wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nScale: %s\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Basic %s\r\n\r\n", m_szContentBaseURL, nSpeed, CSeq, MediaServerVerson, szSessionID, szBasic);
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	m_bPauseFlag = false;
	WriteLog(Log_Debug, "CNetClientRecvRtsp = %X,nClient = %d ,RtspSpeed() \r\n%s", this, nClient, szResponseBuffer);
	return true;
}

//拖动播放
bool CNetClientRecvRtsp::RtspSeek(char* szSeekTime)
{
	if (nRtspProcessStep != RtspProcessStep_PLAYSucess || nClient <= 0 || m_nXHRtspURLType != XHRtspURLType_RecordPlay)
		return false;

	if (m_wwwType == WWW_Authenticate_None)
	{
#ifdef USE_BOOST
		if (boost::all(szSeekTime, boost::is_digit()) == true)
#else
		if (ABL::is_digits(szSeekTime) == true)
#endif
		  sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nRange: npt=%s-0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szSeekTime, CSeq, MediaServerVerson, szSessionID);
		else 
		  sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nRange: %s\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n", m_szContentBaseURL, szSeekTime, CSeq, MediaServerVerson, szSessionID);
	}
	else if (m_wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("PLAY", m_szContentBaseURL); //要注意 uri ,有时候没有最后的 斜杠 /

#ifdef USE_BOOST
		if (boost::all(szSeekTime, boost::is_digit()) == true)
#else
		if (ABL::is_digits(szSeekTime) == true)
#endif	
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nRange: npt=%s-0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n\r\n", m_szContentBaseURL, szSeekTime, CSeq, MediaServerVerson, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);
		else 
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nRange: %s\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUpgrade: StreamSystem4.1\r\n\r\n", m_szContentBaseURL, szSeekTime, CSeq, MediaServerVerson, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (m_wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
#ifdef USE_BOOST
		if (boost::all(szSeekTime, boost::is_digit()) == true)
#else
		if (ABL::is_digits(szSeekTime) == true)
#endif
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nRange: npt=%s-0\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Basic %s\r\n\r\n", m_szContentBaseURL, szSeekTime, CSeq, MediaServerVerson, szSessionID, szBasic);
		else
			sprintf(szResponseBuffer, "PLAY %s RTSP/1.0\r\nRange: %s\r\nCSeq: %d\r\nUser-Agent: %s\r\nSession: %s\r\nAuthorization: Basic %s\r\n\r\n", m_szContentBaseURL, szSeekTime, CSeq, MediaServerVerson, szSessionID, szBasic);
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	CSeq++;
	m_bPauseFlag = false;
	WriteLog(Log_Debug, "CNetClientRecvRtsp = %X, nClient = %d ,RtspSeek() \r\n%s", this, nClient, szResponseBuffer);
	return true;
}

//根据rtp负载类型启动rtp、PS解包 
bool   CNetClientRecvRtsp::StartRtpPsDemux()
{
	if (rtpDecoder[0] == NULL && rtpDecoder[1] == NULL)
	{
 		hRtpHandle[0].alloc = rtp_alloc;
		hRtpHandle[0].free = rtp_free;
		hRtpHandle[0].packet = rtp_decode_packet;
		if (nRtspRtpPayloadType == RtspRtpPayloadType_ES)
		{
 		    rtpDecoder[0] = rtp_payload_decode_create(nVideoPayload, szVideoName, &hRtpHandle[0], this);
  		}else  
			rtpDecoder[0] = rtp_payload_decode_create(nVideoPayload, "MP2P", &hRtpHandle[0], this);

		//只有rtp负载PS时，才创建PS解包 
		if(nRtspRtpPayloadType == RtspRtpPayloadType_PS)
		  int32_t ret = ps_demux_start(NetClientRtspRecv_demux_callback, (void*)this, e_ps_dux_timestamp, &psHandle);

		hRtpHandle[1].alloc = rtp_alloc2;
		hRtpHandle[1].free = rtp_free;
		hRtpHandle[1].packet = rtp_decode_packetAudio;

		if (strcmp(szAudioName, "AAC") == 0)
			strcpy(szSdpAudioName, "mpeg4-generic");
		else if (strcmp(szAudioName, "G711_A") == 0)
			strcpy(szSdpAudioName, "pcma");
		else if (strcmp(szAudioName, "G711_U") == 0)
			strcpy(szSdpAudioName, "pcmu");
		else 
			strcpy(szSdpAudioName, "NONE");

		if(strcmp(szSdpAudioName,"NONE") != 0)
		   rtpDecoder[1] = rtp_payload_decode_create(nAudioPayload, szSdpAudioName, &hRtpHandle[1], this);

		return true;
	}
	else
		return false;
}

//发送心跳
void  CNetClientRecvRtsp::SendOptionsHeartbeat()
{
	if (GetTickCount64() - nSendOptionsHeartbeatTimer < 1000 * 25)
		return;

	nSendOptionsHeartbeatTimer = GetTickCount64();

	WWW_AuthenticateType wwwType = AuthenticateType;
	if (wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nSession: %s\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_szContentBaseURL, CSeq, szSessionID);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char* szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("OPTIONS", m_szContentBaseURL); //要注意 uri ,有时候没有最后的 斜杠 /

		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_szContentBaseURL, CSeq, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_szContentBaseURL, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nSession: %s\r\nAuthorization: Basic %s\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_szContentBaseURL, CSeq, szSessionID, szBasic);
	}

	unsigned int nRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	if (nRet != 0)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
 
	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq ++;
}
