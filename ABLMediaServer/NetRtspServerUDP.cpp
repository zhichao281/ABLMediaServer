/*
功能：
    负责rtsp推流接入udp方式的视频、音频解包
	 
日期    2024-02-14
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetRtspServerUDP.h"


#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);


#endif

//rtp解码
void RTP_DEPACKET_CALL_METHOD NetRtspServerUDP_rtpdepacket_callback(_rtp_depacket_cb* cb)
{
 	CNetRtspServerUDP* pUserHandle = (CNetRtspServerUDP*)cb->userdata;

	if (!pUserHandle->bRunFlag)
		return;

	if (pUserHandle != NULL)
	{
		if (cb->handle == pUserHandle->hRtpHandle[0])
		{
			if (pUserHandle->m_bHaveSPSPPSFlag && pUserHandle->m_nSpsPPSLength > 0)
			{
				if (pUserHandle->CheckVideoIsIFrame(pUserHandle->szVideoName, cb->data, cb->datasize))
				{
					memmove(cb->data + pUserHandle->m_nSpsPPSLength, cb->data, cb->datasize);
					memcpy(cb->data, pUserHandle->m_pSpsPPSBuffer, pUserHandle->m_nSpsPPSLength);
					pUserHandle->pMediaSource->PushVideo(cb->data, cb->datasize + pUserHandle->m_nSpsPPSLength, pUserHandle->szVideoName);
				}
				else
					pUserHandle->pMediaSource->PushVideo(cb->data, cb->datasize, pUserHandle->szVideoName);
			}
			else
				pUserHandle->pMediaSource->PushVideo(cb->data, cb->datasize, pUserHandle->szVideoName); 
#ifdef WriteRtpDecodeFile
			if (pUserHandle->fWriteVideoFile != NULL)
			{
			   fwrite(cb->data, 1, cb->datasize, pUserHandle->fWriteVideoFile);
			   fflush(pUserHandle->fWriteVideoFile);
 			}
#endif
		}
		else if (cb->handle == pUserHandle->hRtpHandle[1])
		{
			if (strcmp(pUserHandle->szAudioName, "AAC") == 0)
			{//aac
				pUserHandle->SplitterRtpAACData(cb->data, cb->datasize);
			}
			else if (strcmp(pUserHandle->szAudioName, "MP3") == 0 && cb->datasize > 4)
			{//mp3
				if (cb->data[0] == 0x00 && cb->data[1] == 0x00 && cb->data[2] == 0x00 && cb->data[3] == 0x00)
					pUserHandle->SplitterMp3Buffer(cb->data + 4, cb->datasize - 4);
				else
					pUserHandle->SplitterMp3Buffer(cb->data, cb->datasize);
		    }
			else
			{// G711A 、G711U
				pUserHandle->pMediaSource->PushAudio(cb->data, cb->datasize, pUserHandle->szAudioName, pUserHandle->nChannels, pUserHandle->nSampleRate);
			}
#ifdef WriteRtpDecodeFile
			if (pUserHandle->fWriteAudioFile != NULL)
			{
				fwrite(cb->data, 1, cb->datasize, pUserHandle->fWriteAudioFile);
				fflush(pUserHandle->fWriteAudioFile);
			}
#endif
		}
	}
}

CNetRtspServerUDP::CNetRtspServerUDP(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	bRunFlag = true;
	memset(szFullMp3Buffer, 0x00, sizeof(szFullMp3Buffer));
	memset(szMp3HeadFlag, 0x00, sizeof(szMp3HeadFlag));
	szMp3HeadFlag[0] = 0xFF;
	szMp3HeadFlag[1] = 0xFB;
	hRtpHandle[0] = hRtpHandle[1] = 0;
	memset(szVideoName, 0x00, sizeof(szVideoName));
	memset(szAudioName, 0x00, sizeof(szAudioName));
	nVideoPayload = 0;
	nAudioPayload = 0;
	sample_index = 0; 
	nChannels = 0; 
	nSampleRate = 0;  

	strcpy(m_szShareMediaURL,szShareMediaURL);
	nMediaClient = 0;
	nClient = hClient;
	nCreateDateTime = GetTickCount64();
#ifdef WriteRtpDecodeFile
	char szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X_%d.264", ABL_MediaSeverRunPath, this,rand());
	fWriteVideoFile = fopen(szFileName,"wb");
	sprintf(szFileName, "%s%X_%d.g711a", ABL_MediaSeverRunPath, this,rand());
	fWriteAudioFile = fopen(szFileName, "wb");
#endif
	WriteLog(Log_Debug, "CNetRtspServerUDP 构造 = %X  nClient = %llu ", this, nClient);
}

CNetRtspServerUDP::~CNetRtspServerUDP()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if(netBaseNetType == NetBaseNetType_RtspServerRecvPushVideo)
 	   rtp_depacket_stop(hRtpHandle[0]);
	else  if (netBaseNetType == NetBaseNetType_RtspServerRecvPushAudio)
	   rtp_depacket_stop(hRtpHandle[1]);

#ifdef WriteRtpDecodeFile
 	fclose(fWriteVideoFile);
 	fclose(fWriteAudioFile);
#endif
	WriteLog(Log_Debug, "CNetRtspServerUDP 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
	pDisconnectBaseNetFifo.push((unsigned char*)&hParent,sizeof(hParent));
}

int CNetRtspServerUDP::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetRtspServerUDP::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetRtspServerUDP::SendVideo()
{
	return 0;
}

int CNetRtspServerUDP::SendAudio()
{

	return 0;
}

int CNetRtspServerUDP::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	nRecvDataTimerBySecond = 0;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (netBaseNetType == NetBaseNetType_RtspServerRecvPushVideo)
	{
		if (!bUpdateVideoFrameSpeedFlag)
		{//更新视频源的帧速度
			int nVideoSpeed = CalcVideoFrameSpeed(pData, nDataLength);
			if (nVideoSpeed > 0 && pMediaSource != NULL)
			{
				bUpdateVideoFrameSpeedFlag = true;
				WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ", nClient,pMediaSource->m_szURL, pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, netBaseNetType);
			}
		}
		rtp_depacket_input(hRtpHandle[0], pData, nDataLength);
	}
	else if (netBaseNetType == NetBaseNetType_RtspServerRecvPushAudio)
		rtp_depacket_input(hRtpHandle[1], pData, nDataLength);

	return 0;
}

int CNetRtspServerUDP::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetRtspServerUDP::SendFirstRequst()
{
  	 return 0;
}

//请求m3u8文件
bool  CNetRtspServerUDP::RequestM3u8File()
{
	return true;
}

#ifdef USE_BOOST
bool   CNetRtspServerUDP::CreateVideoRtpDecode(boost::shared_ptr<CMediaStreamSource> mediaServer, char* VideoName, int nVidoePT)

#else
bool   CNetRtspServerUDP::CreateVideoRtpDecode(std::shared_ptr<CMediaStreamSource> mediaServer, char* VideoName, int nVidoePT)


#endif

{
	if (strlen(VideoName) == 0 || nVidoePT <= 0 || mediaServer == NULL)
		return false;
	pMediaSource = mediaServer;
	strcpy(szVideoName, VideoName);
	nVideoPayload = nVidoePT;

	int nRet2 = 0;
	if (strlen(szVideoName) > 0)
	{
		nRet2 = rtp_depacket_start(NetRtspServerUDP_rtpdepacket_callback, (void*)this, (uint32_t*)&hRtpHandle[0]);
		if (strcmp(szVideoName, "H264") == 0)
			rtp_depacket_setpayload(hRtpHandle[0], nVideoPayload, e_rtpdepkt_st_h264);
		else if (strcmp(szVideoName, "H265") == 0)
			rtp_depacket_setpayload(hRtpHandle[0], nVideoPayload, e_rtpdepkt_st_h265);
	}

	return true;
}

#ifdef USE_BOOST
bool   CNetRtspServerUDP::CreateAudioRtpDecode(boost::shared_ptr<CMediaStreamSource> mediaServer, char* AudioName, int nAudioPT, int Channels, int SampleRate, int nSampleIndex)

#else
bool   CNetRtspServerUDP::CreateAudioRtpDecode(std::shared_ptr<CMediaStreamSource> mediaServer, char* AudioName, int nAudioPT, int Channels, int SampleRate, int nSampleIndex)


#endif

{
	if (strlen(AudioName) == 0 || mediaServer == NULL )
		return  false;
	int nRet2 = 0;
	pMediaSource = mediaServer;
	strcpy(szAudioName, AudioName);
	nAudioPayload = nAudioPT;
	nChannels = Channels;
	nSampleRate = SampleRate;
	sample_index = nSampleIndex;

	nRet2 = rtp_depacket_start(NetRtspServerUDP_rtpdepacket_callback, (void*)this, (uint32_t*)&hRtpHandle[1]);

	if (strcmp(szAudioName, "G711_A") == 0)
	{
		rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_g711a);
	}
	else if (strcmp(szAudioName, "G711_U") == 0)
	{
		rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_g711u);
	}
	else if (strcmp(szAudioName, "AAC") == 0)
	{
		rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_aac);
	}
	else if (strstr(szAudioName, "G726LE") != NULL)
	{
		rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_g726le);
	}
	else if (strstr(szAudioName, "MP3") != NULL)
	{
		rtp_depacket_setpayload(hRtpHandle[1], nAudioPayload, e_rtpdepkt_st_mp3);
	}
	return true;
}

//追加adts信息头
void  CNetRtspServerUDP::AddADTSHeadToAAC(unsigned char* szData, int nAACLength)
{
	int len = nAACLength + 7;
	uint8_t profile = 2;
	uint8_t sampling_frequency_index = sample_index;
	uint8_t channel_configuration = nChannels;
	aacData[0] = 0xFF; /* 12-syncword */
	aacData[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
	aacData[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
	aacData[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
	aacData[4] = (uint8_t)(len >> 3);
	aacData[5] = ((len & 0x07) << 5) | 0x1F;
	aacData[6] = 0xFC | ((len / 1024) & 0x03);

	memcpy(aacData + 7, szData, nAACLength);
}

//对AAC的rtp包进行切割
void  CNetRtspServerUDP::SplitterRtpAACData(unsigned char* rtpAAC, int nLength)
{
	au_header_length = (rtpAAC[0] << 8) + rtpAAC[1];
	au_header_length = (au_header_length + 7) / 8;
	ptr = rtpAAC;

	au_size = 2;
	au_numbers = au_header_length / au_size;

	//超过5帧都丢弃，有些音频乱打包
	if (au_numbers > 16)
		return;

	ptr += 2;
	pau = ptr + au_header_length;

	for (int i = 0; i < au_numbers; i++)
	{
		SplitterSize[i] = (ptr[0] << 8) | (ptr[1] & 0xF8);
		SplitterSize[i] = SplitterSize[i] >> 3;

		if (SplitterSize[i] > nLength || SplitterSize[i] <= 0)
		{
			WriteLog(Log_Debug, "CNetRtspServerUDP=%X ,nClient = %llu, rtp 切割长度 有误 ", this, nClient);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return;
		}

		AddADTSHeadToAAC((unsigned char*)pau, SplitterSize[i]);

		if (netBaseNetType == NetBaseNetType_RtspServerRecvPushAudio && pMediaSource != NULL)
		{
			pMediaSource->PushAudio(aacData, SplitterSize[i] + 7, szAudioName, nChannels, nSampleRate);
		}

		ptr += au_size;
		pau += SplitterSize[i];
	}
}

//对mp3数据包进行切割
void  CNetRtspServerUDP::SplitterMp3Buffer(unsigned char* szMp3Buffer, int nLength)
{
	if (szMp3Buffer == NULL || nLength <= 0)
		return;
	int nPos1 = -1, nPos2 = -1;

	for (int i = 0; i < nLength; i++)
	{
		if (nPos1 == -1 && nPos2 == -1)
		{
			if (memcmp(szMp3Buffer + i, szMp3HeadFlag, 2) == 0)
			{
				nPos1 = i;
			}
		}
		else if (nPos1 != -1 && nPos2 == -1)
		{
			if (memcmp(szMp3Buffer + i, szMp3HeadFlag, 2) == 0)
			{
				nPos2 = i;
				pMediaSource->PushAudio(szMp3Buffer + nPos1, nPos2 - nPos1, szAudioName, nChannels, nSampleRate);

				nPos1 = i;
				nPos2 = -1;
			}
		}
	}

	if (nPos1 != -1 && nPos2 == -1)
	{
		pMediaSource->PushAudio(szMp3Buffer + nPos1, nLength - nPos1, szAudioName, nChannels, nSampleRate);
	}
}