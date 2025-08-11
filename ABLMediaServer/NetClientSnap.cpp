/*
���ܣ�
    ��Ƶץ�Ĺ��� 
 	 
����    2022-03-16
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientSnap.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern boost::shared_ptr<CPictureFileSource> GetPictureFileSource(char* szShareURL, bool bLock);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort; 
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
int CNetClientSnap::nPictureNumber           = 1;
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern std::shared_ptr<CPictureFileSource> GetPictureFileSource(char* szShareURL, bool bLock);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort;
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
int CNetClientSnap::nPictureNumber = 1;
#endif

CNetClientSnap::CNetClientSnap(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	bWaitIFrameFlag = false;
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	netBaseNetType = NetBaseNetType_SnapPicture_JPEG;
	nMediaClient = 0;
	nNetGB28181ProxyType = 0;
	bSnapSuccessFlag = false; //��λΪ��δץ�ĳɹ�

	SplitterAppStream(szShareMediaURL);

#ifdef  OS_System_Windows
	sprintf(szPicturePath, "%s%s", ABL_MediaServerPort.picturePath, m_addStreamProxyStruct.app);
	::CreateDirectory(szPicturePath, NULL);

	sprintf(szPicturePath, "%s%s\\%s\\", ABL_MediaServerPort.picturePath, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
	::CreateDirectory(szPicturePath, NULL);
#else
	sprintf(szPicturePath, "%s%s", ABL_MediaServerPort.picturePath, m_addStreamProxyStruct.app);
	umask(0);
	mkdir(szPicturePath, 777);
	SetPathAuthority(szPicturePath);
 
	sprintf(szPicturePath, "%s%s/%s/", ABL_MediaServerPort.picturePath, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
 	mkdir(szPicturePath, 777);
	SetPathAuthority(szPicturePath);
	
#endif

	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);

	WriteLog(Log_Debug, "CNetClientSnap ���� = %X  nClient = %llu ", this, nClient);
}

CNetClientSnap::~CNetClientSnap()
{
	std::lock_guard<std::mutex> lock(NetClientSnapLock);

	if (!bSnapSuccessFlag)
	{
	   sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"getSnap Error , Catpuring takes time %d millisecond .\"}", IndexApiCode_RequestProcessFailed, GetTickCount64() - nPrintTime);
	   ResponseHttp2(nClient_http, szResponseBody, false);
  	}

 	m_videoFifo.FreeFifo();

	WriteLog(Log_Debug, "CNetClientSnap ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetClientSnap::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	//������߼��
	nRecvDataTimerBySecond = 0;

	if (!bSnapSuccessFlag)
	{
		if (bWaitIFrameFlag)
		{//��Ҫ�ȵ�I֡
			if (CheckVideoIsIFrame(szVideoCodec, pVideoData, nDataLength) == false )
			{//�ȴ�I֡
				return 0;
			}else 
				bWaitIFrameFlag = false ;
		}

		m_videoFifo.push(pVideoData, nDataLength);
 	}
	return 0;
}

int CNetClientSnap::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetClientSnap::SendVideo()
{
	std::lock_guard<std::mutex> lock(NetClientSnapLock);

	unsigned char* pData = NULL;
	int            nLength = 0;
	char           szDownLoadImage[string_length_2048] = { 0 };

	if (!bSnapSuccessFlag && (pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (!videoDecode.m_bInitDecode)
			videoDecode.startDecode(mediaCodecInfo.szVideoName, 0, 0);

		if (videoDecode.m_bInitDecode )
		{
			if (videoDecode.DecodeYV12Image(pData, nLength) > 0 && videoDecode.pDPicture->key_frame == 1)
			{
 #ifdef OS_System_Windows
				SYSTEMTIME st;
				GetLocalTime(&st);
				sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d%02d.jpg", szRecordPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, nPictureNumber);
				sprintf(szFileNameOrder, "%04d%02d%02d%02d%02d%02d%02d.jpg", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, nPictureNumber);;
#else
				time_t now;
				time(&now);
				struct tm *local;
				local = localtime(&now);
				sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d%02d.jpg", szRecordPath, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, nPictureNumber);
				sprintf(szFileNameOrder, "%04d%02d%02d%02d%02d%02d%02d.jpg", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, nPictureNumber);;
#endif
				nPictureNumber ++;
				if (nPictureNumber > 99)
					nPictureNumber = 1;

				auto pPicture = GetPictureFileSource(m_szShareMediaURL, true);
				if (pPicture)
				{
					sprintf(szPictureFileName, "%s%s", szPicturePath, szFileName);
					pPicture->UpdateExpirePictureFile(szPictureFileName);

					if (videoDecode.CaptureJpegFromAVFrame(szPictureFileName, 98))
					{
					   bSnapSuccessFlag = true ;
					   pPicture->AddPictureFile(szFileNameOrder);

					   if (atoi(m_getSnapStruct.captureReplayType) == 1)
					   {//����url
					       sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success , Catpuring takes time %d millisecond .\",\"url\":\"http://%s:%d/%s/%s/%s\"}", GetTickCount64() - nPrintTime, ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpServerPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, szFileName);
					       ResponseHttp(nClient_http, szResponseBody, false);
					   }
					   else
					   {//ֱ�ӷ���ͼƬ
						   auto  pHttpClient =  GetNetRevcBaseClient(nClient_http);
						   if (pHttpClient != NULL)
						   {
							   CNetServerHTTP* httResponse = (CNetServerHTTP*) pHttpClient.get();
							   if (httResponse)
							   {
								   sprintf(szDownLoadImage, "/%s/%s/%s", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, szFileNameOrder);
								   httResponse->index_api_downloadImage(szDownLoadImage);
							   }
						   }
 					   }

					   if(ABL_MediaServerPort.snapObjectDestroy == 1)
						   pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
					   else
					   {//�ӿ����̡߳������߳��Ƴ�
						   bWaitIFrameFlag = true;//��Ҫ�ȵ�I֡
						   auto pMediaSouce = GetMediaStreamSource(m_szShareMediaURL);
						   if (pMediaSouce)
						   {//�ӿ����̣߳������߳��Ƴ�
							   pMediaSouce->DeleteClientFromMap(nClient);
						   }
					   }
					}
				}
				else
				{
					sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Error , Catpuring takes time %d millisecond .\"}", IndexApiCode_RequestProcessFailed, GetTickCount64() - nPrintTime);
					ResponseHttp(nClient_http, szResponseBody, false);
				}
 			}
		}

		m_videoFifo.pop_front();
	}
	return 0;
}

int CNetClientSnap::SendAudio()
{

	return 0;
}

int CNetClientSnap::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CNetClientSnap::ProcessNetData()
{
 	return 0;
}

//���͵�һ������
int CNetClientSnap::SendFirstRequst()
{
	auto pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
	if (pMediaSource == NULL)
	{
		WriteLog(Log_Debug, "CNetClientSnap = %X nClient = %llu ,������ý��Դ %s", this, nClient, m_szShareMediaURL);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1 ;
	}
	bSnapSuccessFlag = false; //��λΪ��δץ�ĳɹ�
	nPrintTime = nCreateDateTime = GetTickCount64();//ˢ��ʱ��
	pMediaSource->AddClientToMap(nClient);

    return 0;
}

//����m3u8�ļ�
bool  CNetClientSnap::RequestM3u8File()
{
	return true;
}