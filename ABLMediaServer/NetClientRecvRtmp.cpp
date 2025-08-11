/*
���ܣ�
    ʵ��rtmp�ͻ��˵Ľ���ģ�� �� 
	 
����    2021-07-18
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientRecvRtmp.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

#endif
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern int                                   SampleRateArray[] ;
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO
extern CMediaFifo                            pDisconnectMediaSource;      //�������ý��Դ 

extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result, uint16_t nLocalPort);

extern void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);

static int rtmp_client_send(void* param, const void* header, size_t len, const void* data, size_t bytes)
{
	CNetClientRecvRtmp* pClient = (CNetClientRecvRtmp*)param;

	if (pClient != NULL && pClient->bRunFlag.load())
	{
		if (len > 0 && header != NULL)
		{
			pClient->nWriteRet = XHNetSDK_Write(pClient->nClient, (uint8_t*)header, len, ABL_MediaServerPort.nSyncWritePacket);
			if (pClient->nWriteRet != 0)
			{
				pClient->nWriteErrorCount++;
				WriteLog(Log_Debug, "rtmp_client_send ����ʧ�ܣ����� nWriteErrorCount = %d ", pClient->nWriteErrorCount);
			}
			else
				pClient->nWriteErrorCount = 0;
		}
		if (bytes > 0 && data != NULL)
		{
			pClient->nWriteRet = XHNetSDK_Write(pClient->nClient, (uint8_t*)data, bytes, ABL_MediaServerPort.nSyncWritePacket);
			if (pClient->nWriteRet != 0)
			{
				pClient->nWriteErrorCount ++;
				WriteLog(Log_Debug,"rtmp_client_send ����ʧ�ܣ����� nWriteErrorCount = %d ", pClient->nWriteErrorCount);
			}
			else
				pClient->nWriteErrorCount = 0;
		}
	}
	return len + bytes;
}

static int rtmp_client_onaudio(void* param, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetClientRecvRtmp* pNetClientRtmp = (CNetClientRecvRtmp*)param ;
	if (pNetClientRtmp == NULL || !pNetClientRtmp->bRunFlag.load())
		return 0;

	flv_demuxer_input(pNetClientRtmp->flvDemuxer, FLV_TYPE_AUDIO, data, bytes, timestamp);
	return 0;
}

static int rtmp_client_onvideo(void* param, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetClientRecvRtmp* pNetClientRtmp = (CNetClientRecvRtmp*)param;
	if (pNetClientRtmp == NULL || !pNetClientRtmp->bRunFlag)
		return 0;
	flv_demuxer_input(pNetClientRtmp->flvDemuxer, FLV_TYPE_VIDEO, data, bytes, timestamp);
	return 0; 
}

static int rtmp_client_onscript(void* param, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetClientRecvRtmp* pNetClientRtmp = (CNetClientRecvRtmp*)param;
	if (pNetClientRtmp == NULL || !pNetClientRtmp->bRunFlag.load())
		return 0;
	return  flv_demuxer_input(pNetClientRtmp->flvDemuxer, FLV_TYPE_SCRIPT, data, bytes, timestamp);
}

static int NetRtmpClientRecvCallBackFLV(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
	CNetClientRecvRtmp* pClient = (CNetClientRecvRtmp*)param;

	static char s_pts[64], s_dts[64];
	static uint32_t v_pts = 0, v_dts = 0;
	static uint32_t a_pts = 0, a_dts = 0;

	//printf("[%c] pts: %s, dts: %s, %u, cts: %d, ", flv_type(codec), ftimestamp(pts, s_pts), ftimestamp(dts, s_dts), dts, (int)(pts - dts));
	if (pClient == NULL || pClient->pMediaSource == NULL || !pClient->bRunFlag.load())
		return 0;

	if (FLV_AUDIO_AAC == codec && pClient->m_addStreamProxyStruct.disableAudio[0] == 0x30)
	{
		a_pts = pts;
		a_dts = dts;

		if (strlen(pClient->pMediaSource->m_mediaCodecInfo.szAudioName) == 0 && bytes > 4 && data != NULL)
		{
			unsigned char* pAudioData = (unsigned char*)data;
			strcpy(pClient->pMediaSource->m_mediaCodecInfo.szAudioName, "AAC");

			//����Ƶ�����ֻռ4λ��  8 7 6 5 4 3 2 1  �� 6 ~ 3 λ����4��λ������Ҫ��0x3c �����㣬�ѱ��λȫ����Ϊ0 ���������ƶ�2λ��
			unsigned char nSampleIndex = ((pAudioData[2] & 0x3c) >> 2) & 0x0F;  //�� pb[2] �л�ȡ����Ƶ�ʵ����
			if (nSampleIndex <= 12)
				pClient->pMediaSource->m_mediaCodecInfo.nSampleRate = SampleRateArray[nSampleIndex];

			//ͨ���������� pAVData[2]  ����2��λ�������2λ���� 0x03 �����㣬�õ���λ�����ƶ�2λ ���� �� �� pAVData[3] ��������2λ
			//pAVData[3] ������2λ��ȡ���� �� �� 0xc0 �����㣬������6λ��ΪʲôҪ����6λ����Ϊ��2λ�������λ������Ҫ���ұ��ƶ�6λ
			pClient->pMediaSource->m_mediaCodecInfo.nChannels = ((pAudioData[2] & 0x03) << 2) | ((pAudioData[3] & 0xc0) >> 6);
		}

		if (pClient->pMediaSource)
			pClient->pMediaSource->PushAudio((unsigned char*)data, bytes, pClient->pMediaSource->m_mediaCodecInfo.szAudioName, pClient->pMediaSource->m_mediaCodecInfo.nChannels, pClient->pMediaSource->m_mediaCodecInfo.nSampleRate);

		//assert(bytes == get_adts_length((const uint8_t*)data, bytes));
#ifdef  WriteFlvToEsFileFlag
		if (pClient != NULL)
		{
			fwrite(data, 1, bytes, pClient->fWriteAudio);
			fflush(pClient->fWriteAudio);
		}
#endif
		
	}
	else if (FLV_VIDEO_H264 == codec || FLV_VIDEO_H265 == codec)
	{
		//printf("diff: %03d/%03d %s", (int)(pts - v_pts), (int)(dts - v_dts), flags ? "[I]" : "");
		v_pts = pts;
		v_dts = dts;

		if (!pClient->bUpdateVideoFrameSpeedFlag)
		{//������ƵԴ��֡�ٶ�
			int nVideoSpeed = pClient->CalcFlvVideoFrameSpeed(pts,1000);
			if (nVideoSpeed > 0 && pClient->pMediaSource != NULL)
			{
				pClient->bUpdateVideoFrameSpeedFlag = true;
				WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pClient->nClient, pClient->pMediaSource->m_szURL, pClient->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pClient->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pClient->netBaseNetType);

				sprintf(pClient->szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", pClient->hParent);
				pClient->ResponseHttp(pClient->nClient_http, pClient->szResponseBody, false);

				auto  pParentPtr = GetNetRevcBaseClient(pClient->hParent);
				if (pParentPtr && pParentPtr->bProxySuccessFlag == false)
					pClient->bProxySuccessFlag = pParentPtr->bProxySuccessFlag = true;
			}
		}

		if (pClient->pMediaSource && pClient->m_addStreamProxyStruct.disableVideo[0] == 0x30 )
		{//֧�ֹ��˵���Ƶ֡
			if (FLV_VIDEO_H264 == codec)
				pClient->pMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
			else
				pClient->pMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
		}

		//unsigned char* pVideoData = (unsigned char*)data;
		//WriteLog(Log_Debug, "CNetRtspServer=%X ,nClient = %llu, rtmp ����ص� %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X , timeStamp = %d ,datasize = %d ", pClient, pClient->nClient, (unsigned char*)pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4], pVideoData[5], pVideoData[6], pVideoData[7], pVideoData[8], pVideoData[9], pVideoData[10], pVideoData[11], pVideoData[12],dts,bytes);

#ifdef  WriteFlvToEsFileFlag
		if (pClient != NULL)
		{
			fwrite(data, 1, bytes, pClient->fWriteVideo);
			fflush(pClient->fWriteVideo);
		}
#endif
		 
	}
	else if (FLV_AUDIO_MP3 == codec)
	{
	}
	else if (FLV_AUDIO_ASC == codec || FLV_VIDEO_AVCC == codec || FLV_VIDEO_HVCC == codec)
	{
		// nothing to do
	}
	else if ((3 << 4) == codec)
	{
		//fwrite(data, bytes, 1, aac);
	}
	else
	{
		// nothing to do
		//assert(0);
	}
	return 0;
}

CNetClientRecvRtmp::CNetClientRecvRtmp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	rtmp = NULL ;
	bCheckRtspVersionFlag = false;
	bDeleteRtmpPushH265Flag = false;
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	NetDataFifo.InitFifo(MaxNetDataCacheBufferLength);
	strcpy(m_szShareMediaURL, szShareMediaURL);

	int r;

	pMediaSource = NULL;
	flvDemuxer = NULL;
	nWriteRet = 0;
	nWriteErrorCount = 0;

#ifdef  WriteFlvFileByDebug
	char  szFlvFile[256] = { 0 };
	sprintf(szFlvFile,".\\%X_%d.flv", this, rand());
	s_flv = flv_writer_create(szFlvFile);
#endif
#ifdef  WriteFlvToEsFileFlag
	char    szVideoFile[256] = { 0 };
	char    szAudioFile[256] = { 0 };
	sprintf(szVideoFile, ".\\%X_%d.264", this, rand());
	sprintf(szAudioFile, ".\\%X_%d.aac", this, rand());
	fWriteVideo = fopen(szVideoFile, "wb");
	fWriteAudio = fopen(szAudioFile, "wb");
#endif
	memset(&handler, 0, sizeof(handler));
	handler.send = rtmp_client_send;
	handler.onaudio = rtmp_client_onaudio;
	handler.onvideo = rtmp_client_onvideo;
	handler.onscript = rtmp_client_onscript;

 	if (ParseRtspRtmpHttpURL(szIP) == true)
 	  uint32_t  ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1, memcmp(m_rtspStruct.szSrcRtspPullUrl, "rtmps://", 8) == 0 ? true : false );
 
	nVideoDTS = 0 ;
	nAudioDTS = 0 ;
	memset(szRtmpName, 0x00, sizeof(szRtmpName));
	netBaseNetType = NetBaseNetType_RtmpClientRecv;

	WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu ",this, nClient);
}

CNetClientRecvRtmp::~CNetClientRecvRtmp()
{
    WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 1 ",this, nClient);
	bRunFlag.exchange(false);
	std::lock_guard<std::mutex> lock(businessProcMutex);

    WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 2 ",this, nClient);
	 
	//�������쳣�Ͽ�
	if (bUpdateVideoFrameSpeedFlag == false)
	{
	  sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"faied. Abnormal didconnection \",\"key\":%llu}", IndexApiCode_RecvRtmpFailed, 0);
	  ResponseHttp2(nClient_http,szResponseBody, false);
 	}
     WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 3 ",this, nClient);

    WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 4 ",this, nClient);

	int rtmpState = 0;
	if (rtmp != NULL)
	{
		rtmpState = rtmp_client_getstate(rtmp);
		if(rtmpState >= 3)
		  rtmp_client_stop(rtmp);

		rtmp_client_destroy(rtmp);
	}
    WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 5 ",this, nClient);

	if(flvDemuxer)
	  flv_demuxer_destroy(flvDemuxer);

    WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 6 ",this, nClient);

#ifdef  WriteFlvFileByDebug
	flv_writer_destroy(s_flv);
#endif
#ifdef  WriteFlvToEsFileFlag
 	fclose(fWriteVideo);
	fclose(fWriteAudio);
#endif

	NetDataFifo.FreeFifo();
  	
	//����û�дﵽ֪ͨ
	if (ABL_MediaServerPort.hook_enable == 1  && bUpdateVideoFrameSpeedFlag == false)
	{
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_not_arrive;
		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_not_arrive\",\"app\":\"%s\",\"stream\":\"%s\",\"sourceURL\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu}", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream,m_addStreamProxyStruct.url, ABL_MediaServerPort.mediaServerID, netBaseNetType, hParent);
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}
    WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 7 ",this, nClient);

	malloc_trim(0);
	
    WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu step 8 ",this, nClient);

	//����ǽ������������ҳɹ����������ģ�����Ҫɾ��ý������Դ szURL ������ /Media/Camera_00001 
	if(strlen(m_szShareMediaURL) > 0 && pMediaSource != NULL)
		pDisconnectMediaSource.push((unsigned char*)m_szShareMediaURL, strlen(m_szShareMediaURL));
   
	WriteLog(Log_Debug, "CNetClientRecvRtmp ���� = %X  nClient = %llu app = %s ,stream = %s ,bUpdateVideoFrameSpeedFlag = %d", this, nClient, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, bUpdateVideoFrameSpeedFlag);
}

int CNetClientRecvRtmp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetClientRecvRtmp::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}
int CNetClientRecvRtmp::SendVideo()
{
	return 0;
}

int CNetClientRecvRtmp::SendAudio()
{

	return 0;
}

int CNetClientRecvRtmp::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag.load())
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	nRecvDataTimerBySecond = 0;
	NetDataFifo.push(pData, nDataLength);
	return 0;
}

int CNetClientRecvRtmp::ProcessNetData()
{
 	unsigned char* pData = NULL;
	int            nLength;

	pData = NetDataFifo.pop(&nLength);
	if(pData != NULL && rtmp != NULL )
	{
		if (nLength > 0)
			rtmp_client_input(rtmp, pData, nLength);
 
 		NetDataFifo.pop_front();
	}
	return 0;
}

//��url�����ȡapp \ stream 
bool   CNetClientRecvRtmp::GetAppStreamByURL(char* app, char* stream)
{
	int nPos1, nPos2;
	if (!(memcmp(szClientIP, "rtmp://", 7) == 0 || memcmp(szClientIP, "rtmps://", 8) == 0))
		return false;
	string strURL = szClientIP;
	nPos1 = strURL.find("/", 10);
	if (nPos1 > 0 && nPos1 != string::npos)
	{
		nPos2 = strURL.find("/", nPos1 + 1);
		if (nPos2 > 0 && nPos2 != string::npos)
		{
			memcpy(app, szClientIP + nPos1 + 1, nPos2 - nPos1 - 1);
			memcpy(stream, szClientIP + nPos2 + 1, strlen(szClientIP) - nPos2 - 1);
			return true;
		}
		else
			return false;
	}else 
	  return true;
}

//���͵�һ������
int CNetClientRecvRtmp::SendFirstRequst()
{
	char szApp[string_length_1024] = { 0 }, szStream[string_length_2048] = { 0 };
	if (!GetAppStreamByURL(szApp, szStream))
	{
		WriteLog(Log_Debug, "CNetClientRecvRtmp = %X ��ȡrtmp�е�app��stream ���� ,url = %s, nClient = %llu \r\n", this,szClientIP, nClient);

		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%llu}", IndexApiCode_ConnectFail," [app��stream] Error", hParent);
		ResponseHttp(nClient_http, szResponseBody, false);

		pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));

		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
	}

	if (strlen(m_szShareMediaURL) > 0 )
	{
		pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hParent, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
		if (pMediaSource)
		{
			pMediaSource->enable_mp4 = (strcmp(m_addStreamProxyStruct.enable_mp4, "1") == 0) ? true : false;
			pMediaSource->enable_hls = (strcmp(m_addStreamProxyStruct.enable_hls, "1") == 0) ? true : false;
			pMediaSource->fileKeepMaxTime = atoi(m_addStreamProxyStruct.fileKeepMaxTime);
			pMediaSource->videoFileFormat = atoi(m_addStreamProxyStruct.videoFileFormat);
		}
		else 
		{
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		}
 	}
	flvDemuxer = flv_demuxer_create(NetRtmpClientRecvCallBackFLV, this);
	
	//rtmp�ͻ������ӣ���Ҫȥ����2��·�� 
	string strRtmpURL = szClientIP;
	int nPos = strRtmpURL.rfind("/", strlen(szClientIP));
	if (nPos <= 0 && nPos != string::npos)
	{
		WriteLog(Log_Debug, "CNetClientRecvRtmp = %X rtmp�е�url���� ,url = %s, nClient = %llu \r\n", this, szClientIP, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));

		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
 	}
	szClientIP[nPos] = 0x00;

	rtmp = rtmp_client_create(szApp, szStream,szClientIP, this, &handler);
	if (rtmp == NULL)
	{
		WriteLog(Log_Debug, "CNetClientRecvRtmp = %X rtmp����ʧ�� ,url = %s, nClient = %llu \r\n", this, szClientIP, nClient);

		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%llu}", IndexApiCode_ConnectFail, "Connect Failed .", hParent);
		ResponseHttp(nClient_http, szResponseBody, false);

		pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));

		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
	}
	int  r = rtmp_client_start(rtmp, 1);

	return 0;
}

//����m3u8�ļ�
bool  CNetClientRecvRtmp::RequestM3u8File()
{
	return true;
}