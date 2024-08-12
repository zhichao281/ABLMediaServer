/*
功能：
       实现
        rtsp发送
日期    2021-07-31
作者    罗家兄弟
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
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 

extern size_t base64_decode(void* target, const char *source, size_t bytes);

extern MediaServerPort                       ABL_MediaServerPort;

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

CNetClientSendRtsp::CNetClientSendRtsp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nServer = hServer;
	nClient = hClient;
	hRtpVideo = 0;
	hRtpAudio = 0;
	nSendRtpVideoMediaBufferLength = nSendRtpAudioMediaBufferLength = 0;
	nCalcAudioFrameCount = 0;
	nStartVideoTimestamp = VideoStartTimestampFlag; //视频初始时间戳 
	nSendRtpFailCount = 0;//累计发送rtp包失败次数 
	strcpy(m_szShareMediaURL, szShareMediaURL);

	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	nPrintCount = 0;
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
	bRunFlag = true;
	strcpy(szTrackIDArray[1], "streamid=0");
	strcpy(szTrackIDArray[2], "streamid=1");
	WriteLog(Log_Debug, "CNetClientSendRtsp 构造 nClient = %llu ", nClient);
}

CNetClientSendRtsp::~CNetClientSendRtsp()
{
	WriteLog(Log_Debug, "CNetClientSendRtsp 等待任务退出 nTime = %llu, nClient = %llu ",GetTickCount64(), nClient);
	bRunFlag = false;
 	std::lock_guard<std::mutex> lock(businessProcMutex);
	
	for (int i = 0; i < 3; i++)
	{
		while (!bExitProcessFlagArray[i])
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			//Sleep(5);
	}
	WriteLog(Log_Debug, "CNetClientSendRtsp 任务退出完毕 nTime = %llu, nClient = %llu ", GetTickCount64(), nClient);
 
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

	WriteLog(Log_Debug, "CNetClientSendRtsp 析构 nClient = %llu \r\n", nClient);
	malloc_trim(0);
}
int CNetClientSendRtsp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0;
	if (!bRunFlag || m_addPushProxyStruct.disableVideo[0] != 0x30 )
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
	if (!bRunFlag || m_addPushProxyStruct.disableAudio[0] != 0x30)
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

//网络数据拼接 
int CNetClientSendRtsp::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(netDataLock);

	//网络断线检测
	nRecvDataTimerBySecond = 0;

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
				WriteLog(Log_Debug, "CNetClientSendRtsp = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
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
int32_t  CNetClientSendRtsp::XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain)
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
	while (!bIsInvalidConnectFlag && bRunFlag)
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

//获取HTTP方法，httpURL 
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
			{//去掉后面的
				szTempRtsp[nPos3] = 0x00;
			}
			strTempRtsp = szTempRtsp;

			//增加554 端口
			nPos4 = strTempRtsp.find(":", 8);
			if (nPos4 <= 0)
			{//没有554 端口
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

 //把http头数据填充到结构中
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

//统计rtspURL  rtsp://190.15.240.11:554/Media/Camera_00001 路径 / 的数量 
int  CNetClientSendRtsp::GetRtspPathCount(char* szRtspURL)
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

//rtp打包回调视频
void Video_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetClientSendRtsp* pRtspClient = (CNetClientSendRtsp*)cb->userdata;
	if(pRtspClient->bRunFlag)
	  pRtspClient->ProcessRtpVideoData(cb->data, cb->datasize);
}

//rtp 打包回调音频
void Audio_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetClientSendRtsp* pRtspClient = (CNetClientSendRtsp*)cb->userdata;
	if(pRtspClient->bRunFlag)
	  pRtspClient->ProcessRtpAudioData(cb->data, cb->datasize);
}

void CNetClientSendRtsp::ProcessRtpVideoData(unsigned char* pRtpVideo,int nDataLength)
{
	if ((MaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0 )
	{//剩余空间不够存储 ,防止出错 
		SumSendRtpMediaBuffer(szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength);
 		nSendRtpVideoMediaBufferLength = 0;
	}

	memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nStartVideoTimestamp != VideoStartTimestampFlag &&  nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//产生一帧新的视频 
		//WriteLog(Log_Debug, "CNetClientSendRtsp= %X, 发送一帧视频 ，Length = %d ", this, nSendRtpVideoMediaBufferLength);
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
		{//剩余空间不够存储 ,防止出错 
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
	{//单独发送音频
		nSendRtpAudioMediaBufferLength = 0;
		szSendRtpAudioMediaBuffer[nSendRtpAudioMediaBufferLength + 0] = '$';
		szSendRtpAudioMediaBuffer[nSendRtpAudioMediaBufferLength + 1] = 0;

		nAudioRtpLen = htons(nDataLength);
		memcpy(szSendRtpAudioMediaBuffer + (nSendRtpAudioMediaBufferLength + 2), (unsigned char*)&nAudioRtpLen, sizeof(nAudioRtpLen));
		memcpy(szSendRtpAudioMediaBuffer + (nSendRtpAudioMediaBufferLength + 4), pRtpAudio, nDataLength);
		XHNetSDK_Write(nClient, szSendRtpAudioMediaBuffer, nDataLength + 4, 1);
	}
}

//累积rtp包，发送
void CNetClientSendRtsp::SumSendRtpMediaBuffer(unsigned char* pRtpMedia, int nRtpLength)
{
	std::lock_guard<std::mutex> lock(MediaSumRtpMutex);
 
	if (bRunFlag)
	{
		nSendRet = XHNetSDK_Write(nClient, pRtpMedia, nRtpLength, 1);
		if (nSendRet != 0)
		{
			nSendRtpFailCount ++ ;
			//WriteLog(Log_Debug, "发送rtp 包失败 ,累计次数 nSendRtpFailCount = %d 次 ，nClient = %llu ", nSendRtpFailCount, nClient);
			if (nSendRtpFailCount >= 30)
			{
				bRunFlag = false;			
				WriteLog(Log_Debug, "发送rtp 包失败 ,累计次数 已经达到 nSendRtpFailCount = %d 次，即将删除 ，nClient = %llu ", nSendRtpFailCount, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			}
		}
		else
			nSendRtpFailCount = 0;
	}
}

//根据媒体信息拼装 SDP 信息
bool CNetClientSendRtsp::GetRtspSDPFromMediaStreamSource(RtspSDPContentStruct sdpContent, bool bGetFlag)
{
	memset(szRtspSDPContent, 0x00, sizeof(szRtspSDPContent));

	//视频
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
		WriteLog(Log_Debug, "CNetClientSendRtsp = %X ，没有视频资源 ,nClient = %llu", this, nClient);
	}

	int nRet = 0;
	if (strlen(sdpContent.szVideoName) > 0)
	{
		nRet = rtp_packet_start(Video_rtp_packet_callback_func_send, (void*)this, &hRtpVideo);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ，创建视频rtp打包失败,nClient = %llu,  nRet = %d", this,nClient, nRet);
			return false;
		}
 		optionVideo.handle = hRtpVideo;
		optionVideo.ssrc = nVideoSSRC;
		optionVideo.mediatype = e_rtppkt_mt_video;
 		optionVideo.payload = nVideoPayload;
		optionVideo.ttincre = 90000 / mediaCodecInfo.nVideoFrameRate; //视频时间戳增量

		inputVideo.handle = hRtpVideo;
		inputVideo.ssrc = optionVideo.ssrc;

		nRet = rtp_packet_setsessionopt(&optionVideo);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CRecvRtspClient = %X ，rtp_packet_setsessionopt 设置视频rtp打包失败 ,nClient = %llu nRet = %d", this, nClient, nRet);
			return false;
		}
	}
 	memset(szRtspAudioSDP, 0x00, sizeof(szRtspAudioSDP));
 
	memset((char*)&optionAudio, 0x00, sizeof(optionAudio));
	if (strcmp(sdpContent.szAudioName, "G711_A") == 0 && ABL_MediaServerPort.nEnableAudio == 1)
	{
		optionAudio.ttincre = 320; //g711a 的 长度增量
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
		optionAudio.ttincre = 320; //g711u 的 长度增量
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
		optionAudio.ttincre = 1024 ;//aac 的长度增量
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

	//追加音频SDP
	if (strlen(szRtspAudioSDP) > 0 && ABL_MediaServerPort.nEnableAudio == 1)
	{
		int32_t nRet = rtp_packet_start(Audio_rtp_packet_callback_func_send, (void*)this, &hRtpAudio);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ，创建音频rtp打包失败 ,nClient = %llu,  nRet = %d", this,nClient, nRet);
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
	WriteLog(Log_Debug, "组装好SDP :\r\n%s", szRtspSDPContent);

	return true;
}

//处理rtsp数据
void  CNetClientSendRtsp::InputRtspData(unsigned char* pRecvData, int nDataLength)
{
#if 0
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
		RtspSDPContentStruct sdpContent;

		auto pMediaSource = GetMediaStreamSource(m_szShareMediaURL, true);
		if (pMediaSource == NULL)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ,媒体源 %s 不存在 , nClient = %llu ", this, m_szShareMediaURL,nClient);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		//记下媒体源
		SplitterAppStream(m_szShareMediaURL);
		sprintf(m_addStreamProxyStruct.url, "rtsp://localhost:%d/%s/%s", ABL_MediaServerPort.nRtspPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);

		//从媒体源拷贝媒体信息
		strcpy(sdpContent.szVideoName, pMediaSource->m_mediaCodecInfo.szVideoName);
		strcpy(sdpContent.szAudioName, pMediaSource->m_mediaCodecInfo.szAudioName);
		sdpContent.nChannels = pMediaSource->m_mediaCodecInfo.nChannels;
		sdpContent.nSampleRate = pMediaSource->m_mediaCodecInfo.nSampleRate;

		//拷贝媒体源信息
		memcpy((char*)&mediaCodecInfo, (char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));

		if (GetRtspSDPFromMediaStreamSource(sdpContent, false) == false)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X ,创建 SDP 失败 , nClient = %llu ", this, nClient);

			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%d}", IndexApiCode_RtspSDPError, "rtsp push Error ", 0);
			ResponseHttp(nClient_http, szResponseBody, false);

			DeleteNetRevcBaseClient(nClient);
			return;
		}

		SendANNOUNCE(AuthenticateType);
   	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_ANNOUNCE && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		GetFieldValue("Session", szSessionID);
		if (strlen(szSessionID) == 0)
		{//有些服务器检查Session,如果不对，会立刻关闭
			string strSessionID;
			char   szTempSessionID[128] = { 0 };
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
		SendSetup(AuthenticateType);
 	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_SETUP && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		if (nMediaCount == 1)
		{//只有1个媒体，直接发送Player
			SendRECORD(AuthenticateType);
		}
		else if (nMediaCount == 2)
		{
			if (nSendSetupCount == 1)
				SendSetup(AuthenticateType);//需要再发送
			else
			{
				SendRECORD(AuthenticateType);
			}
		}

		nRecvLength = nHttpHeadEndLength = 0;
   	}
	else if (memcmp(data_, "RTSP/1.0 200", 12) == 0 && nRtspProcessStep == RtspProcessStep_RECORD && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "收到 RECORD 回复命令，rtsp交互完毕 nClient = %llu ", nClient);
		auto  pMediaSource = GetMediaStreamSource(m_szShareMediaURL, true);
		if (pMediaSource == NULL )
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp = %X nClient = %llu ,不存在媒体源 %s", this, nClient, m_szShareMediaURL);
			DeleteNetRevcBaseClient(nClient);
			return ;
		}
		m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
		m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);
		pMediaSource->AddClientToMap(nClient);

		bUpdateVideoFrameSpeedFlag = true; //代表成功交互

		//回复http请求
		sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", hParent);
		ResponseHttp(nClient_http, szResponseBody, false);

		//在父类标记成功
		auto   pParentPtr = GetNetRevcBaseClient(hParent);
		if (pParentPtr && pParentPtr->bProxySuccessFlag == false)
			bProxySuccessFlag = pParentPtr->bProxySuccessFlag = true;

 	}
	else if (memcmp(pRecvData, "TEARDOWN", 8) == 0 && strstr((char*)pRecvData, "\r\n\r\n") != NULL)
	{
		WriteLog(Log_Debug, "收到 TEARDOWN 命令，立即执行删除 nClient = %llu ", nClient);
		bRunFlag = false;
		DeleteNetRevcBaseClient(nClient);
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
		//回复http请求
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s\",\"key\":%d}", IndexApiCode_RtspSDPError, pRecvData, 0);
		ResponseHttp(nClient_http, szResponseBody, false);

		bIsInvalidConnectFlag = true; //确认为非法连接 
		WriteLog(Log_Debug, "非法的rtsp 命令，立即执行删除 nClient = %llu , \r\n%s ",nClient, pRecvData);

		//删除掉代理拉流、推流
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


//获取认证方式
bool  CNetClientSendRtsp::GetWWW_Authenticate()
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

//处理网络数据
int CNetClientSendRtsp::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(netDataLock);

	bExitProcessFlagArray[2] = false; 
	tRtspProcessStartTime = GetTickCount64();
	while (!bIsInvalidConnectFlag && bRunFlag && netDataCacheLength > 4)
	{
	    uint32_t nReadLength = 4;
		data_Length = 0;

		int nRet = XHNetSDKRead(nClient, data_ + data_Length, &nReadLength, true, true);
		if (nRet == 0 && nReadLength == 4)
		{
			data_Length = 4;
			if (data_[0] == '$')
			{//rtp 数据
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
					{//收到RTCP包，需要回复rtcp报告包
						SendRtcpReportData(nVideoSSRC, data_[1]);
						//WriteLog(Log_Debug, "this =%X ,收到 视频 的RTCP包，需要回复rtcp报告包，netBaseNetType = %d  收到RCP包长度 = %d ", this, netBaseNetType, nReadLength);
					}
					else if (data_[1] == 0x03)
					{//收到RTCP包，需要回复rtcp报告包
						SendRtcpReportData(nVideoSSRC+1, data_[1]);
						//WriteLog(Log_Debug, "this =%X ,收到 音频 的RTCP包，需要回复rtcp报告包，netBaseNetType = %d  收到RCP包长度 = %d ", this, netBaseNetType, nReadLength);
					}
				}
				else
				{
					WriteLog(Log_Debug, "ReadDataFunc() ,尚未读取到rtp数据 ! ABLRtspChan = %llu ", nClient);
					bExitProcessFlagArray[2] = true;
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return -1;
				}
			}
			else
			{//rtsp 数据
				if (!ReadRtspEnd())
				{
					WriteLog(Log_Debug, "ReadDataFunc() ,尚未读取到rtsp (1)数据 ! nClient = %llu ", nClient);
					bExitProcessFlagArray[2] = true;
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
 					return  -1;
				}
				else
				{
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
			WriteLog(Log_Debug, "CNetClientSendRtsp= %X  , ProcessNetData() ,尚未读取到数据 ! , nClient = %llu", this, nClient);
			bExitProcessFlagArray[2] = true;
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		if (GetTickCount64() - tRtspProcessStartTime > 16000)
		{
			WriteLog(Log_Debug, "CNetClientSendRtsp= %X  , ProcessNetData() ,RTSP 网络处理超时 ! , nClient = %llu", this, nClient);
			bExitProcessFlagArray[2] = true;
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}
	}

	bExitProcessFlagArray[2] = true;
	return 0;
}

//发送rtcp 报告包
void  CNetClientSendRtsp::SendRtcpReportData(unsigned int nSSRC, int nChan)
{
	memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
	rtcpSR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

	ProcessRtcpData((char*)szRtcpSRBuffer, rtcpSRBufferLength, nChan);
}

//发送rtcp 报告包 接收端
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
	XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 4, 1);
}

//发送第一个请求
int CNetClientSendRtsp::SendFirstRequst()
{
	SendOptions(AuthenticateType);
 	return 0;
}

//请求m3u8文件
bool  CNetClientSendRtsp::RequestM3u8File()
{
	return true;
}

//从 SDP中获取  SPS，PPS 信息
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

void  CNetClientSendRtsp::UserPasswordBase64(char* szUserPwdBase64)
{
	char szTemp[string_length_2048] = { 0 };
	sprintf(szTemp, "%s:%s", m_rtspStruct.szUser, m_rtspStruct.szPwd);
	Base64Encode((unsigned char*)szUserPwdBase64, (unsigned char*)szTemp, strlen(szTemp));
}

//确定SDP里面视频，音频的总媒体数量, 从Describe中找到 trackID，大华的从0开始，海康，华为的从1开始
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
				{//没有rtsp类似的地址，比如 trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//有rtsp类似的地址，比如海康的摄像头 a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
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
				{//没有rtsp类似的地址，比如 trackID=0,trackID=1,trackID=2
					strcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID);
				}
				else
				{//有rtsp类似的地址，比如海康的摄像头 a=control:rtsp://admin:abldyjh2019@192.168.1.109:554/trackID=1 
					memcpy(szTrackIDArray[nTrackIDOrer], szTempTraceID + nPos4 + 1, strlen(szTempTraceID) - nPos4 - 1);
				}
				nTrackIDOrer++;
			}
		}
	}
}

void  CNetClientSendRtsp::SendOptions(WWW_AuthenticateType wwwType)
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
		szResponse = (char*)author.computeDigestResponse("DESCRIBE", m_rtspStruct.szSrcRtspPullUrl); //要注意 uri ,有时候没有最后的 斜杠 /

		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);

	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);
		sprintf(szResponseBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nAuthorization: Basic %s\r\nAccept: application/sdp\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szBasic);
	}

	strcat(szResponseBuffer, szRtspSDPContent);
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);
	nRtspProcessStep = RtspProcessStep_ANNOUNCE;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}

/*
要优化，有些摄像机 {trackID=1 、 trackID=2} ，有些摄像机是，比如大华 {trackID=0、trackID=1}
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
		szResponse = (char*)author.computeDigestResponse("SETUP", m_rtspStruct.szSrcRtspPullUrl); //要注意 uri ,有时候没有最后的 斜杠 /

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

	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);

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
		szResponse = (char*)author.computeDigestResponse("PLAY", m_rtspStruct.szSrcRtspPullUrl); //要注意 uri ,有时候没有最后的 斜杠 /

 		sprintf(szResponseBuffer, "RECORD %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szSessionID, m_rtspStruct.szUser, m_rtspStruct.szRealm, m_rtspStruct.szNonce, m_rtspStruct.szSrcRtspPullUrl, szResponse);

		author.reclaimDigestResponse(szResponse);
	}
	else if (wwwType == WWW_Authenticate_Basic)
	{
		UserPasswordBase64(szBasic);

 		sprintf(szResponseBuffer, "RECORD %s RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: ABL_RtspServer_3.0.1\r\nSession: %s\r\nAuthorization: Basic %s\r\nRange: npt=0.000-\r\n\r\n", m_rtspStruct.szSrcRtspPullUrl, CSeq, szSessionID, szBasic);
	}
	XHNetSDK_Write(nClient, (unsigned char*)szResponseBuffer, strlen(szResponseBuffer), 1);

	nRtspProcessStep = RtspProcessStep_RECORD;

	WriteLog(Log_Debug, "\r\n%s", szResponseBuffer);

	CSeq++;
}
