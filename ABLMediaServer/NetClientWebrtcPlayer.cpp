/*
功能：
    负责webrtc的播放请求、断开请求 
	 
日期    2023-08-03
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientWebrtcPlayer.h"
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
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern MediaServerPort                       ABL_MediaServerPort;

#include "../webrtc-streamer/inc/rtc_obj_sdk.h"

CNetClientWebrtcPlayer::CNetClientWebrtcPlayer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
#ifdef WebRtcVideoFileFlag
	nWriteFileCount = 0;
	char szFileName[256] = { 0 };
	sprintf(szFileName,"%s%X.264", ABL_MediaSeverRunPath, this);
	fWriteVideoFile = fopen(szFileName, "wb");

	sprintf(szFileName, "%s%X_Length.txt", ABL_MediaSeverRunPath, this);
 	fWriteFrameLengthFile = fopen(szFileName, "wb");;
#endif
	nSpsPositionPos = 0;
	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_NetClientWebrtcPlayer;
	nMediaClient = 0;
	nCreateDateTime = GetTickCount64();
	nClient = hClient;
	bRunFlag = true;

	pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
	if (pMediaSource == NULL)
	{
		WriteLog(Log_Debug, "CNetClientWebrtcPlayer = %X  nClient = %llu ,不存在媒体源 %s ,准备删除 webrtc 播放类", this, nClient, m_szShareMediaURL);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
	if(pMediaSource->bCreateWebRtcPlaySourceFlag.load() == false )
	{
		if (VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL) != NULL)
		{
	      VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL)->Init("H264", pMediaSource->m_mediaCodecInfo.nWidth, pMediaSource->m_mediaCodecInfo.nHeight, pMediaSource->m_mediaCodecInfo.nVideoFrameRate);
	      pMediaSource->bCreateWebRtcPlaySourceFlag.exchange(true);
 	    }
		WriteLog(Log_Debug, "CNetClientWebrtcPlayer = %X  nClient = %llu audioname =%s ", this, nClient, pMediaSource->m_mediaCodecInfo.szAudioName);

		ABL::to_upper(pMediaSource->m_mediaCodecInfo.szAudioName);
		int format= AUDIOMIX_AVSampleFormat::AUDIOMIX_FMT_S16;

		std::map<std::string, std::string> streamInfo;

		if (strcmp(pMediaSource->m_mediaCodecInfo.szAudioName, "AAC") == 0) {
			format = AUDIOMIX_AVSampleFormat::AUDIOMIX_FMT_FLTP;
			streamInfo["codec_id"] = std::to_string(AV_CODEC_ID_AAC);
		}
		if (strcmp(pMediaSource->m_mediaCodecInfo.szAudioName, "G711_A") == 0) {
			format = AUDIOMIX_AVSampleFormat::AUDIOMIX_FMT_S16;
			streamInfo["codec_id"] = std::to_string(AV_CODEC_ID_PCM_ALAW);
		}
		if (strcmp(pMediaSource->m_mediaCodecInfo.szAudioName, "G711_U") == 0) {
			format = AUDIOMIX_AVSampleFormat::AUDIOMIX_FMT_S16;
			streamInfo["codec_id"] = std::to_string(AV_CODEC_ID_PCM_MULAW);
		}
		m_resampler = AudioResamplerAPI::CreateAudioResampler(
			(AUDIOMIX_AVSampleFormat)format, pMediaSource->m_mediaCodecInfo.nSampleRate, pMediaSource->m_mediaCodecInfo.nChannels,
			(AUDIOMIX_AVSampleFormat)AUDIOMIX_AVSampleFormat::AUDIOMIX_FMT_S16, m_sample_rate, m_nb_channels);

	
		streamInfo["sample_rate"] = std::to_string(pMediaSource->m_mediaCodecInfo.nSampleRate);
		streamInfo["nb_channels"] =std::to_string(pMediaSource->m_mediaCodecInfo.nChannels);
		streamInfo["format"] = std::to_string(format);
	

		m_decder = FFmpegAudioDecoderAPI::CreateDecoder(streamInfo);
		// 处理音频数据的线程函数
		ABL::ThreadPool::getInstance().append([&]()
			{
				stopThread.store(false);
				while (!stopThread.load())
				{
					int sample_size = 0;
					uint8_t* output_data[AV_NUM_DATA_POINTERS] = { 0 };
					int ret = m_resampler->GetOneFrame(output_data, &sample_size);
					if (ret < 0)
					{
						//	std::unique_lock<std::mutex> lock(m_mutex);
							//audioDataCV.wait(lock, [&]() { return  !stopThread.load(); });
						if (stopThread.load())
						{
							return;
						}
						for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
						{
							if (output_data[i] != nullptr) {
								delete[] output_data[i];
								output_data[i] = nullptr;
							}
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(2));
						continue;
					}

					if (AudioCaptureManager::getInstance().GetInput(m_szShareMediaURL) && nSpsPositionPos >= 0)
					{
						AudioCaptureManager::getInstance().GetInput(m_szShareMediaURL)->onData("", output_data[0], sample_size, 1);
					}
				
					for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
						if (output_data[i] != nullptr) {
							delete[] output_data[i];
							output_data[i] = nullptr;
						}
					}
					continue;
				}
			});


	}
	pMediaSource->AddClientToMap(nClient);
	pMediaSource->nWebRtcPushStreamID = nClient;
	SplitterAppStream(szShareMediaURL);
	sprintf(m_addStreamProxyStruct.url, "http://%s:%d/webrtc-streamer.html?video=/%s/%s", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
	WriteLog(Log_Debug, "CNetClientWebrtcPlayer 构造 = %X  nClient = %llu ", this, nClient);
}

CNetClientWebrtcPlayer::~CNetClientWebrtcPlayer()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	stopThread.store(true);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	if (m_decder != nullptr)
	{
		delete  m_decder;
		m_decder = nullptr;
	}
	if (m_resampler != nullptr)
	{
		delete  m_resampler;
		m_resampler = nullptr;
	}

	if (pMediaSource != NULL)
	{
		if (pMediaSource->bCreateWebRtcPlaySourceFlag.load() == true)
			pMediaSource->bCreateWebRtcPlaySourceFlag.exchange(false);
 	}
#ifdef WebRtcVideoFileFlag
	if(fWriteVideoFile)
	  fclose(fWriteVideoFile);
	if(fWriteFrameLengthFile)
	  fclose(fWriteFrameLengthFile);
#endif
  	WriteLog(Log_Debug, "CNetClientWebrtcPlayer 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetClientWebrtcPlayer::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag)
		return -1;

	nRecvDataTimerBySecond = 0 ;

	if(strcmp(szVideoCodec,"H264") != 0  )
		return -1 ;

#ifdef WebRtcVideoFileFlag
	if (fWriteVideoFile && nDataLength >= 1000 * 100 * 200 )
	{
		nWriteFileCount += nDataLength ;
		if (nWriteFileCount < 1024 * 1024 * 50)
		{
		  fwrite(pVideoData,1, nDataLength,fWriteVideoFile);
		  fflush(fWriteVideoFile);
 		}
 	}

	if (fWriteFrameLengthFile)
	{
		if (nWriteFileCount < 1024 * 1024 * 50)
		{
		   fprintf(fWriteFrameLengthFile, "frame = %d  , %02x %02x %02x %02x %02x \r\n", nDataLength, pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4]);
		   fflush(fWriteFrameLengthFile);
		}
	}
#endif
	nSpsPositionPos = FindSPSPositionPos("H264", pVideoData, nDataLength);

	if (VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL) && nSpsPositionPos >= 0 )
	{
	   VideoCaptureManager::getInstance().GetInput(m_szShareMediaURL)->onData("H264", pVideoData + nSpsPositionPos , nDataLength - nSpsPositionPos, 1);
 	}
 
	return 0;
}

int CNetClientWebrtcPlayer::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag)
		return -1;

	nRecvDataTimerBySecond = 0;


	int outSize = 0;
	uint8_t* outData[AV_NUM_DATA_POINTERS] = { 0 };
	bool res = m_decder->DecodeData(pVideoData, nDataLength, outData, &outSize);
	if (res && outSize>0)
	{
		res = m_resampler->FillingPCM(outData, outSize);
	
	}
	// 释放资源
	for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
		if (outData[i] != nullptr) {
			delete[] outData[i];
			outData[i] = nullptr;
		}
	}

	return 0;
}

int CNetClientWebrtcPlayer::SendVideo()
{
	return 0;
}

int CNetClientWebrtcPlayer::SendAudio()
{

	return 0;
}

int CNetClientWebrtcPlayer::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CNetClientWebrtcPlayer::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetClientWebrtcPlayer::SendFirstRequst()
{
	 return 0;
}

//请求m3u8文件
bool  CNetClientWebrtcPlayer::RequestM3u8File()
{
	return true;
}