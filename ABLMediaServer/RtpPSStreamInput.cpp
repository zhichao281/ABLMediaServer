/*
功能：
    接收ps流，形成媒体源，用于接收推送ps流
 	 
日期    2022-07-12
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "RtpPSStreamInput.h"
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
extern int                                   SampleRateArray[];

void RTP_DEPACKET_CALL_METHOD RTP10000_rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	CRtpPSStreamInput* pThis = (CRtpPSStreamInput*)cb->userdata;
 
	if (pThis != NULL && pThis->bRunFlag)
	{
		if (pThis->nSSRC == 0)
			pThis->nSSRC = cb->ssrc; //默认第一个ssrc 
		if (pThis->nSSRC == cb->ssrc)
		{
			if (ABL_MediaServerPort.gb28181LibraryUse == 1)
			{//自研
				ps_demux_input(pThis->psDeMuxHandle, cb->data, cb->datasize);
			}
			else
			{//北京老陈
				if (pThis->psBeiJingLaoChen)
				  ps_demuxer_input(pThis->psBeiJingLaoChen, cb->data, cb->datasize);
			}
		}
	}
}

void PS_DEMUX_CALL_METHOD RTP10000_RtpRecv_demux_callback(_ps_demux_cb* cb)
{
	CRtpPSStreamInput* pThis = (CRtpPSStreamInput*)cb->userdata;
	if (!pThis->bRunFlag)
		return;

	if (pThis && cb->streamtype == e_rtpdepkt_st_h264 || cb->streamtype == e_rtpdepkt_st_h265 ||
		cb->streamtype == e_rtpdepkt_st_mpeg4 || cb->streamtype == e_rtpdepkt_st_mjpeg)
	{
		if (cb->streamtype == e_rtpdepkt_st_h264)
			pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
		else if (cb->streamtype == e_rtpdepkt_st_h265)
			pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");

		if (!pThis->bUpdateVideoFrameSpeedFlag)
		{//更新视频源的帧速度
			int nVideoSpeed = pThis->CalcFlvVideoFrameSpeed(cb->pts, 90000);
			if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
			{
				pThis->pMediaSource->netBaseNetType = NetBaseNetType_NetGB28181UDPPSStreamInput;//指定为PS流接入
				pThis->bUpdateVideoFrameSpeedFlag = true;

				if (pThis->pMediaSource)
					pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//记录是否录像

				WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
			}
		}
	}
	else if (pThis)
	{
		if (cb->streamtype == e_rtpdepkt_st_aac)
		{//aac
			if(pThis->nRecvSampleRate == 0 &&  pThis->nRecvChannels == 0 )
			  pThis->GetAACAudioInfo2(cb->data, cb->datasize, &pThis->nRecvSampleRate,&pThis->nRecvChannels);//获取AAC媒体信息
			if (pThis->nRecvSampleRate > 0 && pThis->nRecvChannels > 0)
			  pThis->pMediaSource->PushAudio(cb->data, cb->datasize,"AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
		}
		else if (cb->streamtype == e_rtpdepkt_st_g711a)
		{// G711A  
			pThis->pMediaSource->PushAudio(cb->data, cb->datasize, "G711_A", 1, 8000);
		}
		else if (cb->streamtype == e_rtpdepkt_st_g711u)
		{// G711U  
			pThis->pMediaSource->PushAudio(cb->data, cb->datasize, "G711_U", 1, 8000);
		}
	}
}

static int on_gb28181_10000_unpacket(void* param, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	CRtpPSStreamInput* pThis = (CRtpPSStreamInput*)param;
	if (!pThis->bRunFlag || pThis->pMediaSource  == NULL )
		return -1;

	if (PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_G711A == avtype || PSI_STREAM_AUDIO_G711U == avtype)
	{
		if (PSI_STREAM_AAC == avtype)
		{//aac
 			if (pThis->nRecvSampleRate == 0 && pThis->nRecvChannels == 0)
				pThis->GetAACAudioInfo2((unsigned char*)data, bytes, &pThis->nRecvSampleRate, &pThis->nRecvChannels);//获取AAC媒体信息

			if(pThis->nRecvSampleRate > 0 && pThis->nRecvChannels > 0)
			  pThis->pMediaSource->PushAudio((unsigned char*)data, bytes,"AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
		}
		else if (PSI_STREAM_AUDIO_G711A == avtype)
		{// G711A  
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_A", 1, 8000);
		}
		else if (PSI_STREAM_AUDIO_G711U == avtype)
		{// G711U  
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_U", 1, 8000);
		}

		if (!pThis->bUpdateVideoFrameSpeedFlag)
		{//更新视频源的帧速度
			int nVideoSpeed = 25 ;
			if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
			{
				pThis->pMediaSource->netBaseNetType = NetBaseNetType_NetGB28181UDPPSStreamInput;//指定为PS流接入
				pThis->bUpdateVideoFrameSpeedFlag = true;

				if (pThis->pMediaSource)
					pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//记录是否录像

				WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
			}
		}
	}
	else if (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype || PSI_STREAM_VIDEO_SVAC == avtype)
	{
		if (PSI_STREAM_H264 == avtype)
			pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
		else if (PSI_STREAM_H265 == avtype)
			pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H265");

		if (!pThis->bUpdateVideoFrameSpeedFlag)
		{//更新视频源的帧速度
			int nVideoSpeed = pThis->CalcFlvVideoFrameSpeed(pts, 90000);
			if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
			{
				pThis->pMediaSource->netBaseNetType = NetBaseNetType_NetGB28181UDPPSStreamInput;//指定为PS流接入
				pThis->bUpdateVideoFrameSpeedFlag = true;

				if (pThis->pMediaSource)
					pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//记录是否录像

				WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
			}
		}
	}
 
	return 0;
}
static void mpeg_ps_dec_testonstream_10000(void* param, int stream, int codecid, const void* extra, int bytes, int finish)
{
	printf("stream %d, codecid: %d, finish: %s\n", stream, codecid, finish ? "true" : "false");
}

CRtpPSStreamInput::CRtpPSStreamInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	nRecvSampleRate = nRecvChannels = 0;
 	netBaseNetType = NetBaseNetType_NetGB28181UDPPSStreamInput;
	strcpy(m_szShareMediaURL, szShareMediaURL);
	nClient = hClient;
	strcpy(szClientIP, szIP);
	bRunFlag = true;
	psBeiJingLaoChen = NULL;

	m_gbPayload = 96;
	SplitterAppStream(m_szShareMediaURL);
	sprintf(m_addStreamProxyStruct.url,"rtp://%s:%d%s",szIP,nPort,szShareMediaURL);
	pMediaSource = NULL; 
	hRtpHandle = 0;

	WriteLog(Log_Debug, "CRtpPSStreamInput 构造 = %X  nClient = %llu ,m_szShareMediaURL = %s ", this, nClient,m_szShareMediaURL);
}

CRtpPSStreamInput::~CRtpPSStreamInput()
{
	std::lock_guard<std::mutex> lock(psRecvLock);
	bRunFlag = false;

	m_videoFifo.FreeFifo();

	if (psDeMuxHandle > 0)
	{
		ps_demux_stop(psDeMuxHandle);
		psDeMuxHandle = 0;
	}

	if (psBeiJingLaoChen != NULL)
	{
		ps_demuxer_destroy(psBeiJingLaoChen);
		psBeiJingLaoChen = NULL;
	}

	if (hRtpHandle > 0)
	{
		rtp_depacket_stop(hRtpHandle);
		hRtpHandle = 0;
	}

	//删除分发源
	if (strlen(m_szShareMediaURL) > 0)
		DeleteMediaStreamSource(m_szShareMediaURL);

	WriteLog(Log_Debug, "CRtpPSStreamInput 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CRtpPSStreamInput::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CRtpPSStreamInput::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CRtpPSStreamInput::SendVideo()
{
	return 0;
}

int CRtpPSStreamInput::SendAudio()
{

	return 0;
}

//rtp 解包
struct ps_demuxer_notify_t notify_10000 = { mpeg_ps_dec_testonstream_10000, };

int CRtpPSStreamInput::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	std::lock_guard<std::mutex> lock(psRecvLock);
	if (!bRunFlag)
		return -1;
	nRecvDataTimerBySecond = 0;

	if (pMediaSource == NULL)
	{
		pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
		if (pMediaSource == NULL)
		{
			bRunFlag = false;
			WriteLog(Log_Debug, "CRtpPSStreamInput 构造 = %X 创建媒体源失败  nClient = %llu ,m_szShareMediaURL = %s ", this, nClient, m_szShareMediaURL);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1 ;
		}
		pMediaSource->netBaseNetType = NetBaseNetType_NetGB28181UDPPSStreamInput;//指定为PS流接入
 	}
 
	if (hRtpHandle == 0)
	{
		rtp_depacket_start(RTP10000_rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle);
		rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_gbps);
 
		WriteLog(Log_Debug, "CRtpPSStreamInput = %X ,创建rtp解包 hRtpHandle = %d ,psDeMuxHandle = %d", this, hRtpHandle, psDeMuxHandle);
	}

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//自研
		if (psDeMuxHandle == 0)
			ps_demux_start(RTP10000_RtpRecv_demux_callback, (void*)this, e_ps_dux_timestamp, &psDeMuxHandle);
	}
	else
	{//北京老陈
		if (psBeiJingLaoChen == NULL)
		{
			psBeiJingLaoChen = ps_demuxer_create(on_gb28181_10000_unpacket, this);
			if (psBeiJingLaoChen != NULL)
				ps_demuxer_set_notify(psBeiJingLaoChen, &notify_10000, this);
		}
	}

	if (hRtpHandle > 0)
		rtp_depacket_input(hRtpHandle, pData, nDataLength);
  
    return 0;
}

int CRtpPSStreamInput::ProcessNetData()
{
  
 	return 0;
}

//发送第一个请求
int CRtpPSStreamInput::SendFirstRequst()
{
  	 return 0;
}

//请求m3u8文件
bool  CRtpPSStreamInput::RequestM3u8File()
{
	return true;
}