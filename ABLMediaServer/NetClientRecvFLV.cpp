/*
功能：
    实现flv客户端的接收模块   
	 
日期    2021-07-19
作者    罗家兄弟
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


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern int                                   SampleRateArray[] ;
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径

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
	if (pClient == NULL || pClient->pMediaSource == NULL || !pClient->bRunFlag)
		return 0;

	if (FLV_AUDIO_AAC == codec)
	{
		a_pts = pts;
		a_dts = dts;

		if (strlen(pClient->pMediaSource->m_mediaCodecInfo.szAudioName) == 0 && bytes > 4 && data != NULL)
		{
			unsigned char* pAudioData = (unsigned char*)data;
			strcpy(pClient->pMediaSource->m_mediaCodecInfo.szAudioName, "AAC");

			//采样频率序号只占4位，  8 7 6 5 4 3 2 1  在 6 ~ 3 位，共4个位。所以要和0x3c 与运算，把别的位全部置为0 ，再往右移动2位，
			unsigned char nSampleIndex = ((pAudioData[2] & 0x3c) >> 2) & 0x0F;  //从 pb[2] 中获取采样频率的序号
			if (nSampleIndex <= 12)
				pClient->pMediaSource->m_mediaCodecInfo.nSampleRate = SampleRateArray[nSampleIndex];

			//通道数量计算 pAVData[2]  中有2个位，在最后2位，根 0x03 与运算，得到两位，左移动2位 ，再 或 上 pAVData[3] 的左边最高2位
			//pAVData[3] 左边最高2位获取方法 先 和 0xc0 与运算，再右移6位，为什么要右移6位？因为这2位是在最高位，所以要往右边移动6位
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
		//WriteLog(Log_Debug, "CNetClientRecvRtmp=%X ,nClient = %llu, rtmp 解包回调 %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X , timeStamp = %d ,datasize = %d ", pClient, pClient->nClient, (unsigned char*)pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4], pVideoData[5], pVideoData[6], pVideoData[7], pVideoData[8], pVideoData[9], pVideoData[10], pVideoData[11], pVideoData[12],dts,bytes);
		if (!pClient->bUpdateVideoFrameSpeedFlag)
		{//更新视频源的帧速度
			int nVideoSpeed = pClient->CalcFlvVideoFrameSpeed(pts,1000);
			if (nVideoSpeed > 0 && pClient->pMediaSource != NULL)
			{
				pClient->bUpdateVideoFrameSpeedFlag = true;
				WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ", pClient->nClient, pClient->pMediaSource->m_szURL, pClient->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
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
		//WriteLog(Log_Debug, "CNetRtspServer=%X ,nClient = %llu, rtmp 解包回调 %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X , timeStamp = %d ,datasize = %d ", pClient, pClient->nClient, (unsigned char*)pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4], pVideoData[5], pVideoData[6], pVideoData[7], pVideoData[8], pVideoData[9], pVideoData[10], pVideoData[11], pVideoData[12],dts,bytes);

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
		uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);

	nVideoDTS = 0;
	nAudioDTS = 0;
	memset(szRtmpName, 0x00, sizeof(szRtmpName));
	reader = NULL;
	netDataCacheLength = nNetStart = nNetEnd = 0;
	netBaseNetType = NetBaseNetType_HttpFlvClientRecv;

	WriteLog(Log_Debug, "CNetClientRecvFLV 构造 = %X nClient = %llu ", this, nClient);
}

CNetClientRecvFLV::~CNetClientRecvFLV()
{
	bRunFlag = false ;
	std::lock_guard<std::mutex> lock(NetClientRecvFLVLock);

	//服务器异常断开
	if (bUpdateVideoFrameSpeedFlag == false)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"faied. Abnormal didconnection \",\"key\":%llu}", IndexApiCode_RecvRtmpFailed, 0);
		ResponseHttp2(nClient_http, szResponseBody, false);
	}

	if (reader)
		flv_reader_destroy(reader);

	if (flvDemuxer)
		flv_demuxer_destroy(flvDemuxer);

	 //如果是接收推流，并且成功接收推流的，则需要删除媒体数据源 szURL ，比如 /Media/Camera_00001 
	if(strlen(m_szShareMediaURL) >0 && pMediaSource != NULL )
   	  DeleteMediaStreamSource(m_szShareMediaURL);

#ifdef  SaveNetDataToFlvFile
	if (fileFLV != NULL)
		fclose(fileFLV);
#endif

#ifdef  WriteHTTPFlvToEsFileFlag
	 fclose(fWriteVideo); 
#endif

	WriteLog(Log_Debug, "CNetClientRecvFLV 析构 = %X nClient = %llu \r\n", this, nClient);
	
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
	if(!bRunFlag)
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
	{//去掉http回复的包头
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

			if (HttpFlvReadPacketSize - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetClientRecvFLV = %X nClient = %llu 数据异常 , 执行删除", this, nClient);
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

    return 0;
}

//模拟文件读取回调函数 
static int http_flv_netRead(void* param, void* buf, int len)
{
	CNetClientRecvFLV* pHttpFlv = (CNetClientRecvFLV*)param;
	if (pHttpFlv == NULL || !pHttpFlv->bRunFlag)
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
    if(!bRunFlag)
		return -1;
 
	if (netDataCacheLength > (1024 * 1024 * 1.256) )
	{//缓存1.256 M数据
 		if (reader == NULL)
 		   reader = flv_reader_create2(http_flv_netRead, this);
	   
		 //创建失败 
		if (reader == NULL)
		{
			bRunFlag = false;
			WriteLog(Log_Debug, "CNetClientRecvFLV = %X nClient = %llu flv_reader_create2 创建失败 ,执行删除 ", this,nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		}
		
 		while (flv_reader_read(reader, &type, &timestamp, &taglen, packet, sizeof(packet)) == 1 )
		{//当剩余 1024 * 1024 时，需要退出 
			flv_demuxer_input(flvDemuxer, type, packet, taglen, timestamp);

			//要剩余些数据，否则当读取到包头时，包头所指的数据长度 大于 缓冲区剩余的数据，就会报错 ,最好大于i帧的长度
			if (netDataCacheLength < 1024 * 768)
				break;
 		}
	}

	return 0;
}

//发送第一个请求
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

			//创建媒体分发资源
			if (strlen(m_szShareMediaURL) > 0)
			{
				pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hParent, MediaSourceType_LiveMedia,0, m_h265ConvertH264Struct);
				if (pMediaSource)
				{
					pMediaSource->netBaseNetType = netBaseNetType;
					pMediaSource->enable_mp4 = (strcmp(m_addStreamProxyStruct.enable_mp4, "1") == 0) ? true : false;
					pMediaSource->enable_hls = (strcmp(m_addStreamProxyStruct.enable_hls, "1") == 0) ? true : false;
				}
				else
				{
					DeleteNetRevcBaseClient(nClient);
					return -1;
				}
  			}

			memcpy(szSubPath, m_rtspStruct.szSrcRtspPullUrl + nPos2, strlen(m_rtspStruct.szSrcRtspPullUrl) - nPos2);
			sprintf(szRequestFLVFile, "GET %s HTTP/1.1\r\nUser-Agent: %s\r\nAccept: */*\r\nRange: bytes=0-\r\nConnection: keep-alive\r\nHost: 190.15.240.11:8088\r\nIcy-MetaData: 1\r\n\r\n", szSubPath, MediaServerVerson);
			XHNetSDK_Write(nClient, (unsigned char*)szRequestFLVFile, strlen(szRequestFLVFile), 1);
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

//请求m3u8文件
bool  CNetClientRecvFLV::RequestM3u8File()
{
	return true;
}