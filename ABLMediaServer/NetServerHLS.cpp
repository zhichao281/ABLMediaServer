/*
���ܣ�
       ʵ��HLS��������ý�����ݷ��͹��� 
����    2021-05-20   ʵ��hls֧��
        2023-11-06   ����¼��ط�hls��֧�� 

����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerHLS.h"

extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);

#ifdef USE_BOOST
extern boost::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#else
extern std::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#endif

extern CMediaFifo                      pDisconnectBaseNetFifo; //������ѵ����� 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern char                            ABL_wwwMediaPath[256]; //www ��·��
extern MediaServerPort                 ABL_MediaServerPort;
extern uint64_t                        ABL_nBaseCookieNumber ; //Cookie ��� 
extern uint64_t                        GetCurrentSecond();
extern CMediaFifo                      pMessageNoticeFifo; //��Ϣ֪ͨFIFO

CNetServerHLS::CNetServerHLS(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	memset(szOrigin, 0x00, sizeof(szOrigin));
	strcpy(m_szShareMediaURL, szShareMediaURL);

	MaxNetDataCacheCount = MaxHttp_FlvNetCacheBufferLength;
	memset(netDataCache, 0x00, sizeof(netDataCache));
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//�������ݻ����С
	bFindHLSNameFlag = false;
	memset(szRequestFileName, 0x00, sizeof(szRequestFileName));

	netBaseNetType = NetBaseNetType_HttpHLSServerSendPush;
	bRequestHeadFlag = false;
	memset(szCookieNumber, 0x00, sizeof(szCookieNumber));

	//�״η����ڴ� 
	pTsFileBuffer = NULL;
	while(pTsFileBuffer == NULL)
	{
		nCurrentTsFileBufferSize = MaxDefaultTsFmp4FileByteCount;
		pTsFileBuffer = new unsigned char[nCurrentTsFileBufferSize];
	}
	WriteLog(Log_Debug, "CNetServerHLS ���� = %X,  nClient = %llu ",this, nClient);
}

CNetServerHLS::~CNetServerHLS()
{
	bRunFlag.exchange(false);
	std::lock_guard<std::mutex> lock(netDataLock);
	
	if (pTsFileBuffer != NULL)
	{
	  delete [] pTsFileBuffer;
	  pTsFileBuffer;
	}
	httpParse.FreeSipString();
 
	WriteLog(Log_Debug, "CNetServerHLS ���� = %X  szRequestFileName = %s, nClient = %llu \r\n", this, szRequestFileName, nClient);
	malloc_trim(0);
}

int CNetServerHLS::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	return 0 ;
}

int CNetServerHLS::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	return 0;
}

int CNetServerHLS::SendVideo()
{

	return 0;
}

int CNetServerHLS::SendAudio()
{

	return 0;
}

int CNetServerHLS::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag.load())
		return -1;
	std::lock_guard<std::mutex> lock(netDataLock);

	//������߼��
	nRecvDataTimerBySecond = 0;

	//WriteLog(Log_Debug, "CNetServerHLS= %X , nClient = %llu ,�յ�Ƭ������\r\n%s",this,nClient,pData);
 
	if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
	{//ʣ��ռ��㹻
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//ʣ��ռ䲻������Ҫ��ʣ���buffer��ǰ�ƶ�
		if (netDataCacheLength > 0 && netDataCacheLength == (nNetEnd - nNetStart) )
		{//���������ʣ��
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;

			if (MaxNetDataCacheCount - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetRtspServer = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return 0;
			}
			//�����ʷ�Ͼ�����
			memset(netDataCache + nNetEnd, 0x00, sizeof(netDataCache) - nNetEnd);
		}
		else
		{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
			nNetStart = 0;
			nNetEnd = 0;
			netDataCacheLength = 0;

			//�����ʷ�Ͼ�����
			memset(netDataCache , 0x00, sizeof(netDataCache));
		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	return true;
}

//��HTTPͷ�л�ȡ������ļ���
bool CNetServerHLS::GetHttpRequestFileName(char* szGetRequestFile,char* szHttpHeadData)
{
	string  strHttpHead = (char*)szHttpHeadData;
	int     nPos1, nPos2;
	char    szFindHead[64] = { 0 };
 	int     nHeadLength = 4 ;

	strcpy(szFindHead, "HEAD ");
	nPos1 = strHttpHead.find(szFindHead, 0);
	if (nPos1 < 0)
	{
		memset(szFindHead, 0x00, sizeof(szFindHead));
		strcpy(szFindHead, "GET ");
		nPos1 = strHttpHead.find(szFindHead, 0);
	}
	else//����head ����m3u8 �ļ� 
		bRequestHeadFlag = true;

	nHeadLength = strlen(szFindHead);

	if (nPos1 >= 0)
	{
		nPos2 = strHttpHead.find(" HTTP/", 0);
		if (nPos2 > 0)
		{
			memcpy(szGetRequestFile, szHttpHeadData + nPos1 + nHeadLength, nPos2 - nPos1 - nHeadLength);
 			//WriteLog(Log_Debug, "CNetServerHLS=%X ,nClient = %llu ,������HTTP ���� �ļ����� %s ", this, nClient, szGetRequestFile);
			return true;
		}
	}
	return false;
}

//��ȡhttp����buffer 
bool  CNetServerHLS::ReadHttpRequest()
{
	std::lock_guard<std::mutex> lock(netDataLock);

	if (netDataCacheLength <= 0 || nNetStart <  0)
  		return false;
 
	string strNetData = (char*)netDataCache ;
	int    nPos;
	nPos = strNetData.find("\r\n\r\n", nNetStart);

	if (nPos <= 0 || (nPos - nNetStart) + 4 > sizeof(szHttpRequestBuffer))
 		return false;
 
	memset(szHttpRequestBuffer, 0x00, sizeof(szHttpRequestBuffer));
	memcpy(szHttpRequestBuffer, netDataCache + nNetStart , (nPos - nNetStart) + 4);

	netDataCacheLength   -=   (nPos - nNetStart) + 4 ; //���4 ������ "\r\n\r\n" , �Ѿ��������ˡ�
	nNetStart             =    nPos  + 4 ; //���4 ������ "\r\n\r\n"�� �Ѿ��������ˡ�

 	return true;
}
 
int CNetServerHLS::ProcessNetData()
{
	if (!bRunFlag.load())
		return -1;

	if (netDataCacheLength > string_length_4096 )
	{
		WriteLog(Log_Debug, "CNetServerHLS = %X , nClient = %llu ,netDataCacheLength = %d, ���͹�����url���ݳ��ȷǷ� ,����ɾ�� ", this, nClient, netDataCacheLength);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
	}

  	if (ReadHttpRequest() == false )
	{
		WriteLog(Log_Debug, "CNetServerHLS = %X ,nClient = %llu , ������δ�������� ",this,nClient);
		if (memcmp(netDataCache, "GET ", 4) != 0)
		{
			WriteLog(Log_Debug, "CNetServerHLS = %X , nClient = %llu , ���յ����ݷǷ� ",this, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		}
		return -1;
 	}

	memset(szDateTime1, 0x00, sizeof(szDateTime1));
	memset(szDateTime2, 0x00, sizeof(szDateTime2));
	
#ifdef OS_System_Windows	
	SYSTEMTIME st;
	GetLocalTime(&st);//Tue, Jun 31 2021 06:19:02 GMT
	sprintf(szDateTime1,"Tue, Jun %02d %04d %02d:%02d:%02d GMT", st.wDay, st.wYear, st.wHour - 8,st.wMinute, st.wSecond);
	sprintf(szDateTime2, "Tue, Jun %02d %04d %02d:%02d:%02d GMT", st.wDay, st.wYear, st.wHour - 8, st.wMinute+2, st.wSecond);
#else
	time_t now;
	time(&now);
	struct tm *local;
	local = localtime(&now);
	
	sprintf(szDateTime1,"Tue, Jun %02d %04d %02d:%02d:%02d GMT", local->tm_mday, local->tm_year+1900, local->tm_hour - 8,local->tm_min, local->tm_sec);
	sprintf(szDateTime2, "Tue, Jun %02d %04d %02d:%02d:%02d GMT", local->tm_mday,local->tm_year+1900, local->tm_hour - 8, local->tm_min+2, local->tm_sec);
#endif

	//����httpͷ
	httpParse.ParseSipString((char*)szHttpRequestBuffer);

	//��ȡ��HTTP������ļ����� 
	memset(szRequestFileName, 0x00, sizeof(szRequestFileName));
	if (!GetHttpRequestFileName(szRequestFileName, szHttpRequestBuffer))
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, ��ȡhttp �����ļ�ʧ�ܣ� nClient = %llu ", this, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}  

 	string strRequestFileName = szRequestFileName;
	int    nPos = 0 ;

    //������Ȩ����
	if (strlen(szPlayParams) == 0 && strstr(szRequestFileName,".m3u8") != NULL )
	{
		nPos = strRequestFileName.find("?", 0);
		if(nPos > 0 )
		  memcpy(szPlayParams, szRequestFileName + (nPos + 1), strlen(szRequestFileName) - nPos - 1);
 	}

	memset(szPushName, 0x00, sizeof(szPushName));
	nPos = strRequestFileName.rfind(".m3u8",strlen(szRequestFileName));
	if (nPos > 0)
	{
		memcpy(szPushName,szRequestFileName,nPos);
 	}else 
	{
		nPos = strRequestFileName.rfind("/", strlen(szRequestFileName));
		if(nPos > 0)
		   memcpy(szPushName, szRequestFileName, nPos);
	}
	SplitterAppStream(szPushName);

	//���Ͳ����¼�֪ͨ�����ڲ��ż�Ȩ
	if (ABL_MediaServerPort.hook_enable == 1 && bOn_playFlag == false && strstr(szRequestFileName, ".m3u8") != NULL)
	{
 		bOn_playFlag = true;
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_on_play;
		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_play\",\"app\":\"%s\",\"stream\":\"%s\",\"readerCount\": %d,\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu,\"ip\":\"%s\" ,\"port\":%d,\"params\":\"%s\"}", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, readerCount, ABL_MediaServerPort.mediaServerID, netBaseNetType, nClient, szClientIP, nClientPort, szPlayParams);
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}

  	//WriteLog(Log_Debug, "CNetServerHLS=%X, ��ȡ��HLS������Դ���� szPushName = %s , nClient = %llu ", this, szPushName, nClient);

	//��ȡConnection ���ӷ�ʽ�� Close ,Keep-Live  
	memset(szConnectionType, 0x00, sizeof(szConnectionType));
	if (!httpParse.GetFieldValue("Connection", szConnectionType))
		strcpy(szConnectionType, "Close");

	//����Cookie 
 	memset(szCookieNumber, 0x00, sizeof(szCookieNumber));
#if  1 //�������е�Cookie�㷨
	sprintf(szCookieNumber, "ABLMediaServer%018llu", ABL_nBaseCookieNumber);
	ABL_nBaseCookieNumber ++;
#else 
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()(); // ����������() ����Ϊ�����ǵ��õ� () �����������
	string tmp_uuid = boost::uuids::to_string(a_uuid);
	boost::algorithm::erase_all(tmp_uuid, "-");
	strcpy(szCookieNumber,tmp_uuid.c_str());
#endif

	//��ȡ szOrigin
	memset(szOrigin, 0x00, sizeof(szOrigin));
	httpParse.GetFieldValue("Origin", szOrigin);
	if (strlen(szOrigin) == 0)
		strcpy(szOrigin, "*");
 	
	//WriteLog(Log_Debug, "CNetServerHLS= %X, nClient = %llu , ��ȡ�������ļ� szRequestFileName = %s ", this,  nClient, szRequestFileName);
	if (strstr(szRequestFileName, RecordFileReplaySplitter) != NULL)
	{//¼��ط�
		SendRecordHLS();
	}else //ʵ������ 
	    SendLiveHLS();//����ʵ����hls 

#if 0  //���������������Ͽ�������VLC���Ų����� ,ffplay Ҳ�������Ų�����
	  //�������,����Ƕ����ӣ�����ɾ��
	  if(strcmp(szConnectionType,"Close") == 0 || strcmp(szConnectionType, "close") == 0 || bRequestHeadFlag == true)
	      pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
#endif

	 httpParse.FreeSipString();
 
	return 0;
}

int CNetServerHLS::SendLiveHLS()
{
	//�������������ҵ�
	auto pushClient = GetMediaStreamSource(szPushName, true);
	if (pushClient == NULL)
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, û����������ĵ�ַ %s nClient = %llu ", this, szPushName, nClient);

		sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
	}
	readerCount = pushClient->mediaSendMap.size();
	//����HLS���ۿ�ʱ��,��ΪHLS���ţ��������ַ�ʽ���ж�ĳ·���Ƿ��ڹۿ���
	pushClient->nLastWatchTime = pushClient->nRecordLastWatchTime = pushClient->nLastWatchTimeDisconect = GetCurrentSecond();

	//����ý��Դ
	sprintf(m_addStreamProxyStruct.url, "http://localhost:%d/%s/%s.m3u8", ABL_MediaServerPort.nHlsPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);

	if (strstr(szRequestFileName, ".m3u8") != NULL)
	{//����M3U8�ļ�
		if (strlen(pushClient->szDataM3U8) == 0)
		{//m3u8�ļ���δ���ɣ����ոտ�ʼ��Ƭ�����ǻ�����3���ļ� 
			WriteLog(Log_Debug, "CNetServerHLS=%X, m3u8�ļ���δ���ɣ����ոտ�ʼ��Ƭ�����ǻ�����3��TS�ļ� %s nClient = %llu ", this, szPushName, nClient);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		}

		memset(szM3u8Content, 0x00, sizeof(szM3u8Content));
		strcpy(szM3u8Content, pushClient->szDataM3U8);
		if (bRequestHeadFlag == true)
		{//HEAD ����
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: close\r\nContent-Length: 0\r\nDate: %s\r\nServer: %s\r\n\r\n", szOrigin, szDateTime1, MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ظ�HEAD���� httpResponseData = %s, nClient = %llu ", this, httpResponseData, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return 0;
		}
		else
		{
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: application/vnd.apple.mpegurl; charset=utf-8\r\nDate: %s\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\nSet-Cookie: AB_COOKIE=%s;expires=%s;path=%s/\r\n\r\n", szOrigin, szConnectionType,
				strlen(szM3u8Content), szDateTime1, MediaServerVerson, szCookieNumber, szDateTime2, szPushName);
		}

		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);
		nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)szM3u8Content, strlen(szM3u8Content), ABL_MediaServerPort.nSyncWritePacket);
		if (nWriteRet != 0 || nWriteRet2 != 0)
		{
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ظ�httpʧ�� szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		WriteLog(Log_Debug, "CNetServerHLS=%X, �������m3u8�ļ� szRequestFileName = %s, nClient = %llu , �ļ��ֽڴ�С %d ", this, szRequestFileName, nClient, strlen(szM3u8Content));
		//WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu ����http�ظ���\r\n%s", this, nClient, httpResponseData);
		//WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu ����http�ظ���\r\n%s", this, nClient, szM3u8Content);
		return 0;
	}
	else if (strstr(szRequestFileName, ".ts") != NULL || strstr(szRequestFileName, ".mp4") != NULL)
	{//����TS�ļ� 
		nTsFileNameOrder = GetTsFileNameOrder(szRequestFileName);
		if (nTsFileNameOrder == -1)
		{
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		//TS�ļ����ֽ���
		fFileByteCount = pushClient->GetTsFileSizeByOrder(nTsFileNameOrder);
		if (fFileByteCount <= 0)
		{//TS�ļ��ֽ���������
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ļ���ʧ�� szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		//���Ҫ��ȡ���ļ��ֽ�������  nCurrentTsFileBufferSize
		if (fFileByteCount > nCurrentTsFileBufferSize)
		{
			delete[] pTsFileBuffer;
			nCurrentTsFileBufferSize = fFileByteCount + 1024 * 512; //������512K 
			pTsFileBuffer = new unsigned char[nCurrentTsFileBufferSize];
		}

		if (ABL_MediaServerPort.nHLSCutType == 1)
		{//��Ƭ��Ӳ��
#ifdef OS_System_Windows
			sprintf(szReadFileName, "%s\\%s", ABL_wwwMediaPath, szRequestFileName);
#else 
			sprintf(szReadFileName, "%s/%s", ABL_wwwMediaPath, szRequestFileName);
#endif
			WriteLog(Log_Debug, "CNetServerHLS=%X, ��ʼ��ȡTS�ļ� szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);
			FILE* tsFile = fopen(szReadFileName, "rb");
			if (tsFile == NULL)
			{//��TS�ļ�ʧ��
				WriteLog(Log_Debug, "CNetServerHLS=%X, �ļ���ʧ�� szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return -1;
			}
			fread(pTsFileBuffer, 1, fFileByteCount, tsFile);
			fclose(tsFile);
		}
		else if (ABL_MediaServerPort.nHLSCutType == 2)
		{
			if (ABL_MediaServerPort.nH265CutType == 2)
			{
				if (nTsFileNameOrder == 0)//������� SPS \ PPS �� 0.mp4 �ļ� 
				{
					if (pushClient->nFmp4SPSPPSLength > 0)
						memcpy(pTsFileBuffer, pushClient->pFmp4SPSPPSBuffer, pushClient->nFmp4SPSPPSLength);
					else
					{
						WriteLog(Log_Debug, "CNetServerHLS=%X, fmp4 ��Ƭû������ 0.mp4 �ļ� szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

						sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
						nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

						pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
						return -1;
					}
				}
				else
					pushClient->CopyTsFileBuffer(nTsFileNameOrder, pTsFileBuffer);
			}
			else
				pushClient->CopyTsFileBuffer(nTsFileNameOrder, pTsFileBuffer);
		}

		//����httpͷ
		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: video/mp2t; charset=utf-8\r\nDate: %s\r\nkeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin,
			szConnectionType,
			fFileByteCount,
			szDateTime1,
			MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

		//����TS���� 
		int             nPos = 0;
		while (fFileByteCount > 0 && pTsFileBuffer != NULL)
		{
			if (fFileByteCount > Send_TsFile_MaxPacketCount)
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer + nPos, Send_TsFile_MaxPacketCount, ABL_MediaServerPort.nSyncWritePacket);
				fFileByteCount -= Send_TsFile_MaxPacketCount;
				nPos += Send_TsFile_MaxPacketCount;
			}
			else
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer + nPos, fFileByteCount, ABL_MediaServerPort.nSyncWritePacket);
				nPos += fFileByteCount;
				fFileByteCount = 0;
			}

			if (nWriteRet2 != 0)
			{//���ͳ���
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				break;
			}
		}
		WriteLog(Log_Debug, "CNetServerHLS=%X, �������TS��FMP4 �ļ� szRequestFileName = %s, nClient = %llu ,�ļ��ֽڴ�С %d", this, szRequestFileName, nClient, nPos);
		return 0;
	}
	else
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, ���� http �ļ��������� szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}
	return 0;
}

int CNetServerHLS::SendRecordHLS()
{
 	if (strstr(szRequestFileName, ".m3u8") != NULL)
	{//����M3U8�ļ�
		string  strTemp = szRequestFileName;
 
#ifdef USE_BOOST
		replace_all(strTemp, RecordFileReplaySplitter, "/");
#else
		ABL::replace_all(strTemp, RecordFileReplaySplitter, "/");
#endif
		sprintf(szRequestFileName, "%s%s", ABL_MediaServerPort.recordPath, strTemp.c_str()+1);
 
		strTemp = szRequestFileName;
		int nPos = strTemp.rfind("?", strTemp.size());
		if (nPos > 0)
			szRequestFileName[nPos] = 0x00;

		FILE* fReadM3u8 = NULL;
		fReadM3u8 = fopen(szRequestFileName, "rb");
		
 		if (fReadM3u8  == NULL)
		{//������m3u8�ļ�  
			WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu , �������ļ� %s  ", this, nClient, szRequestFileName);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		}

		memset(szM3u8Content, 0x00, sizeof(szM3u8Content));
		fread(szM3u8Content, 1, sizeof(szM3u8Content), fReadM3u8);
		fclose(fReadM3u8);

		if (bRequestHeadFlag == true)
		{//HEAD ����
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: close\r\nContent-Length: 0\r\nDate: %s\r\nServer: %s\r\n\r\n", szOrigin, szDateTime1, MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ظ�HEAD���� httpResponseData = %s, nClient = %llu ", this, httpResponseData, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return 0;
		}
		else
		{
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: application/vnd.apple.mpegurl; charset=utf-8\r\nDate: %s\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\nSet-Cookie: AB_COOKIE=%s;expires=%s;path=%s/\r\n\r\n", szOrigin, szConnectionType,
				strlen(szM3u8Content), szDateTime1, MediaServerVerson, szCookieNumber, szDateTime2, szPushName);
		}

		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);
		nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)szM3u8Content, strlen(szM3u8Content), ABL_MediaServerPort.nSyncWritePacket);
		if (nWriteRet != 0 || nWriteRet2 != 0)
		{
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ظ�httpʧ�� szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		} 

		WriteLog(Log_Debug, "CNetServerHLS=%X, �������m3u8�ļ� szRequestFileName = %s, nClient = %llu , �ļ��ֽڴ�С %d ", this, szRequestFileName, nClient, strlen(szM3u8Content));
 	}
	else if (strstr(szRequestFileName, ".ts") != NULL || strstr(szRequestFileName, ".mp4") != NULL)
	{//����TS�ļ� 
		string  strTemp = szRequestFileName;

#ifdef USE_BOOST
		replace_all(strTemp, RecordFileReplaySplitter, "/");
#else
		ABL::replace_all(strTemp, RecordFileReplaySplitter, "/");
#endif
		sprintf(szRequestFileName, "%s%s", ABL_MediaServerPort.recordPath, strTemp.c_str() + 1);

		FILE* fReadMP4 = NULL;
		fReadMP4 = fopen(szRequestFileName, "rb");

		if (fReadMP4 == NULL)
		{//������ mp4 �ļ� 
			WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu , �������ļ� %s  ", this, nClient, szRequestFileName);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		}
	
		//��ȡ�ļ���С 
#ifdef OS_System_Windows 
		struct _stat64 fileBuf;
		int error = _stat64(szRequestFileName, &fileBuf);
		if (error == 0)
			fFileByteCount = fileBuf.st_size;
#else 
		struct stat fileBuf;
		int error = stat(szRequestFileName, &fileBuf);
		if (error == 0)
			fFileByteCount = fileBuf.st_size;
#endif

		//����httpͷ
		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: video/mp2t; charset=utf-8\r\nDate: %s\r\nkeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin,
			szConnectionType,
			fFileByteCount,
			szDateTime1,
			MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), ABL_MediaServerPort.nSyncWritePacket);

		//����TS���� 
		int             nRead = 0;
		while (true)
		{
			nRead = fread(pTsFileBuffer, 1, 1024 * 1024 * 1, fReadMP4);
			if (nRead > 0)
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer, nRead, ABL_MediaServerPort.nSyncWritePacket);
 			}
			else
				break;
 
			if (nWriteRet2 != 0)
			{//���ͳ���
		        if(fReadMP4)
				{
				  fclose(fReadMP4);
				  fReadMP4 = NULL ;
				}
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				break;
			}
		}
		if(fReadMP4)
		{
		   fclose(fReadMP4) ;
		   fReadMP4 = NULL ;
		}
		WriteLog(Log_Debug, "CNetServerHLS=%X, �������TS��FMP4 �ļ� szRequestFileName = %s, nClient = %llu ,�ļ��ֽڴ�С %d", this, szRequestFileName, nClient, fFileByteCount);
		return 0;
	}
	else
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, ���� http �ļ��������� szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}
	return 0;
}
 
//����TS�ļ����� ����ȡ�ļ���� 
int64_t  CNetServerHLS::GetTsFileNameOrder(char* szTsFileName)
{
	string strTsFileName = szTsFileName;
	int    nPos1, nPos2;
	char   szTemp[128] = { 0 };

	nPos1 = strTsFileName.rfind("/", strlen(szTsFileName));
	nPos2 = strTsFileName.find(".ts", 0);
	if (nPos1 > 0 && nPos2 > 0)
	{//ts 
		memcpy(szTemp, szTsFileName + nPos1 + 1, nPos2 - nPos1 - 1);
		return atoi(szTemp);
	}
	else
	{//mp4 
		nPos2 = strTsFileName.find(".mp4", 0);
		if (nPos1 > 0 && nPos2 > 0)
		{
		  memcpy(szTemp, szTsFileName + nPos1 + 1, nPos2 - nPos1 - 1);
		  return atoi(szTemp);
 		}
	}
	return  -1 ;
}

//���͵�һ������
int CNetServerHLS::SendFirstRequst()
{
	return 0;
}

//����m3u8�ļ�
bool  CNetServerHLS::RequestM3u8File()
{
	return true;
}