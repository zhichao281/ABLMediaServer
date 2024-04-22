/*
功能：
    实现标准mp4格式进行录像，如果要采用该格式录像，需要在配置文件中把videoFileFormat 的值设置为2 才采用mp4格式 ，1为fmp4格式
 	  #录像文件采用的文件格式 1 为 fmp4格式 ，2 为 mp4 格式 
      videoFileFormat=2

日期    2022-10-19
作者    罗家兄弟
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


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern MediaServerPort                       ABL_MediaServerPort;
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;    //消息通知FIFO
extern boost::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern MediaServerPort                       ABL_MediaServerPort;
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;    //消息通知FIFO
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
	ascLength = 0;
	memset((char*)&ctx.avc, 0x00, sizeof(ctx.avc));
	memset((char*)&ctx.hevc, 0x00, sizeof(ctx.hevc));

	memset(szFileNameOrder, 0x00, sizeof(szFileNameOrder));
	nCurrentVideoFrames = 0;//当前视频帧数
	nTotalVideoFrames = 0;//录像视频总帧数
	m_videoFifo.InitFifo(MaxWriteMp4BufferCount);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

	strcpy(m_szShareMediaURL, szShareMediaURL);
	netBaseNetType = NetBaseNetType_RecordFile_MP4;
	nClient = hClient;
	nMediaClient = 0;
	bRunFlag = true;

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

	WriteLog(Log_Debug, "CStreamRecordMP4 构造 = %X  nClient = %llu ", this, nClient);
}

CStreamRecordMP4::~CStreamRecordMP4()
{
	bRunFlag = false;

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
	WriteLog(Log_Debug, "CStreamRecordMP4 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
}

int CStreamRecordMP4::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (!bRunFlag)
		return -1;

	if(!m_bOpenFlag)
	  OpenMp4File(mediaCodecInfo.nWidth, mediaCodecInfo.nHeight);

	nRecvDataTimerBySecond = 0;
	nCurrentVideoFrames ++;//当前视频帧数
	nTotalVideoFrames ++;//录像视频总帧数
	nWriteRecordByteSize += nDataLength;

	m_videoFifo.push(pVideoData, nDataLength);

	if (ABL_MediaServerPort.hook_enable == 1 && (GetTickCount64() - nCreateDateTime) >= 1000 * 10 )
	{
		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_Record_Progress;
		sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"TotalVideoDuration\":%llu}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, key, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), (nTotalVideoFrames / mediaCodecInfo.nVideoFrameRate));
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		nCreateDateTime = GetTickCount64();
	}

	if ((nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate) >= ABL_MediaServerPort.fileSecond)
	{
		CloseMp4File();

		nCurrentVideoFrames = 0;//当前文件大小重新复位
	}

	return 0;
}

int CStreamRecordMP4::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag )
		return -1;

	nWriteRecordByteSize += nDataLength;
	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

int CStreamRecordMP4::SendVideo()
{
	if (!bRunFlag)
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
	if (!bRunFlag)
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
	if (!bRunFlag)
		return -1;

 	return 0;
}

//发送第一个请求
int CStreamRecordMP4::SendFirstRequst()
{
 
    return 0;
}

//请求m3u8文件
bool  CStreamRecordMP4::RequestM3u8File()
{
	return true;
}

//创建一个mp4文件
bool CStreamRecordMP4::OpenMp4File(int nWidth, int nHeight)
{
	std::lock_guard<std::mutex> lock(writeMp4Lock);

	if (!m_bOpenFlag)
	{
		memset(&ctx, 0, sizeof(ctx));

		ctx.track = ctx.trackAudio = -1;
		ctx.width = nWidth;
		ctx.height = nHeight;

		bool  bUpdateFlag = false;

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
				bUpdateFlag = pRecord->UpdateExpireRecordFile(szFileName);
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
					WriteLog(Log_Debug, "CStreamRecordFMP4 = %X %s 增加录像文件 nClient = %llu ,nMediaClient = %llu szFileNameOrder %s ", this, m_szShareMediaURL, nClient, nMediaClient,szFileNameOrder);
				}
			}
		}

		if (fWriteMP4 == NULL)
		{
			bRunFlag = false;
			WriteLog(Log_Debug, "创建录像文件失败，准备删除  nClient = %llu ", nClient);
			DeleteNetRevcBaseClient(nClient);
			return false;
		}
 
 		ctx.mov = mov_writer_create(mp4_mov_file_buffer(), fWriteMP4, MOV_FLAG_FASTSTART);

		m_bOpenFlag = true;

		return true;
	}
	else
		return true;
}

//关闭mp4文件
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
		WriteLog(Log_Debug, "CStreamRecordMP4 关闭mp4文件 m_szMp4FileName = %s ", szFileName);

		//完成一个mp4切片文件通知 
		if (ABL_MediaServerPort.hook_enable == 1 )
		{
			GetCurrentDatetime();//获取当前时间
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_Record_mp4;
			sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"fileName\":\"%s\",\"currentFileDuration\":%llu,\"startTime\":\"%s\",\"endTime\":\"%s\",\"fileSize\":%llu}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, szFileNameOrder, (nCurrentVideoFrames / mediaCodecInfo.nVideoFrameRate), szStartDateTime, szCurrentDateTime, nWriteRecordByteSize);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			nWriteRecordByteSize = 0;
		}
		m_bOpenFlag = false;
	}
	return true;
}

//加入视频
bool CStreamRecordMP4::AddVideo(char* szVideoName, unsigned char* pVideoData, int nVideoDataLength)
{
	std::lock_guard<std::mutex> lock(writeMp4Lock);

	if (!m_bOpenFlag || !bRunFlag )
		return false;

	if (ABL_MediaServerPort.nEnableAudio == 0)
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	nVideoFrameCount++;
	vcl = 0; //要赋值0初始值，否则写出来的mp4文件播放不了
	update = 0; //要赋值0初始值，否则写出来的mp4文件播放不了

	if (strcmp(szVideoName, "H264") == 0)
		nSize = h264_annexbtomp4(&ctx.avc, pVideoData, nVideoDataLength, s_buffer, MaxWriteMp4BufferCount, &vcl, &update);
	else if (strcmp(szVideoName, "H265") == 0)
		nSize = h265_annexbtomp4(&ctx.hevc, pVideoData, nVideoDataLength, s_buffer, MaxWriteMp4BufferCount, &vcl, &update);

	if (ctx.track < 0)
	{
		if (strcmp(szVideoName, "H264") == 0)
		{//H264 等待 SPS、PPS 的方法 
			if (ctx.avc.nb_sps < 1 || ctx.avc.nb_pps < 1)
			{
				//ctx->ptr = end;
				return false; // waiting for sps/pps
			}
		}
		else if (strcmp(szVideoName, "H265") == 0)
		{//H265 等待SPS、PPS的方法 
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

	//增加音频轨道
	if (ABL_MediaServerPort.nEnableAudio == 1)
	{
		if (-1 == ctx.trackAudio && ascLength > 0 && strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_AAC, ctx.aac.channels, 16, ctx.aac.sampling_frequency, asc, ascLength);
		else if (-1 == ctx.trackAudio && strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_G711a, 1, 16, 8000, NULL, 0);
		else if (-1 == ctx.trackAudio && strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			ctx.trackAudio = mov_writer_add_audio(ctx.mov, MOV_OBJECT_G711u, 1, 16, 8000, NULL, 0);
 	}

	//如果没有音频，直接开始写视频，如果有音频则需要等待音频句柄有效
	if (nSize > 0 && (ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName, "AAC") != 0 || (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0 && ctx.trackAudio >= 0)))
	{
		mov_writer_write(ctx.mov, ctx.track, s_buffer, nSize, videoDts, videoDts, 1 == vcl ? MOV_AV_FLAG_KEYFREAME : 0);

		videoDts += nVideoStampAdd;
  	}

	return true;
}

//加入音频
bool CStreamRecordMP4::AddAudio(char* szAudioName, unsigned char* pAudioData, int nAudioDataLength)
{
	std::lock_guard<std::mutex> lock(writeMp4Lock);

	//保证视频到达后，再加入音频 【】
	if (!m_bOpenFlag || !bRunFlag )
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
		{//必须有视频轨道才能开始写入音频
			mov_writer_write(ctx.mov, ctx.trackAudio, pAudioData + 7, nAudioDataLength - 7, audioDts, audioDts, 0);
 		}
	}
	else if (strcmp(szAudioName, "G711_A") == 0 || strcmp(szAudioName, "G711_U") == 0)
	{
		if (ctx.trackAudio >= 0)  
		{
			//为了兼容华为VCN填写g711的采样频率为16000，从而G711每帧长度为640，造成MP4文件时间总长度多1倍 
			if (nAudioDataLength > 320)
				nAudioDataLength = 320;

			mov_writer_write(ctx.mov, ctx.trackAudio, pAudioData, nAudioDataLength, audioDts, audioDts, 0);
 		}
	}

	if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp;
	else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		audioDts += nAudioDataLength / 8;

	//同步音视频 
	SyncVideoAudioTimestamp();

	return true;
}

