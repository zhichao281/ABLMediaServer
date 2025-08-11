/*
���ܣ�
        ʵ��hls����������m3u8�ļ�������ts�������Ϊ��׼������������ý��Դ��ʵ�ָ��ָ�ʽý����� 
����    2021-06-07
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientRecvHttpHLS.h"
#ifdef USE_BOOST
extern bool                                   DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                             pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                   ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL);
extern boost::shared_ptr<CMediaStreamSource>  CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern bool                                   DeleteMediaStreamSource(char* szURL);


#else
extern bool                                   DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                             pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                   ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL);
extern std::shared_ptr<CMediaStreamSource>  CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern bool                                   DeleteMediaStreamSource(char* szURL);


#endif
extern MediaServerPort                       ABL_MediaServerPort;
extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result, uint16_t nLocalPort);

extern void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);

const char* ftimestamp(int64_t t, char* buf)
{
	if (PTS_NO_VALUE == t)
	{
		sprintf(buf, "(null)");
	}
	else
	{
		t /= 90;
		sprintf(buf, "%d:%02d:%02d.%03d", (int)(t / 3600000), (int)((t / 60000) % 60), (int)((t / 1000) % 60), (int)(t % 1000));
	}
	return buf;
}

static int on_hls_ts_packet(void* param, int program, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	static char s_pts[64], s_dts[64];
	CNetClientRecvHttpHLS* pClient = (CNetClientRecvHttpHLS*)param;
	if (pClient == NULL)
		return 0;
 
	if (pClient->pMediaSource == NULL || !pClient->bRunFlag)
		return -1 ;

	if (PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_OPUS == avtype)
	{
		static int64_t a_pts = 0, a_dts = 0;
		if (PTS_NO_VALUE == dts)
			dts = pts;
		//assert(0 == a_dts || dts >= a_dts);
		//printf("[A][%d:%d] pts: %s(%lld), dts: %s(%lld), diff: %03d/%03d, bytes: %u\n", program, stream, ftimestamp(pts, s_pts), pts, ftimestamp(dts, s_dts), dts, (int)(pts - a_pts) / 90, (int)(dts - a_dts) / 90, (unsigned int)bytes);
		a_pts = pts;
		a_dts = dts;
        
		//pClient->hlsAudioFifo.push((unsigned char*)data, bytes);

#ifdef SaveAudioToAACFile
		 if (pClient->fileSaveAAC)
		 {
			 fwrite((unsigned char*)data, 1, bytes , pClient->fileSaveAAC);
			 fflush(pClient->fileSaveAAC);
		 }
#endif
	}
	else if (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype)
	{
		if (PSI_STREAM_H264 == avtype)
		{
			if (strlen(pClient->mediaCodecInfo.szVideoName) == 0)
				strcpy(pClient->mediaCodecInfo.szVideoName, "H264");
  		}
		else if (PSI_STREAM_H265 == avtype)
		{
			if (strlen(pClient->mediaCodecInfo.szVideoName) == 0)
				strcpy(pClient->mediaCodecInfo.szVideoName, "H265");
		}
 		pClient->hlsVideoFifo.push((unsigned char*)data, bytes);

		if (!pClient->bUpdateVideoFrameSpeedFlag && pClient->nOldPTS != 0)
		{//������ƵԴ��֡�ٶ�
			int nVideoSpeed = 25;
			if((pts - pClient->nOldPTS) != 0)
			  nVideoSpeed =  90000 / (pts - pClient->nOldPTS );

			if (nVideoSpeed > 0 && pClient->pMediaSource != NULL)
			{
				pClient->bUpdateVideoFrameSpeedFlag = true;
				WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pClient->nClient, pClient->pMediaSource->m_szURL, pClient->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pClient->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pClient->netBaseNetType);

				sprintf(pClient->szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", pClient->hParent);
				pClient->ResponseHttp(pClient->nClient_http, pClient->szResponseBody, false);
			}
		}
		pClient->nOldPTS = pts;
	}
	else
	{
		static int64_t x_pts = 0, x_dts = 0;
		//assert(0 == x_dts || dts >= x_dts);
		//printf("[%d][%d:%d] pts: %s(%lld), dts: %s(%lld), diff: %03d/%03d%s\n", avtype, program, stream, ftimestamp(pts, s_pts), pts, ftimestamp(dts, s_dts), dts, (int)(pts - x_pts) / 90, (int)(dts - x_dts) / 90, flags ? " [I]" : "");
		x_pts = pts;
		x_dts = dts;
		//assert(0);
	}
	return 0;
}

static void mpeg_ts_dec_testonstream(void* param, int stream, int codecid, const void* extra, int bytes, int finish)
{
	printf("stream %d, codecid: %d, finish: %s\n", stream, codecid, finish ? "true" : "false");
}

struct ts_demuxer_notify_t hls_notify = {
	mpeg_ts_dec_testonstream,
};

void  CNetClientRecvHttpHLS::AddAdtsToAACData(unsigned char* szData, int nAACLength)
{
	int len = nAACLength + 7;
	uint8_t profile = 2;
	uint8_t sampling_frequency_index = 8;
	uint8_t channel_configuration = 1;
	aacData[0] = 0xFF; /* 12-syncword */
	aacData[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
	aacData[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
	aacData[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
	aacData[4] = (uint8_t)(len >> 3);
	aacData[5] = ((len & 0x07) << 5) | 0x1F;
	aacData[6] = 0xFC | ((len / 1024) & 0x03);

	memcpy(aacData + 7, szData, nAACLength);
	hlsAudioFifo.push(aacData, nAACLength + 7);
}

CNetClientRecvHttpHLS::CNetClientRecvHttpHLS(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
#ifdef SaveAudioToAACFile
	char szAACFile[256] = { 0 };
	sprintf(szAACFile, "D:\\%X_%d.aac", this,rand());
	fileSaveAAC = fopen(szAACFile,"wb");
#endif
	strcpy(mediaCodecInfo.szAudioName, "AAC");
	nOldPTS = 0;
	nCallBackVideoTime = GetTickCount();
	nOldRequestM3u8Number = DefaultM3u8Number;
	requestFileFifo.InitFifo(1024 * 64);
	strcpy(m_szShareMediaURL, szShareMediaURL);

	MaxNetDataCacheCount = MaxHttp_HLSCNetCacheBufferLength + 4;
	memset(netDataCache, 0x00, sizeof(netDataCache));
	netDataCacheLength =  nNetStart = nNetEnd = 0;

	strcpy(szHttpURL, szIP);
	memset(szRequestM3u8File, 0x00, sizeof(szRequestM3u8File));
	int nPos;
	string strRequestUrl = szIP;
	nPos = strRequestUrl.find("/", 8);
	if (nPos > 0 && nPos != string::npos)
	{
		memcpy(szRequestM3u8File, szIP + nPos , strlen(szIP) - nPos );
		requestFileFifo.push((unsigned char*)szRequestM3u8File,strlen(szRequestM3u8File));
	}

	int ret;
	if (ParseRtspRtmpHttpURL(szHttpURL))
	{
	   ret = XHNetSDK_Connect((int8_t*)(m_rtspStruct.szIP), atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, 8000, 1);
	}

	nContentBodyLength = MaxDefaultContentBodyLength;
	pContentBody = new unsigned  char[nContentBodyLength];//���� 
	nContentLength = 0; //ʵ�ʳ���
	nRecvContentLength = 0;//�Ѿ��յ��ĳ���
	bRecvHttpHeadFlag = false ;//��δ�������Http ͷ
	nSendTsFileTime = GetTickCount();
	netBaseNetType = NetBaseNetType_HttpHLSClientRecv;//HLS ��������
	nHLSRequestFileStatus = HLSRequestFileStatus_NoRequsetFile;
	bCanRequestM3u8File = true;

	ts = ts_demuxer_create(on_hls_ts_packet, this);
	ts_demuxer_set_notify(ts, &hls_notify, this);

	memset(szSourceURL, 0x00, sizeof(szSourceURL));
	NetDataFifo.InitFifo(1024*1024*2);
	pMediaSource = NULL;
	hlsVideoFifo.InitFifo(MaxDefaultMediaFifoLength);
	hlsAudioFifo.InitFifo(1024 * 512);
	bRunFlag = bExitCallbackThreadFlag = true  ;

#if  0
	strcpy(szSourceURL, "/Media/Camera_00002");
#endif

#ifdef  SaveTSBufferToFile
	 nTsFileOrder = 1 ;
#endif
	WriteLog(Log_Debug, "CNetClientRecvHttpHLS= %X, ����  ,nClient = %llu ��szRUL = %s ", this,nClient,szIP);
}

CNetClientRecvHttpHLS::~CNetClientRecvHttpHLS()
{
	std::lock_guard<std::mutex> lock(netDataLock);
	bRunFlag = false;
	WriteLog(Log_Debug, "CNetClientRecvHttpHLS= %X, ��ʼ����HLS nClient = %llu ", this, nClient);
	requestFileFifo.FreeFifo();
 
	if (ts != NULL)
	{
	  ts_demuxer_flush(ts);
	  ts_demuxer_destroy(ts);
	  ts = NULL;
	}

	NetDataFifo.FreeFifo();
	hlsVideoFifo.FreeFifo();
	hlsAudioFifo.FreeFifo();

	SAFE_ARRAY_DELETE(pContentBody);
#ifdef SaveAudioToAACFile
	if (fileSaveAAC != NULL)
	{
		fclose(fileSaveAAC);
		fileSaveAAC = NULL;
	 }
#endif
	if (pMediaSource && strlen(m_szShareMediaURL) > 0)
	{
		DeleteMediaStreamSource(m_szShareMediaURL);
		pMediaSource = NULL;
	}
	malloc_trim(0);

	WriteLog(Log_Debug, "CNetClientRecvHttpHLS= %X, ���� nClient = %llu ", this, nClient);
}

int CNetClientRecvHttpHLS::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(netDataLock);

	//������߼��
	nRecvDataTimerBySecond = 0;

	if (nDataLength <= 0)
		return 0;

	NetDataFifo.push(pData, nDataLength);
	return 0;
}

//����m3u8�ļ�
bool  CNetClientRecvHttpHLS::RequestM3u8File()
{
	std::lock_guard<std::mutex> lock(netDataLock);

	//����ļ�FIFOΪ�գ�֤��TS�ļ�ȫ��������ϣ���Ҫ����m3u8�ļ���FIFO 
	if (requestFileFifo.GetSize() == 0 && //fifo Ϊ��
		nHLSRequestFileStatus     == HLSRequestFileStatus_RequestSuccess  //TS�ļ��������
		)	
	{
		//����������m3u8�ļ�
		if (!bCanRequestM3u8File || GetTickCount() - nRequestM3u8Time < 2000 )
			return false;

 		requestFileFifo.push((unsigned char*)szRequestM3u8File, strlen(szRequestM3u8File));
		SendFirstRequst(); 
		return true;
	}
	else
	{
		if ( (GetTickCount() - nSendTsFileTime) > 1000 * 6  &&
			nHLSRequestFileStatus !=  HLSRequestFileStatus_RequestSuccess )
		{//����TS��mp4�ļ���ʱ����������
			requestFileFifo.pop_front(); //ɾ����ǰ�����ļ� 
			bCanRequestM3u8File = true;
			nRecvContentLength = 0;
			nSendTsFileTime = GetTickCount();
			nHLSRequestFileStatus = HLSRequestFileStatus_RequestSuccess;

			requestFileFifo.push((unsigned char*)szRequestM3u8File, strlen(szRequestM3u8File));
			SendFirstRequst();

			WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, ����TS�ļ���ʱ szRequestFileName = %s , ������һ�� , nClient = %llu \r\n", this, szRequestFile, nClient);
			return true;
		}

		return false;
	}
	return false;
}

int CNetClientRecvHttpHLS::ProcessNetData()
{
	//WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X,  �յ����ݳ��� = %d, nClient = %llu ", this, nDataLength, nClient);

	nRecvDataTimerBySecond = 0;//������߼��
	int   nPos, i;
	char  szContentValue[string_length_1024] = { 0 };
	unsigned char  szReturnFlag[4] = { 0x0d,0x0a,0x0d,0x0a };
	unsigned char* pData = NULL;
	int            nDataLength = 0;
	int            nSize = 0;
	int            nLength;

	nSize = NetDataFifo.GetSize();
	for (int i = 0; i < nSize; i++)
	{
		pData = NetDataFifo.pop(&nDataLength);
		if (pData == NULL)
			return 0;

		if (!bRecvHttpHeadFlag)
		{
			if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
			{//ʣ��ռ��㹻
				memcpy(netDataCache + nNetEnd, pData, nDataLength);
				netDataCacheLength += nDataLength;
				nNetEnd += nDataLength;
			}
			netDataCache[nNetEnd] = 0x00;
			//WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X,  �յ����� = %s, nClient = %llu ", this, netDataCache, nClient);
			nPos = -1;
			for (int i = 0; i < netDataCacheLength; i++)
			{
				if (memcmp(netDataCache + i, szReturnFlag, 4) == 0)
				{
					nPos = i;
					break;
				}
			}
			if (nPos == -1)
			{
				NetDataFifo.pop_front();
				WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, httpͷ��δ�������� szRequestFileName = %s, nClient = %llu ", this, szRequestFile, nClient);
				return -1;
			}

			nHLSRequestFileStatus = HLSRequestFileStatus_RecvHttpHead;

			memset(szResponseHead, 0x00, sizeof(szResponseHead));
			memcpy((char*)szResponseHead, (char*)netDataCache + nNetStart, nPos - nNetStart + 4);
			netDataCacheLength -= (nPos - nNetStart + 4);
			nNetStart = nPos + 4;

			httpParse.ParseSipString((char*)szResponseHead);

			//������ļ�������
			if (httpParse.GetFieldValue("Content-Length", szContentValue) == false)
			{
				nHLSRequestFileStatus = HLSRequestFileStatus_RequestSuccess;
				nNetStart = nNetEnd = netDataCacheLength = nRecvContentLength = 0;
				WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X,������ļ������� szRequestFileName = %s, nClient = %llu ", this, szRequestFile, nClient);
				NetDataFifo.pop_front();
				return -1;
			}

			bRecvHttpHeadFlag = true;
			nContentLength = atoi(szContentValue);
			if (nContentLength > nContentBodyLength)
			{//������ݴ���Ĭ��ֵ����Ҫ���·����ڴ�
				delete[] pContentBody;
				pContentBody = NULL;

				nContentBodyLength = nContentLength + (1024 * 1024 * 1);
				pContentBody = new unsigned char[nContentBodyLength];
			}

			nRecvContentLength = 0;
			if (netDataCacheLength > 0)
			{//��ʣ��,����С�ڵ��� nContentLength
				memcpy(pContentBody + nRecvContentLength, netDataCache + nNetStart, netDataCacheLength);

				nRecvContentLength += netDataCacheLength;
				nNetStart += netDataCacheLength;

				netDataCacheLength = 0;
			}
		}
		else
		{
			if (nContentBodyLength - nRecvContentLength > nDataLength)
			{//ƴ��Content 
				memcpy(pContentBody + nRecvContentLength, pData, nDataLength);

				nRecvContentLength += nDataLength;
			}
		}
		NetDataFifo.pop_front();//�Ƴ���ʹ�ù������� 

		if (nRecvContentLength >= nContentLength)
		{//�������
			pContentBody[nRecvContentLength] = 0x00;
			if (strstr(szRequestFile, ".m3u8") != NULL)
			{//����m3u8�ļ�����TS�ļ� ����fifo 
				AddM3u8ToFifo((char*)pContentBody, strlen((char*)pContentBody));
				WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, ����M3U8�ļ���ϣ��ֽڴ�СΪ%d , szRequestFileName = %s, nClient = %llu \r\n", this, nContentLength, szRequestFile, nClient);
			}
			else
			{//TS 
 			   nPos = 0 ;
			   while (nRecvContentLength > TsStreamBlockBufferLength)
			   {
			 	   ts_demuxer_input(ts, pContentBody + nPos , TsStreamBlockBufferLength);
			 	   nPos                +=    TsStreamBlockBufferLength;
			       nRecvContentLength  -=    TsStreamBlockBufferLength;
			   }

#ifdef  SaveTSBufferToFile
				char szTsFile[256] = { 0 };
				string strRequstTsFile = szRequestFile;
				int    nPos;
				char   szTemp2[256] = { 0 };
				nPos = strRequstTsFile.rfind("/", strlen(szRequestFile));
				memcpy(szTemp2, szRequestFile + nPos + 1, strlen(szRequestFile) - nPos);

				sprintf(szTsFile, "%s%s", ABL_MediaSeverRunPath, szTemp2);
				FILE* fTsFile = fopen(szTsFile, "wb");
				if (fTsFile)
				{
					fwrite(pContentBody, 1, nContentLength, fTsFile);
					fclose(fTsFile);
				}
				nTsFileOrder++;
#endif
				WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, ����TS�ļ���ϣ��ֽڴ�СΪ%d , szRequestFileName = %s, nClient = %llu \r\n", this, nContentLength, szRequestFile, nClient);
			}

			bRecvHttpHeadFlag = false;//��δ�������Http ͷ
			nHLSRequestFileStatus = HLSRequestFileStatus_RequestSuccess;

			requestFileFifo.pop_front(); //�Ƴ����Ѿ�������ϵ��ļ�

			//������һ���ļ�
			if (requestFileFifo.GetSize() > 0)
				SendFirstRequst();
			else
			{
				nRequestM3u8Time = GetTickCount();
				bCanRequestM3u8File = true;
			}
 		}
	}
	return 0;
}

int CNetClientRecvHttpHLS::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0 ;
}

int CNetClientRecvHttpHLS::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0 ;
}

int CNetClientRecvHttpHLS::SendVideo()
{
 	int nSize;
	int nLength;
	unsigned char* pData;
 
	nSize = hlsVideoFifo.GetSize();
	if(nSize > 3 && (GetTickCount64() - nCallBackVideoTime) >= 20)
	{
 	     pData = hlsVideoFifo.pop(&nLength);

		if(pMediaSource && pData != NULL && nLength > 0)
		   pMediaSource->PushVideo(pData, nLength, mediaCodecInfo.szVideoName);

		hlsVideoFifo.pop_front();
		nCallBackVideoTime = GetTickCount();
	}

	return 0 ;
}

int CNetClientRecvHttpHLS::SendAudio()
{
	int nSize, nAudioSize;
	int nLength;
	unsigned char* pData;

	//�ص���Ƶ
	nAudioSize = hlsAudioFifo.GetSize();
	if (nAudioSize >= 3)
	{
		for (int i = 0; i < 3; i++)
		{
			pData = hlsAudioFifo.pop(&nLength);

			if(pMediaSource)
			  pMediaSource->PushAudio((unsigned char*)pData, nLength, "AAC", 1, 16000);

			hlsAudioFifo.pop_front();
		}
	}
	return 0 ;
}	

//���͵�һ������
int CNetClientRecvHttpHLS::SendFirstRequst()
{
	memset(szRequestBuffer, 0x00, sizeof(szRequestBuffer));
	unsigned char* pData;
	int            nLength;
	int            nWriteRet;

	nSendTsFileTime = GetTickCount();
	nContentLength = 0; //ʵ�ʳ���
	nRecvContentLength = 0;//�Ѿ��յ��ĳ���
 	nNetStart = nNetEnd = netDataCacheLength = nRecvContentLength = 0;
	memset(netDataCache, 0x00, sizeof(netDataCache));
 
	pData = requestFileFifo.pop(&nLength);
	if (pData != NULL && nLength > 0)
	{
		//����ý��ַ�Դ
		if (strlen(m_szShareMediaURL) > 0 && pMediaSource == NULL )
		{
			pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
			if(pMediaSource == NULL)
			{
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
				return -1;
			}
			pMediaSource->enable_mp4 = (strcmp(m_addStreamProxyStruct.enable_mp4, "1") == 0) ? true : false;
			pMediaSource->enable_hls = (strcmp(m_addStreamProxyStruct.enable_hls, "1") == 0) ? true : false;
			pMediaSource->fileKeepMaxTime = atoi(m_addStreamProxyStruct.fileKeepMaxTime);
		}
		bRecvHttpHeadFlag = false;//��δ�������httpͷ
	    memset(szRequestFile, 0x00, sizeof(szRequestFile));
		memcpy(szRequestFile, (char*)pData, nLength);
		sprintf(szRequestBuffer, "GET %s HTTP/1.1\r\nHost: 10.0.0.239:9088\r\nAccept: */*\r\nConnection: keep-alive\r\nAccept-Language: zh_CN\r\nUser-Agent: %s\r\nRange: bytes=0-\r\n\r\n",
			szRequestFile,
			MediaServerVerson);

		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)szRequestBuffer, strlen(szRequestBuffer), ABL_MediaServerPort.nSyncWritePacket);
		if (nWriteRet != 0 )
		{
			WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, ���������ļ�ʧ�� szRequestFileName = %s, nClient = %llu ", this, szRequestFile, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		nHLSRequestFileStatus = HLSRequestFileStatus_SendRequest;

 	    nSendTsFileTime = GetTickCount();

		WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, �����ļ����� szRequestFileName = %s, nClient = %llu ", this, szRequestFile, nClient);
	}

	return 0;
}

bool   CNetClientRecvHttpHLS::AddM3u8ToFifo(char* szM3u8Data, int nDataLength)
{
	string strM3u8Data = szM3u8Data;
	int    nStart = 0;
	int    nPos,nPos2,nPos3;
	char   szLine[string_length_1024];
	string strLine;
	bool   bEndFlag = false;
	char   szTemp[string_length_1024] = { 0 };
	char   szSubPath[string_length_1024] = { 0 };
	int64_t  nNumberTemp;

	//WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X ,nClient =%llu ,szM3u8Data = %s ", this,nClient, szM3u8Data);

	while (!bEndFlag)
	{
		nPos = strM3u8Data.find("\n", nStart);
		memset(szLine, 0x00, sizeof(szLine));
		if (nPos > 0 && nPos != string::npos)
		{
			memcpy(szLine, szM3u8Data + nStart, nPos - nStart);
			nStart = nPos + 1;
		}
		else
		{
			bEndFlag = true;
			memcpy(szLine, szM3u8Data + nStart, strlen(szM3u8Data) - nStart);
		}

		if (strlen(szLine) > 0)
		{
			memset(szTemp, 0x00, sizeof(szTemp));
			strLine = szLine;
			nPos2 = strLine.find("SEQUENCE:", 0);
			if (nPos2 > 0 && nPos2 != string::npos)
			{
				memcpy(szTemp, szLine + nPos2 + strlen("SEQUENCE:"), strlen(szLine) - nPos2);
				nNumberTemp = atoi(szTemp);
			}
			else
			{
				nPos2 = strLine.find("#EXT", 0);
				if (nPos2 < 0 && nPos2 != string::npos)
				{
 					if (nOldRequestM3u8Number != nNumberTemp)
					{
					 // WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, ��űȽ� nOldRequestM3u8Number = %d, nNumberTemp = %llu ", this, nOldRequestM3u8Number, nNumberTemp);

					  string strOldPath = szRequestM3u8File;
					  nPos3 = strOldPath.rfind("/", strlen(szRequestM3u8File));
					  if (nPos3 > 0 && nPos3 != string::npos && strlen(szSubPath) == 0)
					  {
						  memset(szSubPath, 0x00, sizeof(szSubPath));
						  memcpy(szSubPath, szRequestM3u8File, nPos3);
					  }
					  sprintf(szTemp, "%s/%s", szSubPath, szLine);
					  if (FindTsFileAtHistoryList(szLine) == false)
					  {//������ļ������ظ�
						  bCanRequestM3u8File = false; //����������m3u8 
						  requestFileFifo.push((unsigned char*)szTemp, strlen(szTemp));
						  WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, ��������TS�ļ� szLine = %s, nClient = %llu ", this, szTemp, nClient);
					  }
					}
				}
			}
		}
	}

	//����m3u8���
	if (nOldRequestM3u8Number != nNumberTemp)
 	  nOldRequestM3u8Number = nNumberTemp;

	HistoryM3u8 hisM3u8;
	hisM3u8.nRecvTime = GetTickCount();
	strcpy(hisM3u8.szM3u8Data, szM3u8Data);
	historyM3u8List.push_back(hisM3u8);

	return true;
}

//����m3u8�ļ��Ƿ�����ʷlist����
bool   CNetClientRecvHttpHLS::FindTsFileAtHistoryList(char* szTsFile)
{
	bool  bFind = false;
	HistoryM3u8List::iterator it;
	for (it = historyM3u8List.begin(); it != historyM3u8List.end();)
	{
		if (strstr((*it).szM3u8Data, szTsFile) != NULL)
		{//�ҵ�
			bFind = true;
			//WriteLog(Log_Debug, "CNetClientRecvHttpHLS=%X, ������ʷ�ļ� szTsFile = %s, nClient = %llu ", this, szTsFile, nClient);
			break;
		}
		else
		{
			if (GetTickCount() - (*it).nRecvTime > 1000 * 30)
			{
				historyM3u8List.erase(it++);
			}else
				++it;
		}
	}
	return bFind;
}

