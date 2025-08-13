/*
���ܣ�
	   ʵ��
		rtsp����
����    2021-07-31
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientSendRtsp.h"
#include "LCbase64.h"

#include "netBase64.h"
#include "Base64.hh"
#ifdef USE_BOOST
uint64_t                                     CNetClientSendRtsp::Session = 1000;
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#else
uint64_t                                     CNetClientSendRtsp::Session = 1000;
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#endif
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern size_t base64_decode(void* target, const char *source, size_t bytes);
extern MediaServerPort                       ABL_MediaServerPort;

//AAC����Ƶ�����
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

CNetClientSendRtsp::CNetClientSendRtsp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nServer = hServer;
	nClient = hClient;
	hRtpVideo = 0;
	hRtpAudio = 0;
	nSendRtpVideoMediaBufferLength = nSendRtpAudioMediaBufferLength = 0;
	nCalcAudioFrameCount = 0;
	nStartVideoTimestamp = VideoStartTimestampFlag; //��Ƶ��ʼʱ��� 
	nSendRtpFailCount = 0;//�ۼƷ���rtp��ʧ�ܴ��� 
	strcpy(m_szShareMediaURL, szShareMediaURL);

	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	nPrintCount = 0;
	bIsInvalidConnectFlag = false;

	netDataCacheLength = 0;//�������ݻ����С
	nNetStart = nNetEnd = 0; //����������ʼλ��\����λ��
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

	for (int i = 0; i < MaxRtpHandleCount; i++)
  		hRtpHandle[i] = 0 ;
 
	for (int i = 0; i < 3; i++)
		bExitProcessFlagArray[i] = true;

	m_nSpsPPSLength = 0;
	AuthenticateType = WWW_Authenticate_None;
	nSendSetupCount = 0;
	netBaseNetType = NetBaseNetType_RtspClientPush;

	for (int i = 0; i < 16; i++)
		memset(szTrackIDArray[i], 0x00, sizeof(szTrackIDArray[i]));

	if (ParseRtspRtmpHttpURL(szIP) == true)
		uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);

	nMediaCount = 0;
#ifdef WriteRtpDepacketFileFlag
	fWriteRtpVideo = fopen("d:\\rtspRecv.264", "wb");
 	fWriteRtpAudio = fopen("d:\\rtspRecv.aac", "wb");
	bStartWriteFlag = false;
#endif
	strcpy(szTrackIDArray[1], "streamid=0");
	strcpy(szTrackIDArray[2], "streamid=1");
	WriteLog(Log_Debug, "CNetClientSendRtsp ���� nClient = %llu ", nClient);
}

CNetClientSendRtsp::~CNetClientSendRtsp()
{
	WriteLog(Log_Debug, "CNetClientSendRtsp �ȴ������˳� nTime = %llu, nClient = %llu ",GetTickCount64(), nClient);
	bRunFlag.exchange(false);
 	std::lock_guard<std::mutex> lock(businessProcMutex);
	
	for (int i = 0; i < 3; i++)
	{
		while (!bExitProcessFlagArray[i])
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			//Sleep(5);
	}
	WriteLog(Log_Debug, "CNetClientSendRtsp �����˳���� nTime = %llu, nClient = %llu ", GetTickCount64(), nClient);
 
#ifdef WriteRtpDepacketFileFlag
	if(fWriteRtpVideo)
	  fclose(fWriteRtpVideo);
	if(fWriteRtpAudio)
	  fclose(fWriteRtpAudio);
#endif
	 
	if (hRtpVideo != 0)
	{
		rtp_packet_stop(hRtpVideo);
		hRtpVideo = 0;
	}
	if (hRtpAudio != 0)
	{
		rtp_packet_stop(hRtpAudio);
		hRtpAudio = 0;
	}
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();

	WriteLog(Log_Debug, "CNetClientSendRtsp ���� nClient = %llu \r\n", nClient);
	malloc_trim(0);
}
int CNetClientSendRtsp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0;
	if (!bRunFlag.load() || m_addPushProxyStruct.disableVideo[0] != 0x30 )
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	m_videoFifo.push(pVideoData, nDataLength);

	return 0;
}

int CNetClientSendRtsp::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;
	if (!bRunFlag.load() || m_addPushProxyStruct.disableAudio[0] != 0x30)
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (ABL_MediaServerPort.nEnableAudio == 0)
		return -1;

	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	m_audioFifo.push(pVideoData, nDataLength);

	return 0;
}

int CNetClientSendRtsp::SendVideo()
{
	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		inputVideo.data = pData;
		inputVideo.datasize = nLength;
		rtp_packet_input(&inputVideo);

		m_videoFifo.pop_front();
	}
	return 0;
}

int CNetClientSendRtsp::SendAudio()
{
	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		inputAudio.data = pData;
		inputAudio.datasize = nLength;
		rtp_packet_input(&inputAudio);

		m_audioFifo.pop_front();
	}
	return 0;
}

//��������ƴ�� 
int CNetClientSendRtsp::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(netDataLock);

	//������߼��
	nRecvDataTimerBySecond = 0;

	if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
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

 			if (MaxNetDataCacheCount - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetClientSendRtsp = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
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
 	return 0 ;
}

//��ȡ�������� ��ģ��ԭ���ײ�������ȡ���� 
int32_t  CNetClientSendRtsp::XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain)
{
	int nWaitCount = 0;
	bExitProcessFlagArray[0] = false;
	while (!bIsInvalidConnectFlag && bRunFlag.load())
	{
 		if (netDataCacheLength >= *buffsize)
		{
			memcpy(buffer, netDataCache + nNetStart, *buffsize);
			nNetStart += *buffsize;
			netDataCacheLength -= *buffsize;
			
			bExitProcessFlagArray[0] = true;
 			return 0;
		}
 	//	Sleep(10);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		nWaitCount ++;
		if (nWaitCount >= 100 * 3)
			break;
	}
	bExitProcessFlagArray[0] = true;

	return -1;  
}

bool   CNetClientSendRtsp::ReadRtspEnd()
{
	unsigned int nReadLength = 1;
	unsigned int nRet;
	bool     bRet = false;
	bExitProcessFlagArray[1] = false;
	while (!bIsInvalidConnectFlag && bRunFlag.load())
	{
		nReadLength = 1;
		nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
		if (nRet == 0 && nReadLength == 1)
		{
			data_Length += 1;
			if (data_Length >= 4 && data_[data_Length - 4] == '\r' && data_[data_Length - 3] == '\n' && data_[data_Length - 2] == '\r' && data_[data_Length - 1] == '\n')
			{
				bRet = true;
				break;
			}
		}
		else
		{
			WriteLog(Log_Debug, "ReadRtspEnd() ,��δ��ȡ������ ��CABLRtspClient =%X ,dwClient=%llu ", this, nClient);
			break;
		}

		if (data_Length >= RtspServerRecvDataLength)
		{
			WriteLog(Log_Debug, "ReadRtspEnd() ,�Ҳ��� rtsp �������� ��CABLRtspClient =%X ,dwClient = %llu ", this, nClient);
			break;
		}
	}
	bExitProcessFlagArray[1] = true;
	return bRet;
}

//����
int  CNetClientSendRtsp::FindHttpHeadEndFlag()
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

int  CNetClientSendRtsp::FindKeyValueFlag(char* szData)
{
	int nSize = strlen(szData);
	for (int i = 0; i < nSize; i++)
	{
		if (memcmp(szData + i, ": ", 2) == 0)
			return i;
	}
	return -1;
}

//��ȡHTTP������httpURL 
void CNetClientSendRtsp::GetHttpModemHttpURL(char* szMedomHttpURL)
{//"POST /getUserName?userName=admin&password=123456 HTTP/1.1"
	if (RtspProtectArrayOrder >= MaxRtspProtectCount)
		return;

	int nPos1, nPos2, nPos3, nPos4;
	string strHttpURL = szMedomHttpURL;
	char   szTempRtsp[string_length_2048] = { 0 };
	string strTempRtsp;

	strcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspCmdString, szMedomHttpURL);

	nPos1 = strHttpURL.find(" ", 0);
	if (nPos1 > 0)
	{
		nPos2 = strHttpURL.find(" ", nPos1 + 1);
		if (nPos1 > 0 && nPos2 > 0)
		{
			memcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspCommand, szMedomHttpURL, nPos1);

			//memcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspURL, szMedomHttpURL + nPos1 + 1, nPos2 - nPos1 - 1);
			memcpy(szTempRtsp, szMedomHttpURL + nPos1 + 1, nPos2 - nPos1 - 1);
			strTempRtsp = szTempRtsp;
			nPos3 = strTempRtsp.find("?", 0);
			if (nPos3 > 0)
			{//ȥ�������
				szTempRtsp[nPos3] = 0x00;
			}
			strTempRtsp = szTempRtsp;

			//����554 �˿�
			nPos4 = strTempRtsp.find(":", 8);
			if (nPos4 <= 0)
			{//û��554 �˿�
				nPos3 = strTempRtsp.find("/", 8);
				if (nPos3 > 0)
				{
					strTempRtsp.insert(nPos3, ":554");
				}
			}

			strcpy(RtspProtectArray[RtspProtectArrayOrder].szRtspURL, strTempRtsp.c_str());
		}
	}
}

 //��httpͷ������䵽�ṹ��
int  CNetClientSendRtsp::FillHttpHeadToStruct()
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
	char szTemp[string_length_2048] = { 0 };
	char szKey[string_length_2048] = { 0 };

	for (int i = 0; i < nHttpHeadEndLength - 2; i++)
	{
		if (memcmp(data_ + i, szHttpHeadEndFlag, 2) == 0)
		{
			memset(szTemp, 0x00, sizeof(szTemp));
			memcpy(szTemp, data_ + nPos, i - nPos);

			if ((nFlagLength = FindKeyValueFlag(szTemp)) >= 0)
			{
				memset(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey, 0x00, sizeof(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey));//Ҫ���
				memset(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue, 0x00, sizeof(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue));//Ҫ��� 

 				memcpy(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey, szTemp, nFlagLength);
				memcpy(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue, szTemp + nFlagLength + 2, strlen(szTemp) - nFlagLength - 2);

				strcpy(szKey, RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szKey);
				
#ifdef USE_BOOST
				to_lower(szKey);
#else
				ABL::to_lower(szKey);
#endif
				if (strcmp(szKey, "content-length") == 0)
				{//���ݳ���
					nContentLength = atoi(RtspProtectArray[RtspProtectArrayOrder].rtspField[nKeyCount].szValue);
					RtspProtectArray[RtspProtectArrayOrder].nRtspSDPLength = nContentLength;
				}

				nKeyCount++;

				//��ֹ������Χ
				if (nKeyCount >= MaxRtspValueCount)
					return true;
			}
			else
			{//���� http ������URL 
				GetHttpModemHttpURL(szTemp);
			} 

			nPos = i + 2;
		}
	}

	return true;
} 

bool CNetClientSendRtsp::GetFieldValue(char* szFieldName, char* szFieldValue)
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

//ͳ��rtspURL  rtsp://190.15.240.11:554/Media/Camera_00001 ·�� / ������ 
int  CNetClientSendRtsp::GetRtspPathCount(char* szRtspURL)
{
	string strCurRtspURL = szRtspURL;
	int    nPos1, nPos2;
	int    nPathCount = 0;
 	nPos1 = strCurRtspURL.find("//", 0);
	if (nPos1 < 0)
		return 0;//��ַ�Ƿ� 

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

//rtp����ص���Ƶ
void Video_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetClientSendRtsp* pRtspClient = (CNetClientSendRtsp*)cb->userdata;
	if(pRtspClient->bRunFlag.load())
	  pRtspClient->ProcessRtpVideoData(cb->data, cb->datasize);
}

//rtp ����ص���Ƶ
void Audio_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetClientSendRtsp* pRtspClient = (CNetClientSendRtsp*)cb->userdata;
	if(pRtspClient->bRunFlag.load())
	  pRtspClient->ProcessRtpAudioData(cb->data, cb->datasize);
}

void CNetClientSendRtsp::ProcessRtpVideoData(unsigned char* pRtpVideo,int nDataLength)
{
	if ((MaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0 )
	{//ʣ��ռ䲻���洢 ,��ֹ���� 
		SumSendRtpMediaBuffer(szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength);
 		nSendRtpVideoMediaBufferLength = 0;
	}

	memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nStartVideoTimestamp != VideoStartTimestampFlag &&  nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//����һ֡�µ���Ƶ 
		//WriteLog(Log_Debug, "CNetClientSendRtsp= %X, ����һ֡��Ƶ ��Length = %d ", this, nSendRtpVideoMediaBufferLength);
		SumSendRtpMediaBuffer(szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength);
		nSendRtpVideoMediaBufferLength = 0;
	}
 
	szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 0] = '$';
	szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 1] = 0;
	nVideoRtpLen = htons(nDataLength);
	memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
 	memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 4), pRtpVideo, nDataLength);
 
	nStartVideoTimestamp = nCurrentVideoTimestamp;
	nSendRtpVideoMediaBufferLength += nDataLength + 4;
}
void CNetClientSendRtsp::ProcessRtpAudioData(unsigned char* pRtpAudio, int nDataLength)
{
	if (hRtpVideo != 0)
	{
		if ((MaxRtpSendAudioMediaBufferLength - nSendRtpAudioMediaBufferLength < nDataLength + 4) && nSendRtpAudioMediaBufferLength > 0 )
		{//ʣ��ռ䲻���洢 ,��ֹ���� 
			SumSendRtpMediaBuffer(szSendRtpAudioMediaBuffer, nSendRtpAudioMediaBufferLength);

			nSendRtpAudioMediaBufferLength = 0;
			nCalcAudioFrameCount = 0;
		}

		szSendRtpAudioMediaBuffer[nSendRtpAudioMediaBufferLength + 0] = '$';
		szSendRtpAudioMediaBuffer[nSendRtpAudioMediaBufferLength + 1] = 2;
		nAudioRtpLen = htons(nDataLength);
		memcpy(szSendRtpAudioMediaBuffer + (nSendRtpAudioMediaBufferLength + 2), (unsigned char*)&nAudioRtpLen, sizeof(nAudioRtpLen));
		memcpy(szSendRtpAudioMediaBuffer + (nSendRtpAudioMediaBufferLength + 4), pRtpAudio, nDataLength);

 		nSendRtpAudioMediaBufferLength += nDataLength + 4;
 		nCalcAudioFrameCount ++;

		if (nCalcAudioFrameCount >= 3 && nSendRtpAudioMediaBufferLength > 0 )
		{
			SumSendRtpMediaBuffer(szSendRtpAudioMediaBuffer, nSendRtpAudioMediaBufferLength);
 			nSendRtpAudioMediaBufferLength = 0;
			nCalcAudioFrameCount = 0;
		}
	}
	else
	{//����������Ƶ
		nSendRtpAudioMediaBufferLength = 0;
		szSendRtpAudioMediaBuffer[nSendRtpAudioMediaBufferLength + 0] = '$';
		szSendRtpAudioMediaBuffer[nSendRtpAudioMediaBufferLength + 1] = 0;

		nAudioRtpLen = htons(nDataLength);
		memcpy(szSendRtpAudioMediaBuffer + (nSendRtpAudioMediaBufferLength + 2), (unsigned char*)&nAudioRtpLen, sizeof(nAudioRtpLen));
		memcpy(szSendRtpAudioMediaBuffer + (nSendRtpAudioMediaBufferLength + 4), pRtpAudio, nDataLength);
		XHNetSDK_Write(nClient, szSendRtpAudioMediaBuffer, nDataLength + 4,ABL_MediaServerPort.nSyncWritePacket);
	}
}

//�ۻ�rtp��������
void CNetClientSendRtsp::SumSendRtpMediaBuffer(unsigned char* pRtpMedia, int nRtpLength)
{
	std::lock_guard<std::mutex> lock(MediaSumRtpMutex);
 
	if (bRunFlag.load())
	{
		nSendRet = XHNetSDK_Write(nClient, pRtpMedia, nRtpLength, ABL_MediaServerPort.nSyncWritePacket);
		if (nSendRet != 0)
		{
			nSendRtpFailCount ++ ;
			WriteLog(Log_Debug, "����rtp ��ʧ�� ,�ۼƴ��� nSendRtpFailCount = %d �� ��nClient = %llu, nSendRet = %d ", nSendRtpFailCount, nClient, nSendRet);
			if (nSendRtpFailCount >= 30)
			{
				bRunFlag.exchange(false);			
				WriteLog(Log_Debug, "����rtp ��ʧ�� ,�ۼƴ��� �Ѿ��ﵽ nSendRtpFailCount = %d �Σ�����ɾ�� ��nClient = %llu ", nSendRtpFailCount, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			}
		}
		else
			nSendRtpFailCount = 0;
	}
}

//����ý����Ϣƴװ SDP ��Ϣ
bool CNetClientSendRtsp::GetRtspSDPFromMediaStreamSource(RtspSDPContentStruct sdpContent, bool bGetFlag)
{
	memset(szRtspSDPContent, 0x00, sizeof(szRtspSDPContent));

	//��Ƶ
	nVideoSSRC = rand();
	memset((char*)&optionVideo, 0x00, sizeof(optionVideo));
	if (strcmp(sdpContent.szVideoName,"H264") == 0)
	{
		optionVideo.streamtype = e_rtppkt_st_h264;
		if (bGetFlag)
			nVideoPayload = sdpContent.nVidePayload;
		else
			nVideoPayload = 96;
		sprintf(szRtspSDPContent, "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\ns=No Name\r\nc=IN IP4 190.15.240.36\r\nt=0 0\r\na=tool:libavformat 55.19.104\r\nm=video 6000 RTP/AVP %d\r\nb=AS:832\r\na=rtpmap:%d H264/90000\r\na=fmtp:%d packetization-mode=1\r\na=control:streamid=0\r\n", nVideoPayload, nVideoPayload, nVideoPayload);
		nMediaCount ++;
	}
	else if (strcmp(sdpContent.szVideoName, "H265") == 0)
	{
		optionVideo.streamtype = e_rtppkt_st_h265;
		if (bGetFlag)
			nVideoPayload = sdpContent.nVidePayload;
		else
		    nVideoPayload = 97;
		sprintf(szRtspSDPContent, "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\ns=No Name\r\nc=IN IP4 190.15.240.36\r\nt=0 0\r\na=tool:libavformat 55.19.104\r\nm=video 6000 RTP/AVP %d\r\nb=AS:3007\r\na=rtpmap:%d H265/90000\r\na=fmtp:%d packetization-mode=1\r\na=control:streamid=0\r\n", nVideoPayload, nVideoPayload, nVideoPayload);
		nMediaCount++;
	}
	else if (strcmp(sdpContent.szVideoName, "") == 0)
	{
		WriteLog(Log_Debug, "CNetClientSendRtsp = %X ��û����Ƶ��Դ ,nClient = %llu", this, nClient);
	}

	int nRet = 0;
	if (strlen(sdpContent.szVideoName) > 0)
	{
		nRet = rtp_packet_start(Video_rtp_packet_callback_func_send, (void*)this, &hRtpVideo);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ��������Ƶrtp���ʧ��,nClient = %llu,  nRet = %d", this,nClient, nRet);
			return false;
		}
 		optionVideo.handle = hRtpVideo;
		optionVideo.ssrc = nVideoSSRC;
		optionVideo.mediatype = e_rtppkt_mt_video;
 		optionVideo.payload = nVideoPayload;
		optionVideo.ttincre = 90000 / mediaCodecInfo.nVideoFrameRate; //��Ƶʱ�������

		inputVideo.handle = hRtpVideo;
		inputVideo.ssrc = optionVideo.ssrc;

		nRet = rtp_packet_setsessionopt(&optionVideo);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CRecvRtspClient = %X ��rtp_packet_setsessionopt ������Ƶrtp���ʧ�� ,nClient = %llu nRet = %d", this, nClient, nRet);
			return false;
		}
	}
 	memset(szRtspAudioSDP, 0x00, sizeof(szRtspAudioSDP));
 
	memset((char*)&optionAudio, 0x00, sizeof(optionAudio));
	if (strcmp(sdpContent.szAudioName, "G711_A") == 0 && ABL_MediaServerPort.nEnableAudio == 1)
	{
		optionAudio.ttincre = 320; //g711a �� ��������
		optionAudio.streamtype = e_rtppkt_st_g711a;
		if (bGetFlag)
			nAudioPayload = sdpContent.nAudioPayload;
		else
 		   nAudioPayload = 8;
		if(hRtpVideo != 0)
		 sprintf(szRtspAudioSDP, "m=audio 0 RTP/AVP %d\r\nb=AS:50\r\na=recvonly\r\na=rtpmap:%d PCMA/%d\r\na=control:streamid=1\r\na=framerate:25\r\n",nAudioPayload, nAudioPayload, 8000);
		else
		 sprintf(szRtspAudioSDP, "m=audio 0 RTP/AVP %d\r\nb=AS:50\r\na=recvonly\r\na=rtpmap:%d PCMA/%d\r\na=control:streamid=0\r\na=framerate:25\r\n", nAudioPayload, nAudioPayload, 8000);

		nMediaCount++;
	}
	else if (strcmp(sdpContent.szAudioName, "G711_U") == 0 && ABL_MediaServerPort.nEnableAudio == 1)
	{
		optionAudio.ttincre = 320; //g711u �� ��������
		optionAudio.streamtype = e_rtppkt_st_g711u;
		if (bGetFlag)
			nAudioPayload = sdpContent.nAudioPayload;
		else
			nAudioPayload = 0;
		if (hRtpVideo != 0)
		  sprintf(szRtspAudioSDP, "m=audio 0 RTP/AVP %d\r\nb=AS:50\r\na=recvonly\r\na=rtpmap:%d PCMU/%d\r\na=control:streamid=1\r\na=framerate:25\r\n",nAudioPayload, nAudioPayload, 8000);
		else
		 sprintf(szRtspAudioSDP, "m=audio 0 RTP/AVP %d\r\nb=AS:50\r\na=recvonly\r\na=rtpmap:%d PCMU/%d\r\na=control:streamid=0\r\na=framerate:25\r\n", nAudioPayload, nAudioPayload, 8000);

		nMediaCount++;
	}
	else if (strcmp(sdpContent.szAudioName, "AAC") == 0 && ABL_MediaServerPort.nEnableAudio == 1)
	{
		optionAudio.ttincre = 1024 ;//aac �ĳ�������
		optionAudio.streamtype = e_rtppkt_st_aac;
		if (bGetFlag)
			nAudioPayload = sdpContent.nAudioPayload;
		else
			nAudioPayload = 104;
		if (hRtpVideo != 0)
		  sprintf(szRtspAudioSDP, "m=audio 0 RTP/AVP %d\r\na=rtpmap:%d MPEG4-GENERIC/%d/%d\r\na=fmtp:%d profile-level-id=15; streamtype=5; mode=AAC-hbr; config=1408;SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;\r\na=control:streamid=1\r\n",nAudioPayload, nAudioPayload, sdpContent.nSampleRate, sdpContent.nChannels, nAudioPayload);
		else
		  sprintf(szRtspAudioSDP, "m=audio 0 RTP/AVP %d\r\na=rtpmap:%d MPEG4-GENERIC/%d/%d\r\na=fmtp:%d profile-level-id=15; streamtype=5; mode=AAC-hbr; config=1408;SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;\r\na=control:streamid=0\r\n", nAudioPayload, nAudioPayload, sdpContent.nSampleRate, sdpContent.nChannels, nAudioPayload);

		nMediaCount++;
	}

	//׷����ƵSDP
	if (strlen(szRtspAudioSDP) > 0 && ABL_MediaServerPort.nEnableAudio == 1)
	{
		int32_t nRet = rtp_packet_start(Audio_rtp_packet_callback_func_send, (void*)this, &hRtpAudio);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ��������Ƶrtp���ʧ�� ,nClient = %llu,  nRet = %d", this,nClient, nRet);
			return false;
		}
		optionAudio.handle = hRtpAudio;
		optionAudio.ssrc = nVideoSSRC + 1;
		optionAudio.mediatype = e_rtppkt_mt_audio;
		optionAudio.payload = nAudioPayload;

 		inputAudio.handle = hRtpAudio;
		inputAudio.ssrc = optionAudio.ssrc;

		rtp_packet_setsessionopt(&optionAudio);

		if (hRtpVideo != 0)
		   strcat(szRtspSDPContent, szRtspAudioSDP);
		else
		   strcpy(szRtspSDPContent, szRtspAudioSDP);
	}

	if (bGetFlag)
		strcpy(szRtspSDPContent, sdpContent.szSDPContent);
	WriteLog(Log_Debug, "��װ��SDP :\r\n%s", szRtspSDPContent);

	return true;
}

//����rtsp����
void  CNetClientSendRtsp::InputRtspData(unsigned char* pRecvData, int nDataLength)
{
#if 0
	 if (nDataLength < 1024)
		WriteLog(Log_Debug, "RecvData \r\n%s \r\n", pRecvData);
#endif

    if (memcmp(data_, "RTSP/1.0 401", 12) == 0 && nRtspProcessStep == RtspProcessStep_OPTIONS && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{//�������,OPTIONS��Ҫ����md5��ʽ��֤
		if (!GetWWW_Authenticate())
		{
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return;
		}
		SendOptions(AuthenticateType);
	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_OPTIONS && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		RtspSDPContentStruct sdpContent;

		auto  pMediaSource = GetMediaStreamSource(m_szShareMediaURL, true);
		if (pMediaSource == NULL)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ,ý��Դ %s ������ , nClient = %llu ", this, m_szShareMediaURL,nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return;
		}

		//����ý��Դ
		SplitterAppStream(m_szShareMediaURL);
		sprintf(m_addStreamProxyStruct.url, "rtsp://localhost:%d/%s/%s", ABL_MediaServerPort.nRtspPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);

		//��ý��Դ����ý����Ϣ
		strcpy(sdpContent.szVideoName, pMediaSource->m_mediaCodecInfo.szVideoName);
		strcpy(sdpContent.szAudioName, pMediaSource->m_mediaCodecInfo.szAudioName);
		sdpContent.nChannels = pMediaSource->m_mediaCodecInfo.nChannels;
		sdpContent.nSampleRate = pMediaSource->m_mediaCodecInfo.nSampleRate;

		//����ý��Դ��Ϣ
		memcpy((char*)&mediaCodecInfo, (char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));

		if (GetRtspSDPFromMediaStreamSource(sdpContent, false) == false)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ,���� SDP ʧ�� , nClient = %llu ", this, nClient);

			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%d}", IndexApiCode_RtspSDPError, "rtsp push Error ", 0);
			ResponseHttp(nClient_http, szResponseBody, false);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return;
		}

		SendANNOUNCE(AuthenticateType);
   	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_ANNOUNCE && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		GetFieldValue("Session", szSessionID);
		if (strlen(szSessionID) == 0)
		{//��Щ���������Session,������ԣ������̹ر�
			string strSessionID;
			char   szTempSessionID[128] = { 0 };
			int    nPos;
			if (GetFieldValue("session", szTempSessionID))
			{
				strSessionID = szTempSessionID;
				nPos = strSessionID.find(";", 0);
				if (nPos > 0)
				{//�г�ʱ���Ҫȥ����ʱѡ��
					memcpy(szSessionID, szTempSessionID, nPos);
				}
				else
					strcpy(szSessionID, szTempSessionID);
			}
			else
				strcpy(szSessionID, "1000005");
		}
		SendSetup(AuthenticateType);
 	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_SETUP && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		if (nMediaCount == 1)
		{//ֻ��1��ý�壬ֱ�ӷ���Player
			SendRECORD(AuthenticateType);
		}
		else if (nMediaCount == 2)
		{
			if (nSendSetupCount == 1)
				SendSetup(AuthenticateType);//��Ҫ�ٷ���
			else
			{
				SendRECORD(AuthenticateType);
			}
		}

		nRecvLength = nHttpHeadEndLength = 0;
   	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_RECORD && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "�յ� RECORD �ظ����rtsp������� nClient = %llu ", nClient);
		auto pMediaSource = GetMediaStreamSource(m_szShareMediaURL, true);
		if (pMediaSource == NULL )
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X nClient = %llu ,������ý��Դ %s", this, nClient, m_szShareMediaURL);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return ;
		}
		m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
		m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);
		pMediaSource->AddClientToMap(nClient);

		bUpdateVideoFrameSpeedFlag = true; //����ɹ�����

		//�ظ�http����
		sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", hParent);
		ResponseHttp(nClient_http, szResponseBody, false);

		//�ڸ����ǳɹ�
		auto   pParentPtr = GetNetRevcBaseClient(hParent);
		if (pParentPtr && pParentPtr->bProxySuccessFlag == false)
			bProxySuccessFlag = pParentPtr->bProxySuccessFlag = true;

 	}
	else if (memcmp(pRecvData, "TEARDOWN", 8) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "�յ� TEARDOWN �������ִ��ɾ�� nClient = %llu ", nClient);
		bRunFlag.exchange(false);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
	}
	else if (memcmp(pRecvData, "GET_PARAMETER", 13) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		memset(szCSeq, 0x00, sizeof(szCSeq));
		GetFieldValue("CSeq", szCSeq);

		sprintf(szResponseBuffer, "RTSP/1.0 200 OK\r\nCSeq: %s\r\nPublic: %s\r\nx-Timeshift_Range: clock=20100318T021915.84Z-20100318T031915.84Z\r\nx-Timeshift_Current: clock=20100318T031915.84Z\r\n\r\n", szCSeq, RtspServerPublic);
		nSendRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), ABL_MediaServerPort.nSyncWritePacket);
		if (nSendRet != 0)
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
 	}
	else
	{
		//�ظ�http����
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%d}", IndexApiCode_RtspSDPError, pRecvData, 0);
		ResponseHttp(nClient_http, szResponseBody, false);

		bIsInvalidConnectFlag = true; //ȷ��Ϊ�Ƿ����� 
		WriteLog(Log_Debug, "�Ƿ���rtsp �������ִ��ɾ�� nClient = %llu , \r\n%s ",nClient, pRecvData);

		//ɾ������������������
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
	}
}

bool CNetClientSendRtsp::getRealmAndNonce(char* szDigestString, char* szRealm, char* szNonce)
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


//��ȡ��֤��ʽ
bool  CNetClientSendRtsp::GetWWW_Authenticate()
{
	if (!GetFieldValue("WWW-Authenticate", szWww_authenticate))
	{
		WriteLog(Log_Debug, "��Describe�У���ȡ���� www-authenticate ��Ϣ��\r\n");
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return false;
	}

	//ȷ����������֤����
	string strWww_authenticate = szWww_authenticate;
	int nPos1 = strWww_authenticate.find("Digest ", 0);//ժҪ��֤
	int nPos2 = strWww_authenticate.find("Basic ", 0);//������֤
	if (nPos1 >= 0)
		AuthenticateType = WWW_Authenticate_MD5;
	else if (nPos2 >= 0)
		AuthenticateType = WWW_Authenticate_Basic;
	else
	{//����ʶ����֤��ʽ��
		WriteLog(Log_Debug, "��Describe�У��Ҳ�����ʶ����֤��ʽ ��\r\n");
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return false;
	}

	if (AuthenticateType == WWW_Authenticate_MD5)
	{//ժҪ��֤ ��ȡrealm ,nonce 
		if (getRealmAndNonce(szWww_authenticate, m_rtspStruct.szRealm, m_rtspStruct.szNonce) == false)
		{
			WriteLog(Log_Debug, "��Describe�У���ȡ���� realm,nonce ��Ϣ��\r\n");
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return false;
		}
	}
	return true;
}

//������������
int CNetClientSendRtsp::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(netDataLock);

	bExitProcessFlagArray[2] = false; 
	tRtspProcessStartTime = GetTickCount64();
	while (!bIsInvalidConnectFlag && bRunFlag.load() && netDataCacheLength > 4)
	{
	    uint32_t nReadLength = 4;
		data_Length = 0;

		int nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
		if (nRet == 0 && nReadLength == 4)
		{
			data_Length = 4;
			if (data_[0] == '$')
			{//rtp ����
				memcpy((char*)&nRtpLength, data_ + 2, 2);
				nRtpLength = nReadLength = ntohs(nRtpLength);

				nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
				if (nRet == 0 && nReadLength == nRtpLength)
				{
					if (data_[1] == 0x00)
					{
					}
					else if (data_[1] == 0x02)
					{
						nPrintCount ++;
					}
					else if (data_[1] == 0x01)
					{//�յ�RTCP������Ҫ�ظ�rtcp�����
						SendRtcpReportData(nVideoSSRC, data_[1]);
						//WriteLog(Log_Debug, "this =%X ,�յ� ��Ƶ ��RTCP������Ҫ�ظ�rtcp�������netBaseNetType = %d  �յ�RCP������ = %d ", this, netBaseNetType, nReadLength);
					}
					else if (data_[1] == 0x03)
					{//�յ�RTCP������Ҫ�ظ�rtcp�����
						SendRtcpReportData(nVideoSSRC+1, data_[1]);
						//WriteLog(Log_Debug, "this =%X ,�յ� ��Ƶ ��RTCP������Ҫ�ظ�rtcp�������netBaseNetType = %d  �յ�RCP������ = %d ", this, netBaseNetType, nReadLength);
					}
				}
				else
				{
					WriteLog(Log_Debug, "ReadDataFunc() ,��δ��ȡ��rtp���� ! ABLRtspChan = %llu ", nClient);
					bExitProcessFlagArray[2] = true;
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return -1;
				}
			}
			else
			{//rtsp ����
				if (!ReadRtspEnd())
				{
					WriteLog(Log_Debug, "ReadDataFunc() ,��δ��ȡ��rtsp (1)���� ! nClient = %llu ", nClient);
					bExitProcessFlagArray[2] = true;
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
 					return  -1;
				}
				else
				{
					//���rtspͷ
					if (FindHttpHeadEndFlag() > 0)
						FillHttpHeadToStruct();

					if (nContentLength > 0)
					{
						nReadLength = nContentLength;

						//�����ContentLength ����Ҫ������ContentLength�������ٽ��ж�ȡ 
						if (netDataCacheLength < nContentLength)
						{//ʣ�µ����ݲ��� ContentLength ,��Ҫ�����ƶ��Ѿ���ȡ���ֽ��� data_Length  
							nNetStart -= data_Length;
							netDataCacheLength += data_Length;
							bExitProcessFlagArray[2] = true;
							WriteLog(Log_Debug, "ReadDataFunc (), RTSP �� Content-Length ��������δ��������  nClient = %llu", nClient);
							return 0;
						}

						nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
						if (nRet != 0 || nReadLength != nContentLength)
						{
							WriteLog(Log_Debug, "ReadDataFunc() ,��δ��ȡ��rtsp (2)���� ! nClient = %llu", nClient);
							bExitProcessFlagArray[2] = true;
							pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
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
			}
		}
		else
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp= %X  , ProcessNetData() ,��δ��ȡ������ ! , nClient = %llu", this, nClient);
			bExitProcessFlagArray[2] = true;
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		if (GetTickCount64() - tRtspProcessStartTime > 16000)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp= %X  , ProcessNetData() ,RTSP ���紦��ʱ ! , nClient = %llu", this, nClient);
			bExitProcessFlagArray[2] = true;
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}
	}

	bExitProcessFlagArray[2] = true;
	return 0;
}

//����rtcp �����
void  CNetClientSendRtsp::SendRtcpReportData(unsigned int nSSRC, int nChan)
{
	memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
	rtcpSR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

	ProcessRtcpData((char*)szRtcpSRBuffer, rtcpSRBufferLength, nChan);
}

//����rtcp ����� ���ն�
void  CNetClientSendRtsp::SendRtcpReportDataRR(unsigned int nSSRC, int nChan)
{
	memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
	rtcpRR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

	ProcessRtcpData((char*)szRtcpSRBuffer, rtcpSRBufferLength, nChan);
}

void  CNetClientSendRtsp::ProcessRtcpData(char* szRtcpData, int nDataLength, int nChan)
{
	std::lock_guard<std::mutex> lock(MediaSumRtpMutex);

	szRtcpDataOverTCP[0] = '$';
	szRtcpDataOverTCP[1] = nChan;
	unsigned short nRtpLen = htons(nDataLength);
	memcpy(szRtcpDataOverTCP + 2, (unsigned char*)&nRtpLen, sizeof(nRtpLen));

	memcpy(szRtcpDataOverTCP + 4, szRtcpData, nDataLength);
	XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 4, ABL_MediaServerPort.nSyncWritePacket);
}

//���͵�һ������
int CNetClientSendRtsp::SendFirstRequst()
{
	SendOptions(AuthenticateType);
 	return 0;
}

//����m3u8�ļ�
bool  CNetClientSendRtsp::RequestM3u8File()
{
	return true;
}

//�� SDP�л�ȡ  SPS��PPS ��Ϣ
bool  CNetClientSendRtsp::GetSPSPPSFromDescribeSDP()
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
		{//���滹�б����
			memcpy(m_szSPSPPSBuffer, szSprop_Parameter_Sets + nPos1 + strlen("sprop-parameter-sets="), nPos2 - nPos1 - strlen("sprop-parameter-sets="));
		}
		else
		{//����û�б����
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

void  CNetClientSendRtsp::UserPasswordBase64(char* szUserPwdBase64)
{
	char szTemp[string_length_2048] = { 0 };
	sprintf(szTemp, "%s:%s", m_rtspStruct.szUser, m_rtspStruct.szPwd);
	Base64Encode((unsigned char*)szUserPwdBase64, (unsigned char*)szTemp, strlen(szTemp));
}

//ȷ��SDP������Ƶ����Ƶ����ý������, ��Describe���ҵ� trackID���󻪵Ĵ�0��ʼ����������Ϊ�Ĵ�1��ʼ
void  CNetClientSendRtsp::FindVideoAudioInSDP()
{
	char szTemp[string_length_2048] = { 0 };

	nMediaCount = 0;
	if (strlen(szRtspContentSDP) <= 0)
		return;

	strcpy(szTemp, szRtspContentSDP);
#ifdef USE_BOOST
	to_lower(szTemp);
#else
	ABL::to_lower(szTemp);
#endif
	string strSDP = szTemp;
	string strTraceID;
	char   szTempTraceID[512] = { 0 };
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

				memcpy(szTempTraceID, szRtspContentSDP + nPos2 + 10, nPos3 - nPos2 - 10);
				strTraceID = szTempTraceID;
				nPos4 = strTraceID.rfind("/", strlen(szTempTraceID));
				if (nPos4 <= 0)
				{//û��rtsp���Ƶĵ�ַ������ trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//��rtsp���Ƶĵ�ַ�����纣��������ͷ a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
					memcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID + nPos4 + 1, strlen(szTempTraceID) - nPos4 - 1);
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

				memcpy(szTempTraceID, szRtspContentSDP + nPos2 + 10, nPos3 - nPos2 - 10);
				strTraceID = szTempTraceID;
				nPos4 = strTraceID.rfind("/", strlen(szTempTraceID));
				if (nPos4 <= 0)
				{//û��rtsp���Ƶĵ�ַ������ trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//��rtsp���Ƶĵ�ַ�����纣��������ͷ a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
					memcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID + nPos4 + 1, strlen(szTempTraceID) - nPos4 - 1);
				}
				nTrackIDOrer++;
			}
		}
	}
}

void  CNetClientSendRtsp::SendOptions(WWW_AuthenticateType wwwType)
{
	//ȷ������
 	nSendSetupCount = 0;
	nMediaCount = 0;
	nTrackIDOrer = 1;//��1��ʼ������0��ʼ
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
		szResponse = (char*)author.computeDigestResponse("OPTIONS", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "OPTIONS %s RTSP/1.0\r\nCSeq: %d\r\nAuthorization: Basic %s\r\nUser-Agent: ABL_RtspServer_3.0.1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szBasic);
	}

	unsigned int nRet = XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), ABL_MediaServerPort.nSyncWritePacket);
	if (nRet != 0)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
	nRtspProcessStep = RtspProcessStep_OPTIONS;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

void  CNetClientSendRtsp::SendANNOUNCE(WWW_AuthenticateType wwwType)
{
	if (wwwType == WWW_Authenticate_None)
	{
		sprintf(szResponseBuffer, "ANNOUNCE %s RTSP/1.0\r\nContent-Type: application/sdp\r\nCSeq: %d\r\nUser-Agent: Lavf57.83.100\r\nContent-Length: %d\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, strlen(szRtspSDPContent));
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("DESCRIBE", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);

	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Basic %s\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szBasic);
	}

	strcat(szResponseBuffer, szRtspSDPContent);
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), ABL_MediaServerPort.nSyncWritePacket);
	nRtspProcessStep = RtspProcessStep_ANNOUNCE;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

/*
Ҫ�Ż�����Щ����� {trackID=1 �� trackID=2} ����Щ������ǣ������ {trackID=0��trackID=1}
*/
void  CNetClientSendRtsp::SendSetup(WWW_AuthenticateType wwwType)
{
	nSendSetupCount++;
	if (wwwType == WWW_Authenticate_None)
	{
		if (nSendSetupCount == 1)
		{
			if (m_rtspStruct.szSrcRtspPullUrl[strlen(m_rtspStruct.szSrcRtspPullUrl) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\nSession: %s\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\nSession: %s\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_rtspStruct.szSrcRtspPullUrl[strlen(m_rtspStruct.szSrcRtspPullUrl) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession: %s\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szSessionID);
		}
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("SETUP", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

		if (nSendSetupCount == 1)
		{
			if (m_rtspStruct.szSrcRtspPullUrl[strlen(m_rtspStruct.szSrcRtspPullUrl) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_rtspStruct.szSrcRtspPullUrl[strlen(m_rtspStruct.szSrcRtspPullUrl) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);
		}

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);

		if (nSendSetupCount == 1)
		{
			if (m_rtspStruct.szSrcRtspPullUrl[strlen(m_rtspStruct.szSrcRtspPullUrl) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
		}
		else if (nSendSetupCount == 2)
		{
			if (m_rtspStruct.szSrcRtspPullUrl[strlen(m_rtspStruct.szSrcRtspPullUrl) - 1] == '/')
				sprintf(szResponseBuffer, "SETUP %s%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
			else
				sprintf(szResponseBuffer, "SETUP %s/%s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nAuthorization: Basic %s\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, szTrackIDArray[nSendSetupCount], CSeq, MediaServerVerson, szBasic);
		}
	}

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), ABL_MediaServerPort.nSyncWritePacket);

	nRtspProcessStep = RtspProcessStep_SETUP;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

void  CNetClientSendRtsp::SendRECORD(WWW_AuthenticateType wwwType)
{//\r\nScale: 255
	if (wwwType == WWW_Authenticate_None)
	{
 		sprintf(szResponseBuffer, "RECORD %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szSessionID);
	}
	else if (wwwType == WWW_Authenticate_MD5)
	{
		Authenticator author;
		char*         szResponse;

		author.setRealmAndNonce(m_rtspStruct.szRealm, m_rtspStruct.szNonce);
		author.setUsernameAndPassword(m_rtspStruct.szUser, m_rtspStruct.szPwd);
		szResponse = (char*)author.computeDigestResponse("PLAY", m_rtspStruct.szSrcRtspPullUrl); //Ҫע�� uri ,��ʱ��û������ б�� /

 		sprintf(szResponseBuffer, "RECORD %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);

 		sprintf(szResponseBuffer, "RECORD %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nAuthorization: Basic %s\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szSessionID, szBasic);
	}
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), ABL_MediaServerPort.nSyncWritePacket);

	nRtspProcessStep = RtspProcessStep_RECORD;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}
