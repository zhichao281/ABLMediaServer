/*
���ܣ�
    �������������Ϊmp4(fmp4��ʽ) 
	 
����    2022-01-09
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "StreamRecordFMP4.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSourceNoLock(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern boost::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSourceNoLock(char* szURL);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort;
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern std::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO

#endif

static int StreamRecordFMP4_hls_segment(void* param, const void* data, size_t bytes, int64_t pts, int64_t dts, int64_t duration)
{
	CStreamRecordFMP4* pNetServerHttpMp4 = (CStreamRecordFMP4*)param;
	if (pNetServerHttpMp4 == NULL)
		return 0;
	
	if(!pNetServerHttpMp4->bCheckHttpMP4Flag || !pNetServerHttpMp4->bRunFlag.load())
		return -1 ;

	if (bytes > 0)
	{
		pNetServerHttpMp4->writeTSBufferToMP4File((unsigned char*)data, bytes);
	}

	return 0;
}

CStreamRecordFMP4::CStreamRecordFMP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	bCreateNewRecordFile = false;
	nRecordDateTime = GetTickCount64();
	memset((char*)&avc, 0x00, sizeof(avc));
	memset((char*)&hevc, 0x00, sizeof(hevc));

	nCurrentVideoFrames = 0;//��ǰ��Ƶ֡��
	nTotalVideoFrames = 0;//¼����Ƶ��֡��

	strcpy(m_szShareMediaURL,szShareMediaURL);
 	netBaseNetType = NetBaseNetType_RecordFile_FMP4;
	nMediaClient = 0;
	bAddSendThreadToolFlag = false;
	bWaitIFrameSuccessFlag = false;
	fWriteMP4 = NULL;

	nClient = hClient;
	hls_init_segmentFlag = false;
	audioDts = 0;
	videoDts = 0;
	track_aac = -1;
	track_video = -1;
	hlsFMP4 = hls_fmp4_create((1) * 1000, StreamRecordFMP4_hls_segment, this);
	strcpy(szClientIP, szIP);
	nClientPort = nPort;

	memset(netDataCache ,0x00,sizeof(netDataCache));
	MaxNetDataCacheCount = sizeof(netDataCache);
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//�������ݻ����С
	bFindMP4NameFlag = false;
	memset(szMP4Name, 0x00, sizeof(szMP4Name));
	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

	bCheckHttpMP4Flag = true;
	nCreateDateTime = GetTickCount64();

	WriteLog(Log_Debug, "CStreamRecordFMP4 ���� = %X nClient = %llu ", this, nClient);
}

CStreamRecordFMP4::~CStreamRecordFMP4()
{
	bCheckHttpMP4Flag = false ;
	bRunFlag.exchange(false);
	std::lock_guard<std::mutex> lock(mediaMP4MapLock);

	//ɾ��fmp4��Ƭ���
	if (hlsFMP4 != NULL)
	{
		hls_fmp4_destroy(hlsFMP4);
		hlsFMP4 = NULL;
    }
 
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
 
	if (fWriteMP4)
	{
 		fclose(fWriteMP4);
		fWriteMP4 = NULL;
#ifdef  OS_System_Windows
		ftruncate(szFileName, nWriteRecordByteSize);
#else
		truncate(szFileName, nWriteRecordByteSize);
#endif 
		//���һ��fmp4��Ƭ�ļ�֪ͨ 
		if (ABL_MediaServerPort.hook_enable == 1 )
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_Record_mp4;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_record_mp4\",\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"startTime\":\"%s\",\"endTime\":\"%s\",\"fileSize\":%d}", key, app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), szStartDateTime, GetCurrentDateTime(), nWriteRecordByteSize);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		}
	}

	WriteLog(Log_Debug, "CStreamRecordFMP4 ���� = %X nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CStreamRecordFMP4::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag.load())
		return -1;
	nRecvDataTimerBySecond = 0;
	nCurrentVideoFrames ++;//��ǰ��Ƶ֡��
	nTotalVideoFrames ++ ;//¼����Ƶ��֡��

	m_videoFifo.push(pVideoData, nDataLength);

	if (ABL_MediaServerPort.hook_enable == 1 && (GetTickCount64() - nCreateDateTime ) >= 1000 * 30 )
	{
 		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_Record_Progress;
		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_record_progress\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"TotalVideoDuration\":%llu,\"startTime\":\"%s\",\"endTime\":\"%s\"}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType,key, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), (nTotalVideoFrames / mediaCodecInfo.nVideoFrameRate), szStartDateTime, GetCurrentDateTime());
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		nCreateDateTime = GetTickCount64();
	}
	return 0;
}

int CStreamRecordFMP4::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag.load() || !(strcmp(szAudioCodec,"AAC") == 0 || strcmp(szAudioCodec, "MP3") == 0))
		return -1;

	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

int CStreamRecordFMP4::SendVideo()
{
	std::lock_guard<std::mutex> lock(mediaMP4MapLock);

	if (!bCheckHttpMP4Flag)
		return -1;

	nRecvDataTimerBySecond = 0;

	if (!bCheckHttpMP4Flag)
		return -1;

  if (ABL_MediaServerPort.nEnableAudio == 0)
	 nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	videoDts += nVideoStampAdd;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (hlsFMP4)
			VideoFrameToFMP4File(pData, nLength);

		m_videoFifo.pop_front();
	}
}

int CStreamRecordFMP4::SendAudio()
{
	std::lock_guard<std::mutex> lock(mediaMP4MapLock);

	if (ABL_MediaServerPort.nEnableAudio == 0 || !bCheckHttpMP4Flag || !(strcmp(mediaCodecInfo.szAudioName, "AAC") == 0 || strcmp(mediaCodecInfo.szAudioName, "MP3") == 0))
		return 0 ;
   
 	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (ABL_MediaServerPort.nEnableAudio == 1 && strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		{
			if (nAsyncAudioStamp == -1)
				nAsyncAudioStamp = GetTickCount();

			avtype = PSI_STREAM_AAC;

			if (hlsFMP4 != NULL )
			{
				if (track_aac == -1)
				{
					nAACLength = mpeg4_aac_adts_frame_length(pData, nLength);
					if (nAACLength < 0)
					{
						m_audioFifo.pop_front();
						return false;
					}

					mpeg4_aac_adts_load(pData, nLength, &aacHandle);
					nExtenAudioDataLength = mpeg4_aac_audio_specific_config_save(&aacHandle, szExtenAudioData, sizeof(szExtenAudioData));

					if (nExtenAudioDataLength > 0 && track_video >= 0)
					{
						track_aac = hls_fmp4_add_audio(hlsFMP4, MOV_OBJECT_AAC, mediaCodecInfo.nChannels, 16, mediaCodecInfo.nSampleRate, szExtenAudioData, nExtenAudioDataLength);
					}
				}

				//����hls_init_segment ��ʼ����ɲ���д��Ƶ�Σ��ڻص�������������־ 
				if (track_aac >= 0 && hls_init_segmentFlag)
				{
					hls_fmp4_input(hlsFMP4, track_aac, pData + 7, nLength - 7, audioDts, audioDts, 0);
				}
			}

			audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp;

		}
		else if (ABL_MediaServerPort.nEnableAudio == 1 && strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		{
			if (track_aac == -1 && hlsFMP4 != NULL )
			{
				if(strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 )
				  track_aac = hls_fmp4_add_audio(hlsFMP4, MOV_OBJECT_G711a, mediaCodecInfo.nChannels, 16, mediaCodecInfo.nSampleRate, NULL, 0);
				else 
				  track_aac = hls_fmp4_add_audio(hlsFMP4, MOV_OBJECT_G711u, mediaCodecInfo.nChannels, 16, mediaCodecInfo.nSampleRate, NULL, 0);
			}

			//����hls_init_segment ��ʼ����ɲ���д��Ƶ�Σ��ڻص�������������־ 
			if (track_aac >= 0 && hls_init_segmentFlag && hlsFMP4 != NULL )
			{
				hls_fmp4_input(hlsFMP4, track_aac, pData, nLength , audioDts, audioDts, 0);
			}

			audioDts += nLength / 8 ;
		}

		//ͬ������Ƶ 
		SyncVideoAudioTimestamp();

		m_audioFifo.pop_front();
	}
	return 0;
}

int CStreamRecordFMP4::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	
	return 0;
}

int CStreamRecordFMP4::ProcessNetData()
{
	return 0;
}

//���͵�һ������
int CStreamRecordFMP4::SendFirstRequst()
{

	 return 0;
}

//����m3u8�ļ�
bool  CStreamRecordFMP4::RequestM3u8File()
{
	return true;
}

static int fmp4_hls_init_segment(hls_fmp4_t* hls, void* param)
{
	CStreamRecordFMP4* pNetServerHttpMp4 = (CStreamRecordFMP4*)param;
	if (pNetServerHttpMp4 == NULL)
		return 0;

	int bytes = hls_fmp4_init_segment(hls, pNetServerHttpMp4->s_packet, sizeof(pNetServerHttpMp4->s_packet));

	pNetServerHttpMp4->fTSFileWriteByteCount = pNetServerHttpMp4->nFmp4SPSPPSLength = bytes;
	pNetServerHttpMp4->s_packetLength = bytes;

	if (pNetServerHttpMp4->fWriteMP4 == NULL)
	{
#ifdef OS_System_Windows
		SYSTEMTIME st;
		GetLocalTime(&st);
		sprintf(pNetServerHttpMp4->szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", pNetServerHttpMp4->szRecordPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		sprintf(pNetServerHttpMp4->szFileNameOrder, "%04d%02d%02d%02d%02d%02d.mp4", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
		sprintf(pNetServerHttpMp4->szStartDateTime, "%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
#else
		time_t now;
		time(&now);
		struct tm *local;
		local = localtime(&now);
		sprintf(pNetServerHttpMp4->szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", pNetServerHttpMp4->szRecordPath, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
		sprintf(pNetServerHttpMp4->szFileNameOrder, "%04d%02d%02d%02d%02d%02d.mp4", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
		sprintf(pNetServerHttpMp4->szStartDateTime, "%04d%02d%02d%02d%02d%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
#endif
			auto pRecord = GetRecordFileSource(pNetServerHttpMp4->m_szShareMediaURL);
			if (pRecord)
			{
				pNetServerHttpMp4->nStartDateTime = GetCurrentSecond();
				pNetServerHttpMp4->bUpdateFlag = pRecord->UpdateExpireRecordFile(pNetServerHttpMp4->szFileName,&pNetServerHttpMp4->nOldFileSize);
				if (pNetServerHttpMp4->bUpdateFlag)
				{
					pNetServerHttpMp4->fWriteMP4 = fopen(pNetServerHttpMp4->szFileName, "r+b");
					if (pNetServerHttpMp4->fWriteMP4)
						fseek(pNetServerHttpMp4->fWriteMP4, 0, SEEK_SET);
				}
				else
				   pNetServerHttpMp4->fWriteMP4 = fopen(pNetServerHttpMp4->szFileName, "wb");

				if (pNetServerHttpMp4->fWriteMP4)
				{
				   pRecord->AddRecordFile(pNetServerHttpMp4->szFileNameOrder);
				   WriteLog(Log_Debug, "CStreamRecordFMP4 = %X %s ����¼���ļ� nClient = %llu ,nMediaClient = %llu szFileNameOrder %s ", pNetServerHttpMp4, pNetServerHttpMp4->m_szShareMediaURL, pNetServerHttpMp4->nClient, pNetServerHttpMp4->nMediaClient, pNetServerHttpMp4->szFileNameOrder);
 		         }
  			}
 	}

	if (pNetServerHttpMp4->fWriteMP4)
	{
		fwrite(pNetServerHttpMp4->s_packet, 1, bytes, pNetServerHttpMp4->fWriteMP4);
 		pNetServerHttpMp4->nWriteRecordByteSize += bytes ;
	}
	//����hls_init_segment ��ʼ����ɲ���д��Ƶ����Ƶ�Σ��ڻص�������������־
	pNetServerHttpMp4->hls_init_segmentFlag = true;
	pNetServerHttpMp4->audioDts = 0;

	return 0;
}

bool  CStreamRecordFMP4::VideoFrameToFMP4File(unsigned char* szVideoData, int nLength)
{
	if (track_video < 0 )
	{
		int n;
		//vcl �� update ��Ҫ��ֵΪ 0 ���������ױ��� 
		vcl = 0;
		update = 0;        
		if (memcmp(mediaCodecInfo.szVideoName, "H264", 4) == 0)
		{
			n = h264_annexbtomp4(&avc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		}
		else if (memcmp(mediaCodecInfo.szVideoName, "H265", 4) == 0)
		{
			n = h265_annexbtomp4(&hevc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		}
		else
			return false;

		if (track_video < 0)
		{
			memset(szExtenVideoData, 0x00, sizeof(szExtenVideoData));
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			{//H264 �ȴ� SPS��PPS �ķ��� 
				if (avc.nb_sps < 1 || avc.nb_pps < 1)
				{
 					return false  ; // waiting for sps/pps
				}
				extra_data_size = mpeg4_avc_decoder_configuration_record_save(&avc, szExtenVideoData, sizeof(szExtenVideoData));
			}
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			{//H265 �ȴ�SPS��PPS�ķ��� 
				if (hevc.numOfArrays < 1)
				{
 					return false ; // waiting for vps/sps/pps
				}
			    extra_data_size = mpeg4_hevc_decoder_configuration_record_save(&hevc, szExtenVideoData, sizeof(szExtenVideoData));
			}
			else
				return false;

			if (extra_data_size <= 0)
			{
				return false;
			}

			if (extra_data_size > 0)
			{
				if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				  track_video = hls_fmp4_add_video(hlsFMP4, MOV_OBJECT_H264, mediaCodecInfo.nWidth, mediaCodecInfo.nHeight, szExtenVideoData, extra_data_size);
				else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				  track_video = hls_fmp4_add_video(hlsFMP4, MOV_OBJECT_HEVC, mediaCodecInfo.nWidth, mediaCodecInfo.nHeight, szExtenVideoData, extra_data_size);
			}

			if (nExtenAudioDataLength > 0)
			{
				track_aac = hls_fmp4_add_audio(hlsFMP4, MOV_OBJECT_AAC, mediaCodecInfo.nChannels, 16, mediaCodecInfo.nSampleRate, szExtenAudioData, nExtenAudioDataLength);
			}
		}
	}

	if (track_video >= 0 && hlsFMP4 != NULL )
	{
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, szVideoData, nLength) == true)
		{
			if (!bWaitIFrameSuccessFlag)
				bWaitIFrameSuccessFlag = true;
			flags = 1;
		}
		else
			flags = 0;

		if (!bWaitIFrameSuccessFlag)
			return false ;

		vcl = 0;
		update = 0;        

		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			nMp4BufferLength = h264_annexbtomp4(&avc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			nMp4BufferLength = h265_annexbtomp4(&hevc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);
		else
			return false;

		///���û����Ƶ��ֱ�ӿ�ʼд��Ƶ���������Ƶ����Ҫ�ȴ���Ƶ�����Ч
		if (nMp4BufferLength > 0 && (ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName, "AAC") != 0 || (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0 && track_aac >= 0)))
		{
			if (hls_init_segmentFlag == false)
			{
				fmp4_hls_init_segment(hlsFMP4, this);
			}

			//����hls_init_segment ��ʼ����ɲ���д��Ƶ�Σ��ڻص�������������־ 
			if (hls_init_segmentFlag == true)
				hls_fmp4_input(hlsFMP4, track_video, pH265Buffer, nMp4BufferLength, videoDts, videoDts, (flags == 1) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
	}

	return true;
}

bool CStreamRecordFMP4::writeTSBufferToMP4File(unsigned char* pTSData, int nLength)
{
	bool bUpdateFlag = false;
	if (fWriteMP4 && pTSData != NULL && nLength > 0)
	{
		fwrite(pTSData, 1, nLength, fWriteMP4);
		nWriteRecordByteSize += nLength;

	if (ABL_MediaServerPort.recordFileCutType == 1)
	{//���շ���������ʱ�䳤�ﵽ fileSecond ���������������
		if ((GetTickCount64() - nRecordDateTime) / 1000 >= ABL_MediaServerPort.fileSecond)
		{
			bCreateNewRecordFile = true;
			nRecordDateTime = GetTickCount64();
		}
	}
	else
	{//����¼���ļ�����Ƶ֡������ �� ��Ƶ֡�ٶȼ����¼��ʱ���ﵽ fileSecond ���������������
		if ((nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate) >= ABL_MediaServerPort.fileSecond)
			bCreateNewRecordFile = true; 
	}

	if (bCreateNewRecordFile == true )
	{
			fclose(fWriteMP4);
            bCreateNewRecordFile = false ;
#ifdef  OS_System_Windows
			ftruncate(szFileName, nWriteRecordByteSize);
#else
			truncate(szFileName, nWriteRecordByteSize);
#endif 		
			//���һ��fmp4��Ƭ�ļ�֪ͨ 
			if (ABL_MediaServerPort.hook_enable == 1 )
			{
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = NetBaseNetType_HttpClient_Record_mp4;
				sprintf(msgNotice.szMsg, "{\"eventName\":\"on_record_mp4\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"startTime\":\"%s\",\"endTime\":\"%s\",\"fileSize\":%d}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), szStartDateTime, GetCurrentDateTime(), nWriteRecordByteSize);
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			}
			nCurrentVideoFrames = 0;
			nWriteRecordByteSize = 0;

#ifdef OS_System_Windows
			SYSTEMTIME st;
			GetLocalTime(&st);
			sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", szRecordPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		    sprintf(szFileNameOrder,"%04d%02d%02d%02d%02d%02d.mp4", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
			sprintf(szStartDateTime, "%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
#else
			time_t now;
			time(&now);
			struct tm *local;
			local = localtime(&now);
			sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", szRecordPath, local->tm_year + 1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
		    sprintf(szFileNameOrder, "%04d%02d%02d%02d%02d%02d.mp4", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
			sprintf(szStartDateTime, "%04d%02d%02d%02d%02d%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);;
#endif

			auto pRecord = GetRecordFileSource(m_szShareMediaURL);
			if (pRecord)
			{
				nStartDateTime = GetCurrentSecond();
 				bUpdateFlag = pRecord->UpdateExpireRecordFile(szFileName,&nOldFileSize);
				if (bUpdateFlag)
				{
					fWriteMP4 = fopen(szFileName, "r+b");
					if (fWriteMP4)
						fseek(fWriteMP4, 0, SEEK_SET);
				}
				else
					fWriteMP4 = fopen(szFileName, "wb");

				if (fWriteMP4 != NULL)
				{
					fwrite(s_packet, 1, s_packetLength, fWriteMP4);
					nWriteRecordByteSize += s_packetLength ;
				}
				pRecord->AddRecordFile(szFileNameOrder);
				WriteLog(Log_Debug, "CStreamRecordFMP4 = %X %s ����¼���ļ� nClient = %llu ,nMediaClient = %llu szFileNameOrder %s ", this, m_szShareMediaURL, nClient, nMediaClient, szFileNameOrder);
 			}				
 
			nCreateDateTime = GetTickCount64();
		}

		return true;
	}
	else
		return false;
}