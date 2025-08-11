/*
���ܣ�
    ʵ�ֱ�׼mp4��ʽ����¼�����Ҫ���øø�ʽ¼����Ҫ�������ļ��а�videoFileFormat ��ֵ����Ϊ2 �Ų���mp4��ʽ ��1Ϊfmp4��ʽ
 	  #¼���ļ����õ��ļ���ʽ 1 Ϊ fmp4��ʽ ��2 Ϊ mp4 ��ʽ 
      videoFileFormat=2

����    2022-10-19
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "StreamRecordMP4.h"
#include "mov-buffer.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;    //��Ϣ֪ͨFIFO
extern boost::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern MediaServerPort                       ABL_MediaServerPort;
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;    //��Ϣ֪ͨFIFO
extern std::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
#endif

static int mp4_mov_file_read(void* fp, void* data, uint64_t bytes)
{
	if (bytes == fread(data, 1, bytes, (FILE*)fp))
		return 0;
	return 0 != ferror((FILE*)fp) ? ferror((FILE*)fp) : -1 /*EOF*/;
}

static int mp4_mov_file_write(void* fp, const void* data, uint64_t bytes)
{
	return bytes == fwrite(data, 1, bytes, (FILE*)fp) ? 0 : ferror((FILE*)fp);
}

static int mp4_mov_file_seek(void* fp, int64_t offset)
{
	return fseek((FILE*)fp, offset, SEEK_SET);
}

static int64_t mp4_mov_file_tell(void* fp)
{
	return ftell((FILE*)fp);
}

const struct mov_buffer_t* mp4_mov_file_buffer(void)
{
	static struct mov_buffer_t s_io = {
		mp4_mov_file_read,
		mp4_mov_file_write,
		mp4_mov_file_seek,
		mp4_mov_file_tell,
	};
	return &s_io;
}

CStreamRecordMP4::CStreamRecordMP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	bCreateNewRecordFile = false;
	nRecordDateTime = GetTickCount64();
	ascLength = 0;
	memset((char*)&ctx.avc, 0x00, sizeof(ctx.avc));
	memset((char*)&ctx.hevc, 0x00, sizeof(ctx.hevc));

	memset(szFileNameOrder, 0x00, sizeof(szFileNameOrder));
	nCurrentVideoFrames = 0;//��ǰ��Ƶ֡��
	nTotalVideoFrames = 0;//¼����Ƶ��֡��
	m_videoFifo.InitFifo(MaxWriteMp4BufferCount);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

	strcpy(m_szShareMediaURL, szShareMediaURL);
	netBaseNetType = NetBaseNetType_RecordFile_MP4;
	nClient = hClient;
	nMediaClient = 0;
	bRunFlag.exchange(true);

	nVideoFrameCount = 0;
	fWriteMP4 = NULL;
	ctx.mov = NULL;
	s_buffer = new unsigned char[MaxWriteMp4BufferCount];
	m_bOpenFlag = false;
	nVideoCodec = -1;
	nAudioCodec = -1;
	nAudioChannels = -1;
	nSampleRate = -1;
	nCreateDateTime = GetTickCount64();

	WriteLog(Log_Debug, "CStreamRecordMP4 ���� = %X  nClient = %llu ", this, nClient);
}

CStreamRecordMP4::~CStreamRecordMP4()
{
	bRunFlag.exchange(false);

	CloseMp4File();

	std::lock_guard<std::mutex> lock(writeMp4Lock);

	if (s_buffer != NULL)
	{
		delete[] s_buffer;
		s_buffer = NULL;
	}
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();

	malloc_trim(0);
	WriteLog(Log_Debug, "CStreamRecordMP4 ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
}

int CStreamRecordMP4::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag.load())
		return -1;

	if(!m_bOpenFlag)
	  OpenMp4File(mediaCodecInfo.nWidth, mediaCodecInfo.nHeight);

	nRecvDataTimerBySecond = 0;
	nCurrentVideoFrames ++;//��ǰ��Ƶ֡��
	nTotalVideoFrames ++;//¼����Ƶ��֡��
	nWriteRecordByteSize += nDataLength;

	m_videoFifo.push(pVideoData, nDataLength);

	if (ABL_MediaServerPort.hook_enable == 1 && (GetTickCount64() - nCreateDateTime) >= 1000 * 30 )
	{
 		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_Record_Progress;
		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_record_progress\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"TotalVideoDuration\":%llu,\"startTime\":\"%s\",\"endTime\":\"%s\"}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, key, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), (nTotalVideoFrames / mediaCodecInfo.nVideoFrameRate), szStartDateTime, GetCurrentDateTime());
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		nCreateDateTime = GetTickCount64();
	}

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

	if (bCreateNewRecordFile == true)
  	{
		CloseMp4File();
		bCreateNewRecordFile = false;

		videoDts = audioDts = 0 ; //��Ƶ����Ƶʱ������¸�λ 
		nCurrentVideoFrames = 0;//��ǰ�ļ���С���¸�λ
	}

	return 0;
}

int CStreamRecordMP4::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag.load())
		return -1;

	nWriteRecordByteSize += nDataLength;
	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

int CStreamRecordMP4::SendVideo()
{
	if (!bRunFlag.load())
		return -1;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_videoFifo.pop(&nLength)) != NULL && nLength > 0)
	{
		if (m_bOpenFlag)
			AddVideo(mediaCodecInfo.szVideoName, pData, nLength);

		m_videoFifo.pop_front();
	}
	return 0;
}

int CStreamRecordMP4::SendAudio()
{
	if (!bRunFlag.load())
		return -1;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL && nLength > 0 )
	{
		if (m_bOpenFlag)
			AddAudio(mediaCodecInfo.szAudioName, pData, nLength);

		m_audioFifo.pop_front();
	}
	return 0;
}

int CStreamRecordMP4::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
    return 0;
}

int CStreamRecordMP4::ProcessNetData()
{
	if (!bRunFlag.load())
		return -1;

 	return 0;
}

//���͵�һ������
int CStreamRecordMP4::SendFirstRequst()
{
 
    return 0;
}

//����m3u8�ļ�
bool  CStreamRecordMP4::RequestM3u8File()
{
	return true;
}

//����һ��mp4�ļ�
bool CStreamRecordMP4::OpenMp4File(int nWidth, int nHeight)
{
	std::lock_guard<std::mutex> lock(writeMp4Lock);

	if (!m_bOpenFlag)
	{
		memset(&ctx, 0, sizeof(ctx));

		ctx.track = ctx.trackAudio = -1;
		ctx.width = nWidth;
		ctx.height = nHeight;

		if (fWriteMP4 == NULL)
		{
#ifdef OS_System_Windows
			SYSTEMTIME st;
			GetLocalTime(&st);
			sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", szRecordPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			sprintf(szFileNameOrder, "%04d%02d%02d%02d%02d%02d.mp4", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
			sprintf(szStartDateTime, "%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);;
#else
			time_t now;
			time(&now);
			struct tm *local;
			local = localtime(&now);
			sprintf(szFileName, "%s%04d%02d%02d%02d%02d%02d.mp4", szRecordPath, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
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
					fWriteMP4 = fopen(szFileName, "wb+");
					if (fWriteMP4)
						fseek(fWriteMP4, 0, SEEK_SET);
				}
				else
					fWriteMP4 = fopen(szFileName, "wb+");

				if (fWriteMP4)
				{
					pRecord->AddRecordFile(szFileNameOrder);
					WriteLog(Log_Debug, "CStreamRecordFMP4 = %X %s ����¼���ļ� nClient = %llu ,nMediaClient = %llu szFileNameOrder %s ", this, m_szShareMediaURL, nClient, nMediaClient,szFileNameOrder);
				}
			}
		}

		if (fWriteMP4 == NULL)
		{
			bRunFlag.exchange(false);
			WriteLog(Log_Debug, "����¼���ļ�ʧ�ܣ�׼��ɾ��  nClient = %llu ", nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return false;
		}
 
 		ctx.mov = mov_writer_create(mp4_mov_file_buffer(), fWriteMP4, MOV_FLAG_FASTSTART);

		m_bOpenFlag = true;

		return true;
	}
	else
		return true;
}

//�ر�mp4�ļ�
bool CStreamRecordMP4::CloseMp4File()
{
	std::lock_guard<std::mutex> lock(writeMp4Lock);
	nVideoFrameCount = 0;
	if (m_bOpenFlag)
	{
		if(ctx.mov != NULL )
		   mov_writer_destroy(ctx.mov);
		if(fWriteMP4)
		  fclose(fWriteMP4);

		fWriteMP4 = NULL;
		ctx.mov = NULL;
		WriteLog(Log_Debug, "CStreamRecordMP4 �ر�mp4�ļ� m_szMp4FileName = %s ", szFileName);

		//���һ��mp4��Ƭ�ļ�֪ͨ 
		if (ABL_MediaServerPort.hook_enable == 1 )
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_Record_mp4;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_record_mp4\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"startTime\":\"%s\",\"endTime\":\"%s\",\"fileSize\":%d}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), szStartDateTime, GetCurrentDateTime(), nWriteRecordByteSize);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			nWriteRecordByteSize = 0;
		}
		m_bOpenFlag = false;
	}
	return true;
}

//������Ƶ
bool CStreamRecordMP4::AddVideo(char* szVideoName, unsigned char* pVideoData, int nVideoDataLength)
{
	std::lock_guard<std::mutex> lock(writeMp4Lock);

	if (!m_bOpenFlag || !bRunFlag.load())
		return false;

	if (ABL_MediaServerPort.nEnableAudio == 0)
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	nVideoFrameCount++;
	vcl = 0; //Ҫ��ֵ0��ʼֵ������д������mp4�ļ����Ų���
	update = 0; //Ҫ��ֵ0��ʼֵ������д������mp4�ļ����Ų���

	if (strcmp(szVideoName, "H264") == 0)
		nSize = h264_annexbtomp4(&ctx.avc, pVideoData, nVideoDataLength, s_buffer, MaxWriteMp4BufferCount, &vcl, &update);
	else if (strcmp(szVideoName, "H265") == 0)
		nSize = h265_annexbtomp4(&ctx.hevc, pVideoData, nVideoDataLength, s_buffer, MaxWriteMp4BufferCount, &vcl, &update);

	if (ctx.track < 0)
	{
		if (strcmp(szVideoName, "H264") == 0)
		{//H264 �ȴ� SPS��PPS �ķ��� 
			if (ctx.avc.nb_sps < 1 || ctx.avc.nb_pps < 1)
			{
				//ctx->ptr = end;
				return false; // waiting for sps/pps
			}
		}
		else if (strcmp(szVideoName, "H265") == 0)
		{//H265 �ȴ�SPS��PPS�ķ��� 
			if (ctx.hevc.numOfArrays < 1)
			{
				//ctx->ptr = end;
				return false; // waiting for vps/sps/pps
			}
		}

		if (strcmp(szVideoName, "H264") == 0)
			extra_data_size = mpeg4_avc_decoder_configuration_record_save(&ctx.avc, s_extra_data, sizeof(s_extra_data));
		else if (strcmp(szVideoName, "H265") == 0)
			extra_data_size = mpeg4_hevc_decoder_configuration_record_save(&ctx.hevc, s_extra_data, sizeof(s_extra_data));

		if (extra_data_size <= 0)
		{
			// invalid AVCC
			return false;
		}

 		if (strcmp(szVideoName, "H264") == 0)
			ctx.track = mov_writer_add_video(ctx.mov, MOV_OBJECT_H264, ctx.width, ctx.height, s_extra_data, extra_data_size);
		else if (strcmp(szVideoName, "H265") == 0)
			ctx.track = mov_writer_add_video(ctx.mov, MOV_OBJECT_HEVC, ctx.width, ctx.height, s_extra_data, extra_data_size);

		if (ctx.track < 0)
			return false;
	}

	//������Ƶ���
	if (ABL_MediaServerPort.nEnableAudio == 1)
	{
		if (-1 == ctx.trackAudio && ascLength > 0 && strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_AAC, ctx.aac.channels, 16, ctx.aac.sampling_frequency, asc, ascLength);
		else if (-1 == ctx.trackAudio && strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_G711a, 1, 16, 8000, NULL, 0);
		else if (-1 == ctx.trackAudio && strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_G711u, 1, 16, 8000, NULL, 0);
 	}

	//���û����Ƶ��ֱ�ӿ�ʼд��Ƶ���������Ƶ����Ҫ�ȴ���Ƶ�����Ч
	if (nSize > 0 && (ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName, "AAC") != 0 || (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0 && ctx.trackAudio >= 0)))
	{
		mov_writer_write(ctx.mov, ctx.track, s_buffer, nSize, videoDts, videoDts, 1 == vcl ? MOV_AV_FLAG_KEYFREAME : 0);

		videoDts += nVideoStampAdd;
  	}

	return true;
}

//������Ƶ
bool CStreamRecordMP4::AddAudio(char* szAudioName, unsigned char* pAudioData, int nAudioDataLength)
{
	std::lock_guard<std::mutex> lock(writeMp4Lock);

	//��֤��Ƶ������ټ�����Ƶ ����
	if (!m_bOpenFlag || !bRunFlag.load())
		return false;

	if (strcmp(szAudioName, "AAC") == 0)
	{//AAC
		nAACLength = mpeg4_aac_adts_frame_length(pAudioData, nAudioDataLength);
		if (nAACLength < 0)
			return false;

		if (-1 == ctx.trackAudio)
		{
			mpeg4_aac_adts_load(pAudioData, nAudioDataLength, &ctx.aac);
			ascLength = mpeg4_aac_audio_specific_config_save(&ctx.aac, asc, sizeof(asc));
		}

		if (ctx.trackAudio >= 0 && ctx.track >= 0 && mediaCodecInfo.nSampleRate > 0 )
		{//��������Ƶ������ܿ�ʼд����Ƶ
			mov_writer_write(ctx.mov, ctx.trackAudio, pAudioData + 7, nAudioDataLength - 7, audioDts, audioDts, 0);
 		}
	}
	else if (strcmp(szAudioName, "G711_A") == 0 || strcmp(szAudioName, "G711_U") == 0)
	{
		if (ctx.trackAudio >= 0)  
		{
			//Ϊ�˼��ݻ�ΪVCN��дg711�Ĳ���Ƶ��Ϊ16000���Ӷ�G711ÿ֡����Ϊ640�����MP4�ļ�ʱ���ܳ��ȶ�1�� 
			if (nAudioDataLength > 320)
				nAudioDataLength = 320;

			mov_writer_write(ctx.mov, ctx.trackAudio, pAudioData, nAudioDataLength, audioDts, audioDts, 0);
 		}
	}

	if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp;
	else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		audioDts += nAudioDataLength / 8;

	//ͬ������Ƶ 
	SyncVideoAudioTimestamp();

	return true;
}

