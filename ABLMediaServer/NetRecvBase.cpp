/*
���ܣ�
   ������ա�������� �����������麯�� 
   1 ������������
      virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength) = 0;

   2 ִ�д��� 
      virtual int ProcessNetData() = 0;//�����������ݣ�������н���������������ݵȵ�

����    2021-03-29
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetRecvBase.h"
#ifdef USE_BOOST

extern CMediaFifo                            pDisconnectBaseNetFifo;             //������ѵ����� 
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);

#else

extern CMediaFifo                            pDisconnectBaseNetFifo;             //������ѵ����� 
extern MediaServerPort                       ABL_MediaServerPort;
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);


#endif //USE_BOOST

extern int                                   avpriv_mpeg4audio_sample_rates[];
extern int                                   SampleRateArray[];
unsigned char                                muteAACBuffer1[] = {0x00,0x00,0x00,0x00,0xff,0xf1,0x60,0x40,0x0a,0x7f,0xfc,0xde,0x04,0x00,0x00,0x6c,0x69,0x62,0x66,0x61,0x61,0x63,0x20,0x31,0x2e,0x33,0x30,0x00,0x00,0x02,0x75,0x3b,0x18,0x7e,0x2c,0xb1,0x25,0xbd,0xd9,0x62,0x84,0xb0,0x1d,0x96,0x43,0xe6,0xa1,0xdf,0xab,0x8d,0x4d,0x5e,0xbf,0x30,0x0a,0xff,0xfb,0x90,0x0b,0xbc,0x7a,0x0f,0x2a,0x2a,0xaa,0x56,0x3c,0x5b,0xcd,0xe1,0xff,0x2b,0xe3,0xe3,0xe3,0xe3,0xe3,0xe3,0xe3,0x85,0x30,0x70,0xa6,0x0e,0x14,0xc1,0xc0 };
unsigned char                                muteAACBuffer2[] = {0x00,0x00,0x00,0x00,0xff,0xf1,0x60,0x40,0x06,0xbf,0xfc,0x01,0x4c,0xd4,0xac,0x34,0x14,0x11,0x0b,0x7e,0x45,0xda,0x1e,0x2d,0x4b,0xa9,0x35,0x2a,0x69,0x52,0x42,0xd1,0x22,0x49,0x24,0x96,0x09,0x65,0x0e,0x48,0xe2,0xf9,0xae,0x2e,0xe8,0x59,0x52,0xa2,0x86,0xff,0xc5,0xf8,0xab,0x2f,0xfc,0x7c,0x0e };
unsigned char                                muteAACBuffer3[] = {0x00,0x00,0x00,0x00,0xff,0xf1,0x60,0x40,0x02,0x7f,0xfc,0x01,0x38,0x14,0xac,0x21,0xf4,0x87,0x0b,0xe2,0x30,0x00,0xe0 };
unsigned char                                muteAACBuffer4[] = {0x00,0x00,0x00,0x00,0xff,0xf1,0x60,0x40,0x02,0x7f,0xfc,0x01,0x2c,0x14,0xac,0x21,0xf4,0x87,0x0b,0xf1,0xec,0x00,0x38 };
extern  uint8_t                              NALU_START_CODE[3] ;
extern  uint8_t                              SLICE_START_CODE[4]  ;

CNetRevcBase::CNetRevcBase()
{
	nWebRTC_Comm_State = -1 ;
	readerCount = 0;
	memset(szExtendMediaData, 0x00, sizeof(szExtendMediaData));
	bUpdateFlag = false ;
	nOldFileSize = 0 ;
	memset(szZeroMediaData, 0x00, sizeof(szZeroMediaData));
	addThreadPoolFlag = false;
	nWriteRecordCacheFFLushLength = 0;
	bCreateNewRecordFile = false;
	nRecordDateTime = GetTickCount64();
	nWriteRecordByteSize = 0;
	memset(szCurrentDateTime, 0x00, sizeof(szCurrentDateTime));
	memset(szStartDateTime, 0x00, sizeof(szStartDateTime));
	nMediaClient = nMediaClient2 = 0;
	m_bSendCacheAudioFlag = false;
	nSpeedCount[0] = nSpeedCount[1] = 0;
	m_nVideoFrameSpeed = 3;
	m_RtspNetworkType = RtspNetworkType_Unknow;
 	bAddMuteFlag = false;
	nAddMuteAACBufferOrder = 0;
	memcpy(muteAACBuffer[0].pAACBuffer, muteAACBuffer1, sizeof(muteAACBuffer1));
	muteAACBuffer[0].nAACLength = sizeof(muteAACBuffer1);
	memcpy(muteAACBuffer[1].pAACBuffer, muteAACBuffer2, sizeof(muteAACBuffer2));
	muteAACBuffer[1].nAACLength = sizeof(muteAACBuffer2);
	memcpy(muteAACBuffer[2].pAACBuffer, muteAACBuffer3, sizeof(muteAACBuffer3));
	muteAACBuffer[2].nAACLength = sizeof(muteAACBuffer3);
	memcpy(muteAACBuffer[3].pAACBuffer, muteAACBuffer4, sizeof(muteAACBuffer4));
	muteAACBuffer[3].nAACLength = sizeof(muteAACBuffer4);

	m_nSpsPPSLength = 0;
	m_bHaveSPSPPSFlag = false;
	nReplayClient = 0;
	m_nXHRtspURLType = 0;
	bProxySuccessFlag = false;
	m_bWaitIFrameCount = 0;
	memset(szPlayParams, 0x00, sizeof(szPlayParams));
	bOn_playFlag = false;
	m_rtspPlayerType = RtspPlayerType_Liveing;
	nCurrentVideoFrames = 0;//��ǰ��Ƶ֡��
	nTotalVideoFrames = 0;//¼����Ƶ��֡��
	nTcp_Switch = 0;
	bSendFirstIDRFrameFlag = false;
	bRunFlag.exchange(true);
	nSSRC = 0;
	m_bSendMediaWaitForIFrame = false;
	m_bIsRtspRecordURL = false;
	m_bPauseFlag = false;
	m_nScale = 0;
	netBaseNetType = NetBaseNetType_Unknown;
	bPushMediaSuccessFlag = false;
	bProxySuccessFlag = false;
	memset(app, 0x00, sizeof(app));
	memset(stream, 0x00, sizeof(stream));

	memset(szClientIP,0x00,sizeof(szClientIP)); //���������Ŀͻ���IP 
	nClientPort = 0 ; //���������Ŀͻ��˶˿� 
	nRecvDataTimerBySecond = 0;

	szVideoFrameHead[0] = 0x00;
	szVideoFrameHead[1] = 0x00;
	szVideoFrameHead[2] = 0x00;
	szVideoFrameHead[3] = 0x01;
	bPushSPSPPSFrameFlag = false;

	psHeadFlag[0] = 0x00;
	psHeadFlag[1] = 0x00;
	psHeadFlag[2] = 0x01;
	psHeadFlag[3] = 0xBA;

	nVideoStampAdd = 40  ;
	nAsyncAudioStamp = GetTickCount64() ;
	nCreateDateTime = nProxyDisconnectTime  = GetTickCount64();
	bRecordProxyDisconnectTimeFlag = false;

	videoDts = audioDts = 0;
	bUserNewAudioTimeStamp = false;
	hParent = 0;
	nPrintTime = GetTickCount64();

	nVideoFrameSpeedOrder = 0;
	oldVideoTimestamp = 0;
	bUpdateVideoFrameSpeedFlag = false;
	memset(szMediaSourceURL, 0x00, sizeof(szMediaSourceURL));
	bResponseHttpFlag = false;
	nGB28181ConnectCount = 0; 
	nReConnectingCount = 0 ;

	memset(szRecordPath, 0x00, sizeof(szRecordPath));
	nReplayClient = 0;

	nCalcVideoFrameCount = 0; //�������
	for(int i= 0;i<CalcMaxVideoFrameSpeed ;i++)
	 nVideoFrameSpeedArray[i] = 0;//��Ƶ֡�ٶ�����

	nMediaSourceType = MediaSourceType_LiveMedia;//Ĭ��ʵ������
	duration = 0 ;
	nClientRtcp = 0;
	nRtspRtpPayloadType = RtspRtpPayloadType_Unknow ;  //δ֪
	bConnectSuccessFlag = false;
	bSnapSuccessFlag = false;
	timeout_sec = 10;
	memset(domainName,0x00,sizeof(domainName)); //����
    ifConvertFlag = false;//�Ƿ���Ҫת��
	tUpdateIPTime = nStartProcessJtt1078Time = GetTickCount64();
}

CNetRevcBase::~CNetRevcBase()
{
	//�رջطŵ�ID���ͻ��Ƿ�ý��Դ
	if (nReplayClient > 0)
		pDisconnectBaseNetFifo.push((unsigned char*)&nReplayClient, sizeof(nReplayClient));
}

//������ת��ΪIP��ַ
bool   CNetRevcBase::ConvertDemainToIPAddress()
{
	if (!ifConvertFlag)
		return true;

	hostent* host = gethostbyname(domainName);
	if (host == NULL)
		return false;

	char getIP[128] = { 0 };
	for (int i = 0; host->h_addr_list[i]; i++)
	{
		memset(getIP, 0x00, sizeof(getIP));
		strcpy(getIP,inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
		if (strlen(getIP) > 0)
		{
			strcpy(m_rtspStruct.szIP, getIP);
			WriteLog(Log_Debug, "CNetRevcBase = %X ,nClient = %llu ��domainName = %s ,ת��IPΪ %s ", this, nClient, domainName, m_rtspStruct.szIP);
			return true;
		}
	}

	return false ;
}

//����rtsp\rtmp\http��ز�����IP���˿ڣ��û�������
bool  CNetRevcBase::ParseRtspRtmpHttpURL(char* szURL)
{//rtsp://admin:szga2019@190.15.240.189:554
	int nPos1, nPos2, nPos3, nPos4, nPos5;
	string strRtspURL = szURL;
	char   szIPPort[128] = { 0 };
	string strIPPort;
	char   szSrcRtspPullUrl[string_length_2048] = { 0 };

	//ȫ��תΪСд
	strcpy(szSrcRtspPullUrl, szURL);


#ifdef USE_BOOST
	to_lower(szSrcRtspPullUrl);
#else
	ABL::to_lower(szSrcRtspPullUrl);
#endif

	if ( !(memcmp(szSrcRtspPullUrl, "rtsp://", 7) == 0 || memcmp(szSrcRtspPullUrl, "rtmp://", 7) == 0 || memcmp(szSrcRtspPullUrl, "http://", 7) == 0 || 
		   memcmp(szSrcRtspPullUrl, "rtsps://", 8) == 0 || memcmp(szSrcRtspPullUrl, "rtmps://", 8) == 0 || memcmp(szSrcRtspPullUrl, "https://",8) == 0 ))
		return false;

	memset((char*)&m_rtspStruct, 0x00, sizeof(m_rtspStruct));
	strcpy(m_rtspStruct.szSrcRtspPullUrl, szURL);

	//���� @ ��λ��
	nPos2 = strRtspURL.rfind("@", strlen(szURL));
	if (nPos2 > 0)
	{
		m_rtspStruct.bHavePassword = true;

		nPos1 = strRtspURL.find("//", 0);
		if (nPos1 > 0)
		{
			nPos3 = strRtspURL.find(":", nPos1 + 1);
			if (nPos3 > 0)
			{
				memcpy(m_rtspStruct.szUser, m_rtspStruct.szSrcRtspPullUrl + nPos1 + 2, nPos3 - nPos1 - 2);
				memcpy(m_rtspStruct.szPwd, m_rtspStruct.szSrcRtspPullUrl + nPos3 + 1, nPos2 - nPos3 - 1);

				//���� / ,�����IP���˿�
				nPos4 = strRtspURL.find("/", nPos2 + 1);
				if (nPos4 > 0)
				{
					memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos2 + 1, nPos4 - nPos2 - 1);
				}
				else
				{
					memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos2 + 1, strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos2);
				}

				strIPPort = szIPPort;
				nPos5 = strIPPort.find(":", 0);
				if (nPos5 > 0)
				{//��ָ���˿�
					memcpy(m_rtspStruct.szIP, szIPPort, nPos5);
					memcpy(m_rtspStruct.szPort, szIPPort + nPos5 + 1, strlen(szIPPort) - nPos5 - 1);
				}
				else
				{//û��ָ���˿�
					strcpy(m_rtspStruct.szIP, szIPPort);
					if (memcmp(szSrcRtspPullUrl, "rtsp://",7) == 0)
					   strcpy(m_rtspStruct.szPort, "554");
					else  if (memcmp(szSrcRtspPullUrl, "rtmp://", 7) == 0)
						strcpy(m_rtspStruct.szPort, "1935");
					else  if (memcmp(szSrcRtspPullUrl, "http://", 7) == 0)
						strcpy(m_rtspStruct.szPort, "80");
				}
			}
		}

		//�ظ���ʱ��ȥ���û�������
		memset(szSrcRtspPullUrl, 0x00, sizeof(szSrcRtspPullUrl));
		strcpy(szSrcRtspPullUrl, "rtsp://");
		memcpy(szSrcRtspPullUrl + 7, m_rtspStruct.szSrcRtspPullUrl + (nPos2 + 1), strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos2 - 1);

		memset(m_rtspStruct.szSrcRtspPullUrl, 0x00, sizeof(m_rtspStruct.szSrcRtspPullUrl));
		strcpy(m_rtspStruct.szSrcRtspPullUrl, szSrcRtspPullUrl);
	}
	else
	{
		m_rtspStruct.bHavePassword = false;

		nPos1 = strRtspURL.find("//", 0);
		if (nPos1 > 0)
		{
			nPos2 = strRtspURL.find("/", nPos1 + 2);

			//���� / ,�����IP���˿�
			if (nPos2 > 0)
			{
				memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos1 + 2, nPos2 - nPos1 - 2);
			}
			else
			{
				memcpy(szIPPort, m_rtspStruct.szSrcRtspPullUrl + nPos1 + 2, strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos1 - 2);
			}

			strIPPort = szIPPort;
			nPos5 = strIPPort.find(":", 0);
			if (nPos5 > 0)
			{//��ָ���˿�
				memcpy(m_rtspStruct.szIP, szIPPort, nPos5);
				memcpy(m_rtspStruct.szPort, szIPPort + nPos5 + 1, strlen(szIPPort) - nPos5 - 1);
			}
			else
			{//û��ָ���˿�
				strcpy(m_rtspStruct.szIP, szIPPort);
				if (memcmp(szSrcRtspPullUrl, "rtsp://", 6) == 0)
					strcpy(m_rtspStruct.szPort, "554");
				else  if (memcmp(szSrcRtspPullUrl, "rtmp://", 6) == 0)
					strcpy(m_rtspStruct.szPort, "1935");
				else  if (memcmp(szSrcRtspPullUrl, "http://", 6) == 0)
					strcpy(m_rtspStruct.szPort, "80");
			}
		}
	}

	nPos1 = strRtspURL.find("://", 0);
	if (nPos1 > 0)
	{
		nPos2 = strRtspURL.find("/", nPos1 + 4);
		if (nPos2 > 0)
		{
			memcpy(m_rtspStruct.szRequestFile, szURL + nPos2 , strlen(szURL) - nPos2 - 1);
		}
	}	

	if (strlen(m_rtspStruct.szIP) == 0 || strlen(m_rtspStruct.szPort) == 0)
	{
		return false;
	}
	else
	{
		//�����������ж��Ƿ���Ҫת��ΪIP
		strcpy(domainName, m_rtspStruct.szIP);
		string strDomainName = m_rtspStruct.szIP;
#ifdef USE_BOOST
		replace_all(strDomainName, ".", "");
		if (!boost::all(strDomainName, boost::is_digit()))
#else
		ABL::replace_all(strDomainName, ".", "");
		if (!ABL::is_digits(strDomainName))
#endif
		{//�������֣���Ҫ����ת��ΪIP
			ifConvertFlag = true;

			if (!ConvertDemainToIPAddress())
			{
				WriteLog(Log_Debug, "CNetRevcBase = %X ,nClient = %llu ��domainName = %s ,����תΪIP ʧ�� ", this,nClient,domainName);
				return false;
			}
		}

		char   szRtspURLTrim[2048] = { 0 };
		nPos5 = strRtspURL.find("?", 0);
 		if (nPos5 > 0)
			memcpy(szRtspURLTrim, szURL, nPos5);
		else
			strcpy(szRtspURLTrim, szURL);
		strRtspURL = szRtspURLTrim;
 		nPos5 = strRtspURL.rfind("@", strlen(szURL));
		if (nPos5 > 0)
		{
			strcpy(m_rtspStruct.szRtspURLTrim, "rtsp://");
			memcpy(m_rtspStruct.szRtspURLTrim + 7, szRtspURLTrim + nPos5+1, strlen(szRtspURLTrim) - nPos5);
		}else 
			strcpy(m_rtspStruct.szRtspURLTrim, szRtspURLTrim);

		return true;
	}
}

/*
�����Ƶ�Ƿ���I֡
*/
bool  CNetRevcBase::CheckVideoIsIFrame(char* szVideoName,unsigned char* szPVideoData, int nPVideoLength)
{
	int nPos = 0;
	bool bVideoIsIFrameFlag = false;
	unsigned char  nFrameType = 0x00;
	int nNaluType = 1;
	int nAddStep = 3;

	for (int i = 0; i< nPVideoLength; i++)
	{
		nNaluType = -1;
		if (memcmp(szPVideoData + i,(unsigned char*) NALU_START_CODE, sizeof(NALU_START_CODE)) == 0)
		{//�г��� 00 00 01 ��naluͷ��־
			nNaluType = 1;
			nAddStep = sizeof(NALU_START_CODE);
		}
		else if (memcmp(szPVideoData + i, (unsigned char*)SLICE_START_CODE, sizeof(SLICE_START_CODE)) == 0)
		{//�г��� 00 00 00 01 ��naluͷ��־
			nNaluType = 2;
			nAddStep = sizeof(SLICE_START_CODE);
		}

		if (nNaluType >= 1)
		{//�ҵ�֡Ƭ��
			if (strcmp(szVideoName, "H264") == 0)
			{
				nFrameType = (szPVideoData[i + nAddStep] & 0x1F);
				if (nFrameType == 7 || nFrameType == 8 || nFrameType == 5)
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
			}
			else if (strcmp(szVideoName, "H265") == 0)
			{
				nFrameType = (szPVideoData[i + nAddStep] & 0x7E) >> 1;
				if ((nFrameType >= 16 && nFrameType <= 21) || (nFrameType >= 32 && nFrameType <= 34))
				{//SPS   PPS   IDR 
					bVideoIsIFrameFlag = true;
					break;
				}
			}

			//ƫ��λ�� 
			i += nAddStep;   
		}

		//����Ҫȫ�������ϣ��Ϳ����ж�һ֡����
		if (i >= 256)
			return false;
 	}

	return bVideoIsIFrameFlag;
}

void  CNetRevcBase::SyncVideoAudioTimestamp()
{
	//500����ͬ��һ�� 
	if (GetTickCount() - nAsyncAudioStamp >= 500 )
	{
		if (videoDts / 1000 > audioDts / 1000 )
		{
			nVideoStampAdd = (1000 / mediaCodecInfo.nVideoFrameRate ) - 10  ;
		}
		else if (videoDts / 1000  < audioDts / 1000 )
		{
			nVideoStampAdd = (1000 / mediaCodecInfo.nVideoFrameRate) + 10 ;
		}
		else
			nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

		nAsyncAudioStamp = GetTickCount();

		//WriteLog(Log_Debug, "CNetRevcBase = %X ,nClient = %llu videoDts = %d ,audioDts = %d ", this,nClient, videoDts / 1000, audioDts / 1000 );
	}
}

//������Ƶ֡�ٶ�
int  CNetRevcBase::CalcVideoFrameSpeed(unsigned char* pRtpData, int nLength)
{
	if (pRtpData == NULL)
		return -1 ;
	
	int nVideoFrameSpeed = 25 ;
	rtp_header = (_rtp_header*)pRtpData ;
	if (oldVideoTimestamp == 0)
	{
		oldVideoTimestamp = ntohl(rtp_header->timestamp);
	}
	else
	{
		if (ntohl(rtp_header->timestamp) != oldVideoTimestamp && ntohl(rtp_header->timestamp) > oldVideoTimestamp)
		{
			//WriteLog(Log_Debug, "this = %X ,nVideoFrameSpeed = %llu ", this,(90000 / (ntohl(rtp_header->timestamp) - oldVideoTimestamp)) );
 			nVideoFrameSpeed = 90000 / (ntohl(rtp_header->timestamp) - oldVideoTimestamp);
			if (nVideoFrameSpeed > 120 )
				nVideoFrameSpeed = 120 ;

			oldVideoTimestamp = ntohl(rtp_header->timestamp);

			nVideoFrameSpeedOrder++;
			//WriteLog(Log_Debug, "this = %X ,nVideoFrameSpeed = %llu ", this, nVideoFrameSpeed );
			if (nVideoFrameSpeedOrder < 10)
				return -1;
			else
			{
 				nCalcVideoFrameCount++; //�������
				if (nVideoFrameSpeed == 6)
					nSpeedCount[0] ++;
				else if (nVideoFrameSpeed == 7)
					nSpeedCount[1] ++;

				if (nVideoFrameSpeed > m_nVideoFrameSpeed)
					m_nVideoFrameSpeed = nVideoFrameSpeed;
				 
				if (nCalcVideoFrameCount >= CalcMaxVideoFrameSpeed)
				{
					if (m_nVideoFrameSpeed == 6 )
						m_nVideoFrameSpeed = 25;
					else if (m_nVideoFrameSpeed == 7)
						m_nVideoFrameSpeed = 30;
					else if (m_nVideoFrameSpeed > 120)
						m_nVideoFrameSpeed = 120;

					if(nSpeedCount[0] > 10 )
						m_nVideoFrameSpeed = 25; 
					if (nSpeedCount[1] > 10)
						m_nVideoFrameSpeed = 30;

 					return m_nVideoFrameSpeed;
				}
				else
					return -1;
			}
		}
	}
	return -1;
}

//����flv����Ƶ֡�ٶ�
int   CNetRevcBase::CalcFlvVideoFrameSpeed(int nVideoPTS, int nMaxValue)
{
	int nVideoFrameSpeed = 25;
	if (oldVideoTimestamp == 0)
	{
		oldVideoTimestamp = nVideoPTS;
	}
	else
	{
		if (nVideoPTS != oldVideoTimestamp && nVideoPTS > oldVideoTimestamp)
		{
			nVideoFrameSpeed = nMaxValue / (nVideoPTS  - oldVideoTimestamp);
			if (nVideoFrameSpeed > 120)
				nVideoFrameSpeed = 120 ;

			oldVideoTimestamp = nVideoPTS;
			nVideoFrameSpeedOrder ++;
			//WriteLog(Log_Debug, "this = %X ,nVideoFrameSpeed = %llu ", this, nVideoFrameSpeed );
			if (nVideoFrameSpeedOrder < 10)
				return -1;
			else
			{
				nCalcVideoFrameCount ++ ; //�������
				if (nVideoFrameSpeed == 6)
					nSpeedCount[0] ++;
				else if (nVideoFrameSpeed == 7)
					nSpeedCount[1] ++;

 				if (nVideoFrameSpeed > m_nVideoFrameSpeed)
					m_nVideoFrameSpeed = nVideoFrameSpeed;
 
				if (nCalcVideoFrameCount >= CalcMaxVideoFrameSpeed)
				{
					if (m_nVideoFrameSpeed == 6 )
						m_nVideoFrameSpeed = 25;
					else if (m_nVideoFrameSpeed == 7)
						m_nVideoFrameSpeed = 30;
					else if(m_nVideoFrameSpeed > 120)
						m_nVideoFrameSpeed = 120;

					if (nSpeedCount[0] > 10)
						m_nVideoFrameSpeed = 25;
					if (nSpeedCount[1] > 10)
						m_nVideoFrameSpeed = 30;

 					return m_nVideoFrameSpeed;
				}
				else
					return -1;
			}
		}
		return -1;
	}
	return -1;
}

//�и�app ,stream 
bool  CNetRevcBase::SplitterAppStream(char* szMediaSoureFile)
{
	if (szMediaSoureFile == NULL || szMediaSoureFile[0] != '/' )
		return false;
	string strMediaSource = szMediaSoureFile;
	int  nPos2;

	nPos2 = strMediaSource.find("/", 1);
	if (nPos2 < 0)
		return false;

	strcpy(szMediaSourceURL, szMediaSoureFile);
	memset(m_addStreamProxyStruct.app, 0x00, sizeof(m_addStreamProxyStruct.app));
	memset(m_addStreamProxyStruct.stream, 0x00, sizeof(m_addStreamProxyStruct.stream));

	memcpy(m_addStreamProxyStruct.app, szMediaSoureFile+1, nPos2 - 1 );
	memcpy(m_addStreamProxyStruct.stream, szMediaSoureFile + nPos2 +1 ,strlen(szMediaSoureFile) - nPos2 - 1);

	strcpy(app, m_addStreamProxyStruct.app);
	strcpy(stream, m_addStreamProxyStruct.stream);

	return true;
}

//�ظ��ɹ���Ϣ
bool  CNetRevcBase::ResponseHttp(uint64_t nHttpClient,char* szSuccessInfo,bool bClose)
{
	if (szSuccessInfo == NULL)
		return false;

	//����ʵ�����Ѿ��ظ�
	bResponseHttpFlag = true;

	auto  pClient = GetNetRevcBaseClientNoLock(nHttpClient);
	if (pClient == NULL)
 		return true;
	if (pClient->bResponseHttpFlag)
		return true;

	//�ظ�http����
	string strReponseError = szSuccessInfo ;
#ifdef USE_BOOST
	replace_all(strReponseError, "\r\n", " ");
#else
	ABL::replace_all(strReponseError, "\r\n", " ");
#endif
	strcpy(szSuccessInfo, strReponseError.c_str());

	//���request_uuid 
	InsertUUIDtoJson(szSuccessInfo, pClient->request_uuid);

	int nLength = strlen(szSuccessInfo);
	if(bClose == true)
	  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nLength);
	else
	  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nLength);
 
	XHNetSDK_Write(nHttpClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), ABL_MediaServerPort.nSyncWritePacket);
	XHNetSDK_Write(nHttpClient, (unsigned char*)szSuccessInfo, nLength, ABL_MediaServerPort.nSyncWritePacket);

	pClient->bResponseHttpFlag = true;

	WriteLog(Log_Debug, szSuccessInfo);
	return true;
}

//�ظ��ɹ���Ϣ
bool  CNetRevcBase::ResponseHttp2(uint64_t nHttpClient, char* szSuccessInfo, bool bClose)
{
	if (bResponseHttpFlag)
		return false;

	auto  pClient = GetNetRevcBaseClientNoLock(nHttpClient);
	if (pClient == NULL)
		return true;

	//����ʵ�����Ѿ��ظ�
	bResponseHttpFlag = true;

	//�ظ�http����
	string strReponseError = szSuccessInfo;
#ifdef USE_BOOST
	replace_all(strReponseError, "\r\n", " ");
#else
	ABL::replace_all(strReponseError, "\r\n", " ");
#endif

	strcpy(szSuccessInfo, strReponseError.c_str());

	//���request_uuid 
	InsertUUIDtoJson(szSuccessInfo, pClient->request_uuid);

	int nLength = strlen(szSuccessInfo);
	if (bClose == true)
		sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nLength);
	else
		sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nLength);

	XHNetSDK_Write(nHttpClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), ABL_MediaServerPort.nSyncWritePacket);
	XHNetSDK_Write(nHttpClient, (unsigned char*)szSuccessInfo, nLength, ABL_MediaServerPort.nSyncWritePacket);

	WriteLog(Log_Debug, szSuccessInfo);
	return true;
}

//�ظ�ͼƬ
bool  CNetRevcBase::ResponseImage(uint64_t nHttpClient, HttpImageType imageType,unsigned char* pImageBuffer, int nImageLength, bool bClose)
{
	std::lock_guard<std::mutex> lock(httpResponseLock);

 	if (bClose == true)
	{
		if(imageType == HttpImageType_jpeg)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: image/jpeg;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nImageLength);
		else if (imageType == HttpImageType_png)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: image/png;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nImageLength);
	}
	else
	{
		if (imageType == HttpImageType_jpeg)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: image/jpeg;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nImageLength);
		else if (imageType == HttpImageType_png)
		  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: image/png;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, "keep-alive", nImageLength);
	}

	XHNetSDK_Write(nHttpClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), ABL_MediaServerPort.nSyncWritePacket);

	int nPos = 0;
	int nWriteRet ;
	while (nImageLength > 0 && pImageBuffer != NULL)
	{
		if (nImageLength > Send_ImageFile_MaxPacketCount)
		{
			nWriteRet = XHNetSDK_Write(nHttpClient, (unsigned char*)pImageBuffer + nPos, Send_ImageFile_MaxPacketCount, ABL_MediaServerPort.nSyncWritePacket);
			nImageLength -= Send_ImageFile_MaxPacketCount;
			nPos += Send_ImageFile_MaxPacketCount;
		}
		else
		{
			nWriteRet = XHNetSDK_Write(nHttpClient, (unsigned char*)pImageBuffer + nPos, nImageLength, ABL_MediaServerPort.nSyncWritePacket);
			nPos += nImageLength;
			nImageLength = 0;
		}

		if (nWriteRet != 0)
		{//���ͳ���
  			WriteLog(Log_Debug, "CNetRevcBase = %X nHttpClient = %llu  ����ͼƬ����׼��ɾ�� ", this, nHttpClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nHttpClient, sizeof(nHttpClient));
 			return false  ;
 		}
	}
 
 	return true;
}

//url���� 
bool CNetRevcBase::DecodeUrl(char *Src, char  *url, int  MaxLen)  
{  
    if(NULL == url || NULL == Src || strlen(Src) == 0)  
    {  
        return false;  
    }  
    if(MaxLen == 0)  
    {  
        return false;  
    }  

    char  *p = Src;  // ����ѭ��  
    int    i = 0;    // i��������url����  

    /* ��ʱ����url���������
       ����: %1A%2B%3C
    */  
    char  t = '\0';  
    while(*p != '\0' && MaxLen--)  
    {  
        if(*p == 0x25) // 0x25 = '%'  
        {  
            /* ������ʮ���������г����ֵĴ�д��ĸ,Сд��ĸ,���ֵ��ж� */  
            if(p[1] >= 'A' && p[1] <= 'Z') // ��д��ĸ  
            {  
                t = p[1] - 'A' + 10;  // A = 10,��ͬ  
            }  
            else if(p[1] >= 'a' && p[1] <= 'z') // Сд��ĸ  
            {  
                t = p[1] - 'a' + 10;  
            }  
            else if(p[1] >= '0' && p[1] <= '9') // ����  
            {  
                t = p[1] - '0';  
            }  

            t *= 16;  // �����ŵ�ʮλ��ȥ  

            if(p[2] >= 'A' && p[2] <= 'Z') // ��д��ĸ  
            {  
                t += p[2] - 'A' + 10;  
            }  
            else if(p[2] >= 'a' && p[2] <= 'z') // Сд��ĸ  
            {  
                t += p[2] - 'a' + 10;  
            }  
            else if(p[2] >= '0' && p[2] <= '9') // ����  
            {  
                t += p[2] - '0';  
            }  

            // ���˺ϳ���һ��ʮ��������  
            url[i] = t;  
            p += 3, i++;  
        }  
        else  
        {  
            // û�б�url���������  
            // '+'���⴦��.���൱��һ���ո�  
            if(*p != '+')  
            {  
                url[i] = *p;  
            }  
            else  
            {  
                url[i] = *p;//+�ţ�����ԭ�����ַ��������Ҫ�޸�Ϊ �ո񣬷���Ϊ��url���ƻ� 
            }  
            i++;  
            p++;  
        }  
    }  
    url[i] = '\0';  // ������  
    return true;  
}  

//����¼��㲥��url��ѯ¼���ļ��Ƿ���� 
bool   CNetRevcBase::QueryRecordFileIsExiting(char* szReplayRecordFileURL)
{
	if (strlen(szMediaSourceURL) <= 0)
		return false;

	memset(szSplliterShareURL, 0x00, sizeof(szSplliterShareURL));//¼��㲥ʱ�и��url 
	memset(szReplayRecordFile, 0x00, sizeof(szReplayRecordFile));//¼��㲥�и��¼���ļ����� 
	memset(szSplliterApp, 0x00, sizeof(szSplliterApp));
	memset(szSplliterStream, 0x00, sizeof(szSplliterStream));
 	string strRequestMediaSourceURL = szReplayRecordFileURL;
	int   nPos = strRequestMediaSourceURL.find(RecordFileReplaySplitter, 0);
	if (nPos <= 0)
  		return false ;

 	memcpy(szSplliterShareURL, szMediaSourceURL, nPos);
	memcpy(szReplayRecordFile, szMediaSourceURL + (nPos + strlen(RecordFileReplaySplitter)), strlen(szMediaSourceURL) - nPos - strlen(RecordFileReplaySplitter));

	if (QureyRecordFileFromRecordSource(szSplliterShareURL, szReplayRecordFile) == false)
 		return false ;

	int   nPos2 = strRequestMediaSourceURL.find("/", 2);
	if (nPos2 > 0)
	{
		memcpy(szSplliterApp, szReplayRecordFileURL + 1, nPos2 -1 );
		memcpy(szSplliterStream, szReplayRecordFileURL + nPos2 + 1, nPos - nPos2 -1 );
	}

	return true;
}

//����¼���ļ������㲥��¼��ý��Դ
#ifdef USE_BOOST
boost::shared_ptr<CMediaStreamSource>   CNetRevcBase::CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient)
#else
std::shared_ptr<CMediaStreamSource>   CNetRevcBase::CreateReplayClient(char* szReplayURL, uint64_t* nReturnReplayClient)
#endif
{
#ifdef OS_System_Windows
	sprintf(szRequestReplayRecordFile, "%s%s\\%s\\%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#else
	sprintf(szRequestReplayRecordFile, "%s%s/%s/%s.mp4", ABL_MediaServerPort.recordPath, szSplliterApp, szSplliterStream, szReplayRecordFile);
#endif

	auto pTempSource = GetMediaStreamSource(szReplayURL);
	if (pTempSource == NULL)
	{
		auto replayClient = CreateNetRevcBaseClient(ReadRecordFileInput_ReadFMP4File, 0, 0, szRequestReplayRecordFile, 0, szSplliterShareURL);
		if (replayClient)//��¼¼��㲥��client 
		 *nReturnReplayClient = replayClient->nClient;

		int nWaitCount = 0;
 		while (true)
		{//�ȴ�¼���ļ�������ý��Դ
		   nWaitCount++;
		   //Sleep(200);
		   std::this_thread::sleep_for(std::chrono::milliseconds(200));
		   pTempSource = GetMediaStreamSource(szReplayURL);
		   if (pTempSource != NULL)
			   break;
		   if (nWaitCount >= 30)
			   break;
  		}
		if (pTempSource == NULL)
		{
			if (replayClient)
				pDisconnectBaseNetFifo.push((unsigned char*)&replayClient->nClient, sizeof(replayClient->nClient));
			return NULL;
		}

	    replayClient->hParent = nClient ;
	}
	nMediaSourceType = MediaSourceType_ReplayMedia;
	duration = pTempSource->nMediaDuration;

	return  pTempSource;
}

//����ͨ����������Ƶ�ʻ�ȡ�� sdp �� config 
char*   CNetRevcBase::getAACConfig(int nChanels, int nSampleRate)
{
	int  profile = 1;
	int  samplingFrequencyIndex = 8;
	int  channelConfiguration = nChanels;

	for (int i = 0; i < 13; i++)
	{
		if (avpriv_mpeg4audio_sample_rates[i] == nSampleRate)
		{
			samplingFrequencyIndex = i;
			break;
		}
	}

	unsigned char audioSpecificConfig[2];
	uint8_t  audioObjectType = profile + 1;

	audioSpecificConfig[0] = (audioObjectType << 3) | (samplingFrequencyIndex >> 1);
	audioSpecificConfig[1] = (samplingFrequencyIndex << 7) | (channelConfiguration << 3);
	sprintf(szConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

	return (char*) szConfigStr;
}

//request_uui �� key ֵ ���� �ظ���json 
bool CNetRevcBase::InsertUUIDtoJson(char* szSrcJSON,char* szUUID)
{
	int  nLength2 = strlen(szUUID);
	if (nLength2 > 0 && strlen(szSrcJSON) > 0)
	{
		if (szSrcJSON[0] == '{' && szSrcJSON[strlen(szSrcJSON) - 1] == '}')
		{
			string strJsonSrc = szSrcJSON;
			int    nPos = strJsonSrc.find(",", 0);
			int    nLength3 = 0;
			if (nPos > 0)
			{
				sprintf(szTemp2, "\"request_uuid\":\"%s\",", szUUID);
				nLength3 = strlen(szTemp2);

				//����ԭ����json�ַ�����
				memmove(szSrcJSON + nPos + nLength3, szSrcJSON + nPos, (strlen(szSrcJSON) - nPos) + strlen(szTemp2));
				memcpy(szSrcJSON + nPos + 1, szTemp2, nLength3);

				return true;
			}
		}
		else
			return false;
	}
	else
		return false;
}

//����AAC��Ƶ���ݻ�ȡAACý����Ϣ,ԭ�������Ѿ�������Ƶ,������Ҫ����һ���������»�ȡ����һ�����������Ƶ 
void CNetRevcBase::GetAACAudioInfo2(unsigned char* nAudioData, int nLength, int* nSampleRate, int* nChans)
{
 		unsigned char nSampleIndex = 1;
		unsigned char  nChannels = 1;

		nSampleIndex = ((nAudioData[2] & 0x3c) >> 2) & 0x0F;  //�� szAudio[2] �л�ȡ����Ƶ�ʵ����
		if (nSampleIndex >= 15)
			nSampleIndex = 8;
		*nSampleRate = SampleRateArray[nSampleIndex];

		//ͨ���������� pAVData[2]  ����2��λ�������2λ���� 0x03 �����㣬�õ���λ�����ƶ�2λ ���� �� �� pAVData[3] ��������2λ
		//pAVData[3] ������2λ��ȡ���� �� �� 0xc0 �����㣬������6λ��ΪʲôҪ����6λ����Ϊ��2λ�������λ������Ҫ���ұ��ƶ�6λ
		nChannels = ((nAudioData[2] & 0x03) << 2) | ((nAudioData[3] & 0xc0) >> 6);
		if (nChannels > 2)
			nChannels = 1;
		*nChans = nChannels;
  
		WriteLog(Log_Debug, "CNetRevcBase = %X ,ý����� AAC��Ϣ szAudioName = %s,nChannels = %d ,nSampleRate = %d ", this, "AAC", *nChans, *nSampleRate);
}

int CNetRevcBase::sdp_h264_load(uint8_t* data, int bytes, const char* config)
{
	int n, len, off;
	const char* p, *next;
	const uint8_t startcode[] = { 0x00, 0x00, 0x00, 0x01 };

	off = 0;
	p = config;
	while (p)
	{
		next = strchr(p, ',');
		len = next ? (int)(next - p) : (int)strlen(p);
		if (off + (len + 3) / 4 * 3 + (int)sizeof(startcode) > bytes)
			return -1; // don't have enough space

		memcpy(data + off, startcode, sizeof(startcode));
		n = (int)base64_decode(data + off + sizeof(startcode), p, len);
		assert(n <= (len + 3) / 4 * 3);
		off += n + sizeof(startcode);

		p = next ? next + 1 : NULL;
	}

	return off;
}

//��һ���ַ����п�����������־֮������ַ���
int  CNetRevcBase::GetSubFromString(char* szString, char* szStringFlag1, char* szStringFlag2, char* szOutString)
{
	string strSrc = szString;
	int   nRet = 0 ;
	int nPos1 = 0, nPos2 = 0;
	nPos1 = strSrc.find(szStringFlag1, 0);

	if (nPos1 >= 0)
 		nPos2 = strSrc.find(szStringFlag2, nPos1 + strlen(szStringFlag1));
	if (nPos1 > 0 && nPos2 > nPos1)
	{
		memcpy(szOutString, szString + nPos1+strlen(szStringFlag1), nPos2 - nPos1 - strlen(szStringFlag1));
		nRet = strlen(szOutString);
	}
	else if (nPos1 > 0 && nPos2 < 0)
	{
		memcpy(szOutString, szString + nPos1 + strlen(szStringFlag1), strlen(szString) - nPos1 - strlen(szStringFlag1));
		nRet = strlen(szOutString);
	}
 	return nRet;
}

bool  CNetRevcBase::GetH265VPSSPSPPS(char* szSDPString,int  nVideoPayload)
{//��ȡh265��VPS��SPS��PPS 
	m_bHaveSPSPPSFlag = false;
	char  vpsspsppsStr[string_length_2048] = { 0 };
	char  szFmt[string_length_1024] = { 0 };
	char  szVPS[string_length_2048] = { 0 };
	char  szSPS[string_length_2048] = { 0 };
	char  szPPS[string_length_2048] = { 0 };
	sprintf(szFmt, "a=fmtp:%d", nVideoPayload);
	if (GetSubFromString(szSDPString, szFmt, "\r\n", vpsspsppsStr) > 0)
	{
		GetSubFromString(vpsspsppsStr, "sprop-vps=", ";", szVPS);
		GetSubFromString(vpsspsppsStr, "sprop-sps=", ";", szSPS);
		GetSubFromString(vpsspsppsStr, "sprop-pps=", ";", szPPS);
		int nLength1 = 0, nLength2 = 0, nLength3 = 0;
		if (strlen(vpsspsppsStr) > 0)
		{//ת��Ϊ�����Ƶ�VPS��SPS��PPS
			nLength1 = sdp_h264_load((unsigned char*)m_pSpsPPSBuffer, sizeof(m_pSpsPPSBuffer), szVPS);
			nLength2 = sdp_h264_load((unsigned char*)m_pSpsPPSBuffer + nLength1, sizeof(m_pSpsPPSBuffer), szSPS);
			nLength3 = sdp_h264_load((unsigned char*)m_pSpsPPSBuffer + nLength1 + nLength2, sizeof(m_pSpsPPSBuffer), szPPS);
			m_nSpsPPSLength = nLength1 + nLength2 + nLength3;
			m_bHaveSPSPPSFlag = true;

			WriteLog(Log_Debug, "H265 vps = %s ,sps = %s ,pps = %s ", szVPS, szSPS,szPPS);
		}
	}
	return m_bHaveSPSPPSFlag;
}
//����aac���� �� 1 ͨ�� ��16000 ����Ƶ�� 
void   CNetRevcBase::AddMuteAACBuffer()
{
	if (nAddMuteAACBufferOrder <= 3)
	{
		if(nMediaSourceType == MediaSourceType_LiveMedia)
		   m_audioFifo.push(muteAACBuffer[nAddMuteAACBufferOrder].pAACBuffer + 4 , muteAACBuffer[nAddMuteAACBufferOrder].nAACLength - 4);
		else 
		   m_audioFifo.push(muteAACBuffer[nAddMuteAACBufferOrder].pAACBuffer , muteAACBuffer[nAddMuteAACBufferOrder].nAACLength);
	}
	else
	{
		if (nMediaSourceType == MediaSourceType_LiveMedia)
		  m_audioFifo.push(muteAACBuffer[3].pAACBuffer + 4 , muteAACBuffer[3].nAACLength - 4);
		else 
		  m_audioFifo.push(muteAACBuffer[3].pAACBuffer, muteAACBuffer[3].nAACLength);
	}

	if(nAddMuteAACBufferOrder < 64 )
	  nAddMuteAACBufferOrder ++;
}

//��ȡ��ǰʱ��
void  CNetRevcBase::GetCurrentDatetime()
{
#ifdef OS_System_Windows
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(szCurrentDateTime, "%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
#else
	time_t now;
	time(&now);
	struct tm *local;
	local = localtime(&now);
	sprintf(szCurrentDateTime, "%04d%02d%02d%02d%02d%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
#endif
}

//�ȴ���ȡý��Դ
#ifdef USE_BOOST
boost::shared_ptr<CMediaStreamSource>  CNetRevcBase::WaitGetMediaStreamSource(char* szMediaSourceURL)
#else
std::shared_ptr<CMediaStreamSource>  CNetRevcBase::WaitGetMediaStreamSource(char* szMediaSourceURL)
#endif
{
	if (ABL_MediaServerPort.hook_enable == 0)
		return NULL;

	uint64_t  tCurrentSecond = GetCurrentSecond();
	while (true)
	{
		auto  pCurrentMediaSource = GetMediaStreamSource(szMediaSourceURL);
		if (pCurrentMediaSource != NULL )
			return  pCurrentMediaSource;
		else
		{
			if (GetCurrentSecond() - tCurrentSecond > 15)
				return NULL ;
		//	Sleep(100);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

//����·��Ȩ�� 
void  CNetRevcBase::SetPathAuthority(char* szPath)
{
#ifdef  OS_System_Windows
 
#else
 	sprintf(szCmd,"cd %s",szPath);
	system(szCmd) ;
	system("chmod -R 777 *");
#endif	
}

//���������� ת��Ϊ������ʱ���� 
char*   CNetRevcBase::getDatetimeBySecond(time_t tSecond)
{
	memset(szDatetimeBySecond, 0x00, sizeof(szDatetimeBySecond));
	time_t now = tSecond;
	struct tm *local;
	local = localtime(&now);
	sprintf(szDatetimeBySecond, "%04d%02d%02d%02d%02d%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;

	return szDatetimeBySecond;
}

#ifdef  OS_System_Windows
//�����ļ���С 
bool CNetRevcBase::ftruncate(char* szFileName, DWORD nFileSize)
{
	HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		WriteLog(Log_Debug, "ftruncate::CreateFile() ʧ�� ��szFile = %s ", szFileName);
		return false;
	}

	DWORD dwFileNewSize = SetFilePointer(hFile, nFileSize, NULL, FILE_BEGIN);
	if (dwFileNewSize == INVALID_SET_FILE_POINTER)
	{
		CloseHandle(hFile);
		WriteLog(Log_Debug, "ftruncate::SetFilePointer() ʧ�� ��szFile = %s ", szFileName);
		return false;
	}

	if (!SetEndOfFile(hFile))
	{
		CloseHandle(hFile);
		WriteLog(Log_Debug, "ftruncate::SetEndOfFile() ʧ�� ��szFile = %s ", szFileName);
		return false;
	}

	CloseHandle(hFile); 
	return true;
}

#endif

//����SIM���ĺ���
bool CNetRevcBase::UpdateSim(char* szSIM)
{
	if (szSIM == NULL || atoi(szSIM) == 0)
		return false;

	int nSize = strlen(szSIM);
	int nPos = -1;
	for (int i = 0; i < nSize; i++)
	{
		if (szSIM[i] != 0x30)
		{
			nPos = i;
			break;
		}
	}
	if (nPos != -1)
	{
		memmove(szSIM, szSIM + nPos, nSize - nPos);
		szSIM[nSize - nPos] = 0x00;
	}
	return true;
}

char* CNetRevcBase::GetCurrentDateTime()
{
#ifdef OS_System_Windows
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(szEndDateTime, "%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
#else
	time_t now;
	time(&now);
	struct tm *local;
	local = localtime(&now);
	sprintf(szEndDateTime, "%04d%02d%02d%02d%02d%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
#endif

	return  szEndDateTime;
}
