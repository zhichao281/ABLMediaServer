/*
功能：
    接收TS流，形成媒体源，用于接收推送ts流
 	 
日期    2022-07-12
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "RtpTSStreamInput.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;
#endif


extern CMediaFifo                            pDisconnectBaseNetFifo;       //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256];   //当前路径
extern int                                   avpriv_mpeg4audio_sample_rates[];

static int on_ts_packet(void* param, int program, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	CRtpTSStreamInput* pThis = (CRtpTSStreamInput*)param;
	int count = 0 , len;

	if (pThis == NULL || pThis->pMediaSource == NULL)
		return -1;
	if (!pThis->bRunFlag)
		return -1;

	if (PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_OPUS == avtype)
	{
		len = mpeg4_aac_adts_frame_length((const uint8_t*)data, bytes);
		while (len > 7 && (size_t)len <= bytes)
		{
			if(pThis->aacInfo.channels == 0)
				mpeg4_aac_adts_load((unsigned char*)data, len, &pThis->aacInfo);

			count ++;
			if(pThis->aacInfo.channels > 0 && pThis->aacInfo.sampling_frequency_index >= 0 && pThis->aacInfo.sampling_frequency_index <= 12)
			  pThis->pMediaSource->PushAudio((unsigned char*)data, len, "AAC", pThis->aacInfo.channels, avpriv_mpeg4audio_sample_rates[pThis->aacInfo.sampling_frequency_index]);

			bytes -= len;
			data = (const uint8_t*)data + len;
			len = mpeg4_aac_adts_frame_length((const uint8_t*)data, bytes);
		}
	}else if (PSI_STREAM_AUDIO_G711A == avtype)
	{
		len = 5;
 	}
	else if (PSI_STREAM_AUDIO_G711U == avtype)
	{
		len = 6;
	}
	else if (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype)
	{
		if (PSI_STREAM_H264 == avtype)
			pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
		else
			pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
	}
	else
	{
 
	}
	return 0;
}

static void mpeg_ts_dec_testonstream(void* param, int stream, int codecid, const void* extra, int bytes, int finish)
{
	//printf("stream %d, codecid: %d, finish: %s\n", stream, codecid, finish ? "true" : "false");
}
struct ts_demuxer_notify_t notify_RtpTSStream = {
	mpeg_ts_dec_testonstream,
};

CRtpTSStreamInput::CRtpTSStreamInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
 	netBaseNetType = NetBaseNetType_NetGB28181UDPTSStreamInput;
	strcpy(m_szShareMediaURL, szShareMediaURL);
	nClient = hClient;
	bRunFlag = true;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;

	ts = ts_demuxer_create(on_ts_packet, this);
	if(ts)
	  ts_demuxer_set_notify(ts, &notify_RtpTSStream, this);
	pMediaSource = NULL;
	SplitterAppStream(m_szShareMediaURL);
	sprintf(m_addStreamProxyStruct.url, "rtp://%s:%d%s",szIP,nPort,szShareMediaURL);

	memset((char*)&aacInfo, 0x00, sizeof(aacInfo));
	WriteLog(Log_Debug, "CRtpTSStreamInput 构造 = %X  nClient = %llu ,m_szShareMediaURL = %s ", this, nClient,m_szShareMediaURL);
}

CRtpTSStreamInput::~CRtpTSStreamInput()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(tsRecvLock);

	if (ts)
	{
		ts_demuxer_flush(ts);
		ts_demuxer_destroy(ts);
		ts = NULL;
     }
	 m_videoFifo.FreeFifo();

	//删除分发源
	if (strlen(m_szShareMediaURL) > 0)
		DeleteMediaStreamSource(m_szShareMediaURL);

	WriteLog(Log_Debug, "CRtpTSStreamInput 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CRtpTSStreamInput::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CRtpTSStreamInput::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CRtpTSStreamInput::SendVideo()
{
	return 0;
}

int CRtpTSStreamInput::SendAudio()
{

	return 0;
}

int CRtpTSStreamInput::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag)
		return -1;
	std::lock_guard<std::mutex> lock(tsRecvLock);

	if (nDataLength < 12 || (nDataLength - 12) % 188 != 0 || !bRunFlag)
		return -1;//数据长度非法

	nRecvDataTimerBySecond = 0;

	if (pMediaSource == NULL)
	{
		pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
		if (pMediaSource == NULL)
		{
			bRunFlag = false;
			WriteLog(Log_Debug, "CRtpTSStreamInput 构造 = %X 创建媒体源失败  nClient = %llu ,m_szShareMediaURL = %s ", this, nClient, m_szShareMediaURL);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1 ;
		}
		pMediaSource->netBaseNetType = NetBaseNetType_NetGB28181UDPTSStreamInput;//指定为TS流接入
	}
	else if (pMediaSource)
	{
		if (!pMediaSource->bUpdateVideoSpeed)
		{
			int nSpeed = CalcVideoFrameSpeed(pData, nDataLength);
			if (nSpeed > 0)
			{
				pMediaSource->UpdateVideoFrameSpeed(nSpeed, netBaseNetType);
				WriteLog(Log_Debug, "CRtpTSStreamInput = %X  nClient = %llu , m_szShareMediaURL = %s,更新视频帧速度 = %d", this, nClient, m_szShareMediaURL, nSpeed);
 			}
		}
	}

	int nSize = (nDataLength - 12) / 188;
	int nPos = 0;
	for(int i=0;i<nSize;i++)
	{
	   ts_demuxer_input(ts, pData + (12 + nPos), 188);
	   nPos += 188;
	}

    return 0;
}

int CRtpTSStreamInput::ProcessNetData()
{
  
 	return 0;
}

//发送第一个请求
int CRtpTSStreamInput::SendFirstRequst()
{
  	 return 0;
}

//请求m3u8文件
bool  CRtpTSStreamInput::RequestM3u8File()
{
	return true;
}