/*
���ܣ�
    ʵ��flv�ͻ��˵Ľ���ģ��   
	 
����    2021-07-19
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientRecvFLV.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

#endif

extern MediaServerPort                       ABL_MediaServerPort;
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern int                                   SampleRateArray[] ;
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
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

static int NetClientRecvFLVCallBack(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
	CNetClientRecvFLV* pClient = (CNetClientRecvFLV*)param;

	static char s_pts[64], s_dts[64];
	static uint32_t v_pts = 0, v_dts = 0;
	static uint32_t a_pts = 0, a_dts = 0;

	//printf("[%c] pts: %s, dts: %s, %u, cts: %d, ", flv_type(codec), ftimestamp(pts, s_pts), ftimestamp(dts, s_dts), dts, (int)(pts - dts));
	if (pClient == NULL || pClient->pMediaSource == NULL || !pClient->bRunFlag.load())
		return 0;

	if (FLV_AUDIO_AAC == codec)
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
	}
	else if (FLV_VIDEO_H264 == codec || FLV_VIDEO_H265 == codec)
	{
		//printf("diff: %03d/%03d %s", (int)(pts - v_pts), (int)(dts - v_dts), flags ? "[I]" : "");
		v_pts = pts;
		v_dts = dts;

		//unsigned char* pVideoData = (unsigned char*)data;
		//WriteLog(Log_Debug, "CNetClientRecvRtmp=%X ,nClient = %llu, rtmp ����ص� %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X , timeStamp = %d ,datasize = %d ", pClient, pClient->nClient, (unsigned char*)pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4], pVideoData[5], pVideoData[6], pVideoData[7], pVideoData[8], pVideoData[9], pVideoData[10], pVideoData[11], pVideoData[12],dts,bytes);
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
			}
		}

		if (pClient->pMediaSource)
		{
			if (FLV_VIDEO_H264 == codec)
				pClient->pMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
			else if(FLV_VIDEO_H265 == codec)
				pClient->pMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
		}

		//unsigned char* pVideoData = (unsigned char*)data;
		//WriteLog(Log_Debug, "CNetRtspServer=%X ,nClient = %llu, rtmp ����ص� %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X , timeStamp = %d ,datasize = %d ", pClient, pClient->nClient, (unsigned char*)pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4], pVideoData[5], pVideoData[6], pVideoData[7], pVideoData[8], pVideoData[9], pVideoData[10], pVideoData[11], pVideoData[12],dts,bytes);

#ifdef  WriteHTTPFlvToEsFileFlag
 		if (pClient != NULL)
		{
			if (pClient->bStartWriteFlag == false && pClient->CheckVideoIsIFrame("H264",(unsigned char*)data, bytes))
 				pClient->bStartWriteFlag = true;

			if (pClient->bStartWriteFlag)
			{
				fwrite(data, 1, bytes, pClient->fWriteVideo);
				fflush(pClient->fWriteVideo);
			}
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

CNetClientRecvFLV::CNetClientRecvFLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	bCheckRtspVersionFlag = false;
	bDeleteRtmpPushH265Flag = false;
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	strcpy(m_szShareMediaURL, szShareMediaURL);

	int r;

	pMediaSource = NULL;
	flvDemuxer = NULL;
	nWriteRet = 0;
	nWriteErrorCount = 0;

 	if (ParseRtspRtmpHttpURL(szIP) == true)
 		uint32_t  ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1, memcmp(m_rtspStruct.szSrcRtspPullUrl, "https://", 8) == 0 ? true : false );
 
	nVideoDTS = 0;
	nAudioDTS = 0;
	memset(szRtmpName, 0x00, sizeof(szRtmpName));
	reader = NULL;
	netDataCacheLength = nNetStart = nNetEnd = 0;
	netBaseNetType = NetBaseNetType_HttpFlvClientRecv;

	WriteLog(Log_Debug, "CNetClientRecvFLV ���� = %X nClient = %llu ", this, nClient);
}

CNetClientRecvFLV::~CNetClientRecvFLV()
{
	bRunFlag.exchange(false) ;
	std::lock_guard<std::mutex> lock(NetClientRecvFLVLock);

	//�������쳣�Ͽ�
	if (bUpdateVideoFrameSpeedFlag == false)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"faied. Abnormal didconnection \",\"key\":%llu}", IndexApiCode_RecvRtmpFailed, 0);
		ResponseHttp2(nClient_http, szResponseBody, false);
	}

	if (reader)
		flv_reader_destroy(reader);

	if (flvDemuxer)
		flv_demuxer_destroy(flvDemuxer);

	 //����ǽ������������ҳɹ����������ģ�����Ҫɾ��ý������Դ szURL ������ /Media/Camera_00001 
	if(strlen(m_szShareMediaURL) >0 && pMediaSource != NULL )
		pDisconnectMediaSource.push((unsigned char*)m_szShareMediaURL, strlen(m_szShareMediaURL));

#ifdef  SaveNetDataToFlvFile
	if (fileFLV != NULL)
		fclose(fileFLV);
#endif

#ifdef  WriteHTTPFlvToEsFileFlag
	 fclose(fWriteVideo); 
#endif

	WriteLog(Log_Debug, "CNetClientRecvFLV ���� = %X nClient = %llu \r\n", this, nClient);
	
	malloc_trim(0);
}

int CNetClientRecvFLV::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetClientRecvFLV::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}
int CNetClientRecvFLV::SendVideo()
{
	return 0;
}

int CNetClientRecvFLV::SendAudio()
{

	return 0;
}

int CNetClientRecvFLV::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(NetClientRecvFLVLock);
	if(!bRunFlag.load())
		return -1 ;
	
#ifdef  SaveNetDataToFlvFile
	nNetPacketNumber++;
	if (nNetPacketNumber > 1 && fileFLV && nDataLength > 0)
	{
		fwrite(pData, 1, nDataLength, fileFLV);
		fflush(fileFLV);
	}
#endif

	nRecvDataTimerBySecond = 0;
 
	if (bRecvHttp200OKFlag == false)
	{//ȥ��http�ظ��İ�ͷ
		unsigned char szHttpEndFlag[4] = { 0x0d,0x0a,0x0d,0x0a };
		int nPos = 0;
		for (int i = 0; i < nDataLength; i++)
		{
			if (memcmp(pData + i, szHttpEndFlag, 4) == 0)
			{
				nPos = i;
				break;
			}
		}
 		if (nPos > 0)
		{
			bRecvHttp200OKFlag = true;
 			if (nDataLength - (nPos + 4) > 0)
			{
				memcpy(netDataCache + nNetEnd, pData+(nPos + 4), nDataLength - (nPos + 4));
				netDataCacheLength  += nDataLength - (nPos + 4);
				nNetEnd            += nDataLength - (nPos + 4);
			}
			else
				return 0;
		}
	}

	if (HttpFlvReadPacketSize - nNetEnd >= nDataLength)
	{//ʣ��ռ��㹻
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//ʣ��ռ䲻������Ҫ��ʣ���buffer��ǰ�ƶ�
		if (netDataCacheLength > 0)
		{//���������ʣ��
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;

			if (HttpFlvReadPacketSize - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetClientRecvFLV = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return 0;
			}
		}
		else
		{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
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

//ģ���ļ���ȡ�ص����� 
static int http_flv_netRead(void* param, void* buf, int len)
{
	CNetClientRecvFLV* pHttpFlv = (CNetClientRecvFLV*)param;
	if (pHttpFlv == NULL || !pHttpFlv->bRunFlag.load())
		return 0;

	if (pHttpFlv->netDataCacheLength >= len)
	{
		memcpy(buf, pHttpFlv->netDataCache + pHttpFlv->nNetStart, len);
		pHttpFlv->nNetStart += len;
		pHttpFlv->netDataCacheLength -= len;
 
		return len;
	}
	else
		return 0;
}

int CNetClientRecvFLV::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(NetClientRecvFLVLock);
    if(!bRunFlag.load())
		return -1;
 
	if (netDataCacheLength > (1024 * 1024 * 1.256) )
	{//����1.256 M����
 		if (reader == NULL)
 		   reader = flv_reader_create2(http_flv_netRead, this);
	   
		 //����ʧ�� 
		if (reader == NULL)
		{
			bRunFlag.exchange(false);
			WriteLog(Log_Debug, "CNetClientRecvFLV = %X nClient = %llu flv_reader_create2 ����ʧ�� ,ִ��ɾ�� ", this,nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		}
		
 		while (flv_reader_read(reader, &type, &timestamp, &taglen, packet, sizeof(packet)) == 1 )
		{//��ʣ�� 1024 * 1024 ʱ����Ҫ�˳� 
			flv_demuxer_input(flvDemuxer, type, packet, taglen, timestamp);

			//Ҫʣ��Щ���ݣ����򵱶�ȡ����ͷʱ����ͷ��ָ�����ݳ��� ���� ������ʣ������ݣ��ͻᱨ�� ,��ô���i֡�ĳ���
			if (netDataCacheLength < 1024 * 768)
				break;
 		}
	}

	return 0;
}

//���͵�һ������
int CNetClientRecvFLV::SendFirstRequst()
{
	string  strHttpFlvURL = m_rtspStruct.szSrcRtspPullUrl;
	int nPos1, nPos2;
	char    szSubPath[string_length_2048] = { 0 };
	nPos1 = strHttpFlvURL.find("//", 0);
	if (nPos1 > 0 && nPos1 != string::npos)
	{
		nPos2 = strHttpFlvURL.find("/", nPos1 + 2);
		if (nPos2 > 0 && nPos2 != string::npos)
		{
			flvDemuxer = flv_demuxer_create(NetClientRecvFLVCallBack, this);

			//����ý��ַ���Դ
			if (strlen(m_szShareMediaURL) > 0)
			{
				pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hParent, MediaSourceType_LiveMedia,0, m_h265ConvertH264Struct);
				if (pMediaSource)
				{
					pMediaSource->netBaseNetType = netBaseNetType;
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

			memcpy(szSubPath, m_rtspStruct.szSrcRtspPullUrl + nPos2, strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos2);
			sprintf(szRequestFLVFile, "GET %s HTTP/1.1\r\nUser-Agent: %s\r\nAccept: */*\r\nRange: bytes=0-\r\nConnection: keep-alive\r\nHost: 190.15.240.11:8088\r\nIcy-MetaData: 1\r\n\r\n", szSubPath, MediaServerVerson);
			XHNetSDK_Write(nClient, (unsigned char*)szRequestFLVFile, strlen(szRequestFLVFile), ABL_MediaServerPort.nSyncWritePacket);
		}else
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
	}else
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));

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

//����m3u8�ļ�
bool  CNetClientRecvFLV::RequestM3u8File()
{
	return true;
}