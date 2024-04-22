/*
功能：
        调用ffmepg 的api函数，实现读取 rtmp ,http-flv ,http-mp4 ,http-hls 的码流 
日期    2024-04-05
作者    罗家兄弟
QQ      79941308    
E-Mail  79941308@qq.com
*/
#include "stdafx.h"
#include "NetClientFFmpegRecv.h"
#ifdef USE_BOOST
extern CNetBaseThreadPool* RecordReplayThreadPool;//录像回放线程池
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
#else

extern CNetBaseThreadPool* RecordReplayThreadPool;//录像回放线程池
extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
#endif


extern int avpriv_mpeg4audio_sample_rates[];

#ifdef OS_System_Windows
extern BOOL GBK2UTF8(char *szGbk, char *szUtf8, int Len);
#else
extern int GB2312ToUTF8(char* szSrc, size_t iSrcLen, char* szDst, size_t iDstLen);
#endif

//从回放的录像名字获取点播共享url 
bool  CNetClientFFmpegRecv::GetMediaShareURLFromFileName(char* szRecordFileName,char* szMediaURL)
{
	if (szRecordFileName == NULL || strlen(szRecordFileName) == 0 || szMediaURL == NULL || strlen(szMediaURL) == 0)
		return false;

	string strRecordFileName = szRecordFileName;
#ifdef OS_System_Windows

#ifdef USE_BOOST
	replace_all(strRecordFileName, "\\", "/");
#else
	ABL::replace_all(strRecordFileName, "\\", "/");
#endif


#endif
	int   nPos;
	char  szTempFileName[512] = { 0 };
	nPos = strRecordFileName.rfind("/", strlen(szRecordFileName));
	if (nPos > 0)
	{
		memcpy(szTempFileName, szRecordFileName + nPos + 1, strlen(szRecordFileName) - nPos);
		szTempFileName[strlen(szTempFileName) - 4] = 0x00;
		sprintf(m_szShareMediaURL, "%s%s%s", szMediaURL, RecordFileReplaySplitter, szTempFileName);
		return true;
	}
	else
		return false;
}

//查找视频，音频格式
int CNetClientFFmpegRecv::open_codec_context(int* stream_idx, AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream* st;
	const AVCodec* dec = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		WriteLog(Log_Debug, "Could not find %s stream in input file '%s'\n", av_get_media_type_string(type), szFileNameUTF8);
		return ret;
	}
	else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec)
		{
			WriteLog(Log_Debug, "Failed to find %s codec\n", av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx) {
			WriteLog(Log_Debug, "Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
			WriteLog(Log_Debug, "Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return ret;
		}

		/* Init the decoders */
		if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
			WriteLog(Log_Debug, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

CNetClientFFmpegRecv::CNetClientFFmpegRecv(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	WriteLog(Log_Debug, "CNetClientFFmpegRecv 构造函数 = %X ,nClient = %llu , m_szShareMediaURL = %s  ", this, hClient, szShareMediaURL);
	netBaseNetType = NetBaseNetType_FFmpegRecvNetworkMedia;
	bProxySuccessFlag = false;
	memset(szFFmpegErrorMsg, 0x00, sizeof(szFFmpegErrorMsg));
	strcpy(szReadFileError, "Unknow Error .");
	bResponseHttpFlag = false;
	video_dec_ctx = NULL;
	audio_dec_ctx = NULL;
	video_stream = NULL;
	audio_stream = NULL;

	strcpy(m_szShareMediaURL, szShareMediaURL);
	memset(szFileNameUTF8, 0x00, sizeof(szFileNameUTF8));
	nWaitTime = OpenMp4FileToReadWaitMaxMilliSecond;
	stream_isVideo = -1;
	stream_isAudio = -1;
	buffersrc = NULL;
	bsf_ctx = NULL;
	sample_index = 8;
	m_audioCacheFifo.InitFifo(1024 * 256);
	nInputAudioDelay = 20;
	nInputAudioTime = nCurrentDateTime = GetTickCount64();

	nDownloadFrameCount = 0;

	m_rtspPlayerType = RtspPlayerType_RecordReplay;
	pMediaSource = NULL;
	nClient = hClient;

	WriteLog(Log_Debug, "NetClientFFmpegRecv =  %X ,nClient = %llu 开始读取网络流 %s ", this, nClient, szIP);

	pFormatCtx2 = NULL;
	packet2 = NULL;
	int nRet2 = 0;

	if (strstr(szIP, "rtsp://") != NULL)
	{
		AVDictionary* format_opts = NULL;
		av_dict_set(&format_opts, "buffer_size", "2024000", 0);
		av_dict_set(&format_opts, "timeout", "5000000", 0);
		av_dict_set(&format_opts, "rtsp_transport", "tcp", 0);
		nRet2 = avformat_open_input(&pFormatCtx2, szIP, NULL, &format_opts);
		av_dict_free(&format_opts);
	}
	else
	{
		AVDictionary* format_opts = NULL;
		av_dict_set(&format_opts, "rw_timeout", "3000000", 0); //设置链接超时时间（us）
		nRet2 = avformat_open_input(&pFormatCtx2, szIP, NULL, &format_opts);
		av_dict_free(&format_opts);
	}

	if (nRet2 != 0)
	{
		av_strerror(nRet2, szFFmpegErrorMsg, sizeof(szFFmpegErrorMsg));
		sprintf(szReadFileError, "connect %s failed ! Msg: %s  ", szIP, szFFmpegErrorMsg);
		WriteLog(Log_Debug, "NetClientFFmpegRecv =  %X ,nClient = %llu connect %s failed ,Msg : %s ", this, hClient, szIP, szFFmpegErrorMsg);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}

	//确定是否有媒体源
	if (avformat_find_stream_info(pFormatCtx2, NULL) < 0)
	{
		strcpy(szReadFileError, "file is Not Media File ! ");
		avformat_close_input(&pFormatCtx2);
		WriteLog(Log_Debug, "NetClientFFmpegRecv =  %X ,nClient = %llu 文件中不存在视频、音频流  ", this, hClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}

	//查找出视频源
	if (open_codec_context(&stream_isVideo, &video_dec_ctx, pFormatCtx2, AVMEDIA_TYPE_VIDEO) >= 0)
	{
		video_stream = pFormatCtx2->streams[stream_isVideo];
		if (video_stream->codecpar->codec_id == AV_CODEC_ID_H264)
			strcpy(mediaCodecInfo.szVideoName, "H264");
		else if (video_stream->codecpar->codec_id == AV_CODEC_ID_H265)
		{
			strcpy(mediaCodecInfo.szVideoName, "H265");
		}
		else
		{
			strcpy(szReadFileError, "http-flv (h265) Video Codec Is Not Support ! ");
			WriteLog(Log_Debug, "NetClientFFmpegRecv =  %X ,nClient = %llu ，video_stream->codecpar->codec_id = %d 视频格式不是H264、H265 ", this, hClient, video_stream->codecpar->codec_id);
			avformat_close_input(&pFormatCtx2);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return;
		}

		mediaCodecInfo.nWidth = video_dec_ctx->width;
		mediaCodecInfo.nHeight = video_dec_ctx->height;
		pix_fmt = video_dec_ctx->pix_fmt;
	}

	//查找出音频源
	if (open_codec_context(&stream_isAudio, &audio_dec_ctx, pFormatCtx2, AVMEDIA_TYPE_AUDIO) >= 0)
	{
		audio_stream = pFormatCtx2->streams[stream_isAudio];
		if (audio_stream->codecpar->codec_id == AV_CODEC_ID_PCM_ALAW)
			strcpy(mediaCodecInfo.szAudioName, "G711_A");
		else if (audio_stream->codecpar->codec_id == AV_CODEC_ID_PCM_MULAW)
			strcpy(mediaCodecInfo.szAudioName, "G711_A");
		else if (audio_stream->codecpar->codec_id == AV_CODEC_ID_AAC)
			strcpy(mediaCodecInfo.szAudioName, "AAC");
		else if (audio_stream->codecpar->codec_id == AV_CODEC_ID_MP3)
			strcpy(mediaCodecInfo.szAudioName, "MP3");
		else if (audio_stream->codecpar->codec_id == AV_CODEC_ID_OPUS)
			strcpy(mediaCodecInfo.szAudioName, "OPUS");
		else
			strcpy(mediaCodecInfo.szAudioName, "UNKNOW");

		mediaCodecInfo.nSampleRate = audio_stream->codecpar->sample_rate; //采样频率
		mediaCodecInfo.nChannels = audio_stream->codecpar->ch_layout.nb_channels;
		sample_index = 8;
		for (int i = 0; i < 13; i++)
		{
			if (avpriv_mpeg4audio_sample_rates[i] == mediaCodecInfo.nSampleRate)
			{
				sample_index = i;
				break;
			}
		}

		if (audio_stream->codecpar->codec_id == AV_CODEC_ID_AAC)
		{
			if (mediaCodecInfo.nSampleRate == 48000)
				nInputAudioDelay = 21;
			else if (mediaCodecInfo.nSampleRate == 44100)
				nInputAudioDelay = 23;
			else if (mediaCodecInfo.nSampleRate == 32000)
				nInputAudioDelay = 32;
			else if (mediaCodecInfo.nSampleRate == 24000)
				nInputAudioDelay = 42;
			else if (mediaCodecInfo.nSampleRate == 22050)
				nInputAudioDelay = 49;
			else if (mediaCodecInfo.nSampleRate == 16000)
				nInputAudioDelay = 64;
			else if (mediaCodecInfo.nSampleRate == 12000)
				nInputAudioDelay = 85;
			else if (mediaCodecInfo.nSampleRate == 11025)
				nInputAudioDelay = 92;
			else if (mediaCodecInfo.nSampleRate == 8000)
				nInputAudioDelay = 128;

			mediaCodecInfo.nBaseAddAudioTimeStamp = nInputAudioDelay;
		}
	}

	if (stream_isVideo == -1)
	{
		strcpy(szReadFileError, "rtmp http-flv (h265) Video Codec Is Not Support ! ");
		WriteLog(Log_Debug, "NetClientFFmpegRecv =  %X ,nClient = %llu ，http-flv、rtmp (h265) Video Codec Is Not Support !  ", this, hClient);
		avformat_close_input(&pFormatCtx2);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}

	packet2 = av_packet_alloc();
#ifdef FFMPEG6

#else

	av_init_packet(packet2);
#endif // FFMPEG6

	if (pFormatCtx2->streams[stream_isVideo]->codecpar->extradata_size > 0)
	{
		int ret;
		codecpar = pFormatCtx2->streams[stream_isVideo]->codecpar;
		if (codecpar != NULL)
		{
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				buffersrc = (AVBitStreamFilter*)av_bsf_get_by_name("h264_mp4toannexb");
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				buffersrc = (AVBitStreamFilter*)av_bsf_get_by_name("hevc_mp4toannexb");
			ret = av_bsf_alloc(buffersrc, &bsf_ctx);
			avcodec_parameters_copy(bsf_ctx->par_in, codecpar);
			ret = av_bsf_init(bsf_ctx);
		}
	}

	//记下总时长
	if (ABL_MediaServerPort.videoFileFormat == 3)
		duration = ABL_MediaServerPort.fileSecond * 1000;
	else
		duration = video_stream->duration / 1000000;

	//确定帧速度
	mediaCodecInfo.nVideoFrameRate = video_stream->avg_frame_rate.num / video_stream->avg_frame_rate.den;

	//创建录像点播媒体源 
	pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, 0, MediaSourceType_LiveMedia, duration, m_h265ConvertH264Struct);
	if (pMediaSource == NULL)
	{
		WriteLog(Log_Debug, "NetClientFFmpegRecv 创建媒体源失败 =  %X ,nClient = %llu m_szShareMediaURL %s ", this, hClient, m_szShareMediaURL);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return;
	}
	strcpy(pMediaSource->m_mediaCodecInfo.szVideoName, mediaCodecInfo.szVideoName);
	strcpy(pMediaSource->m_mediaCodecInfo.szAudioName, mediaCodecInfo.szAudioName);
	pMediaSource->m_mediaCodecInfo.nSampleRate = mediaCodecInfo.nSampleRate; //采样频率
	pMediaSource->m_mediaCodecInfo.nChannels = mediaCodecInfo.nChannels;
	pMediaSource->m_mediaCodecInfo.nVideoFrameRate = mediaCodecInfo.nVideoFrameRate;
	if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		pMediaSource->m_mediaCodecInfo.nBaseAddAudioTimeStamp = nInputAudioDelay;

	strcpy(m_addStreamProxyStruct.app, pMediaSource->app);
	strcpy(m_addStreamProxyStruct.stream, pMediaSource->stream);
	strcpy(m_addStreamProxyStruct.url, szIP);

	nAVType = nOldAVType = AVType_Audio;
	nOldPTS = 0;
	nVidepSpeedTime = 40;
	dBaseSpeed = 40.00;
	m_dScaleValue = 1.00;
	m_bPauseFlag = false;
	m_nStartTimestamp = 0;
	nReadVideoFrameCount = nReadAudioFrameCount = 0;
	nVideoFirstPTS = 0;
	nAudioFirstPTS = 0;

	bRestoreVideoFrameFlag = false;//是否需要恢复视频帧总数
	bRestoreAudioFrameFlag = false;//是否需要恢复音频帧总数

	mov_readerTime = GetTickCount64();

#ifdef WriteAACFileFlag
	char aacFile[256] = { 0 };
	sprintf(aacFile, "%s%X.aac", ABL_MediaSeverRunPath, this);
	fWriteAAC = fopen(aacFile, "wb");
#endif 
	RecordReplayThreadPool->InsertIntoTask(nClient);
}

CNetClientFFmpegRecv::~CNetClientFFmpegRecv()
{
	if (!bResponseHttpFlag)
	{//回复代理拉流请求
		bResponseHttpFlag = true;
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Error : %s \",\"key\":%llu}", IndexApiCode_RequestFileNotFound, szReadFileError, hParent);
		ResponseHttp(nClient_http, szResponseBody, false);
	}

	WriteLog(Log_Debug, "CNetClientFFmpegRecv 析构函数 = %X ,nClient = %llu ", this, nClient);
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);

	if (pFormatCtx2 != NULL)
	{
		if (video_dec_ctx)
			avcodec_free_context(&video_dec_ctx);
		if (audio_dec_ctx)
			avcodec_free_context(&audio_dec_ctx);

		avformat_close_input(&pFormatCtx2);
		pFormatCtx2 = NULL;
		if (bsf_ctx != NULL)
			av_bsf_free(&bsf_ctx);

		av_packet_unref(packet2);
		av_packet_free(&packet2);
	}

	//删除分发源
	if (strlen(m_szShareMediaURL) > 0)
		DeleteMediaStreamSource(m_szShareMediaURL);

	m_audioCacheFifo.FreeFifo();
#ifdef WriteAACFileFlag
	fclose(fWriteAAC);
#endif 
	malloc_trim(0);
}

int CNetClientFFmpegRecv::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{

	return 0;
}

int CNetClientFFmpegRecv::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);
	nRecvDataTimerBySecond = 0;

	//修改媒体源的提供ID
	if (pMediaSource->nClient == 0 && hParent > 0)
	{
		pMediaSource->nClient = hParent;

		//修改代理拉流成功，往后拉流断线后可以反复重连 
		auto   pParentPtr = GetNetRevcBaseClient(hParent);
		if (pParentPtr && pParentPtr->bProxySuccessFlag == false)
			bProxySuccessFlag = pParentPtr->bProxySuccessFlag = true;
	}

	if (!bResponseHttpFlag && nReadVideoFrameCount >= 5)
	{//回复代理拉流请求
		bResponseHttpFlag = true;
		sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", hParent);
		ResponseHttp(nClient_http, szResponseBody, false);
	}

	nCurrentDateTime = GetTickCount64();
	if (m_bPauseFlag == true)
	{
				//Sleep(2);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		RecordReplayThreadPool->InsertIntoTask(nClient);
		return -1;
	}

	if (nWaitTime == OpenMp4FileToReadWaitMaxMilliSecond)
	{//打开mp4文件后需要等待一段事件，否则读取文件会失败
		if (nCurrentDateTime - mov_readerTime < nWaitTime)
		{
			//Sleep(2);
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			RecordReplayThreadPool->InsertIntoTask(nClient);
			return 0;
		}
	}

	if (nCurrentDateTime - mov_readerTime >= nWaitTime)
	{
		mov_readerTime = nCurrentDateTime;
		nReadRet = av_read_frame(pFormatCtx2, packet2);

		if (packet2->stream_index == stream_isVideo)
		{
			nAVType = AVType_Video;
			if (bsf_ctx != NULL)
			{//H264\H265 转换
				ret1 = av_bsf_send_packet(bsf_ctx, packet2);
				ret2 = av_bsf_receive_packet(bsf_ctx, packet2);
			}
		}
		else if (packet2->stream_index == stream_isAudio)
		{
			nAVType = AVType_Audio;

		}
	}

	if (pMediaSource->bUpdateVideoSpeed == false)
	{
		WriteLog(Log_Debug, "nClient = %llu , 更新视频源 %s 的帧速度成功，初始速度为%d ,更新后的速度为%d, ", nClient, pMediaSource->m_szURL, pMediaSource->m_mediaCodecInfo.nVideoFrameRate, 25);
		pMediaSource->UpdateVideoFrameSpeed(mediaCodecInfo.nVideoFrameRate, netBaseNetType);
		pMediaSource->bUpdateVideoSpeed = true;
	}

	if (nAVType == AVType_Video && packet2->size > 0)
	{//读取视频
		pMediaSource->PushVideo(packet2->data, packet2->size, mediaCodecInfo.szVideoName);
		nReadVideoFrameCount++;
		if (((1000 / mediaCodecInfo.nVideoFrameRate)) > 0)
		{
#ifdef  OS_System_Windows
			nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) - 8;
#else 
			nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) - 3;
#endif   
		}
		else
			nWaitTime = 1;
		nRecvDataTimerBySecond = 0;
	}
	else if (nAVType == AVType_Audio && packet2->size > 0)
	{//音频直接读取
		nWaitTime = 1;
		if (nAudioFirstPTS == 0)
			nAudioFirstPTS = packet2->pts;

		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		{
			if (packet2->size > 0 && packet2->data != NULL)
			{
				if (packet2->data[0] == 0xff && packet2->data[1] == 0xf1)
				{//已经有ff f1 
					m_audioCacheFifo.push(packet2->data, packet2->size);
				}
				else
				{
					AddADTSHeadToAAC(packet2->data, packet2->size); //增加ADTS头
#ifdef WriteAACFileFlag
					fwrite(pAACBufferADTS, 1, packet2->size + 7, fWriteAAC);
					fflush(fWriteAAC);
#endif 
					m_audioCacheFifo.push(pAACBufferADTS, +packet2->size + 7);
				}
				//获取AAC音频时间戳增量
				if (mediaCodecInfo.nBaseAddAudioTimeStamp == 0)
					mediaCodecInfo.nBaseAddAudioTimeStamp = pMediaSource->m_mediaCodecInfo.nBaseAddAudioTimeStamp;
			}
		}
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		{
			nInputAudioDelay = (packet2->size / 80) * 10;

			m_audioCacheFifo.push(packet2->data, packet2->size);

			//g711 时间戳增量
			if (mediaCodecInfo.nBaseAddAudioTimeStamp == 0)
				mediaCodecInfo.nBaseAddAudioTimeStamp = 320;
		}
	}
	av_packet_unref(packet2);

	if (nReadRet < 0)
	{//文件读取出错 
		av_strerror(nReadRet, szFFmpegErrorMsg, sizeof(szFFmpegErrorMsg));
		WriteLog(Log_Debug, "ProcessNetData 读取完毕 ,nClient = %llu \r\n%s", nClient, szFFmpegErrorMsg);
		DeleteNetRevcBaseClient(nClient);
		return -1;
	}
	nOldAVType = nAVType;
	//Sleep(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	//加入音频
	if (m_audioCacheFifo.GetSize() > 0)
	{
		nRecvDataTimerBySecond = 0;
		nInputAudioTime = nCurrentDateTime;
		unsigned char* pData = NULL;
		int            nLength = 0;

		while ((pData = m_audioCacheFifo.pop(&nLength)) != NULL)
		{
			if (pData != NULL && nLength > 0)
				pMediaSource->PushAudio(pData, nLength, mediaCodecInfo.szAudioName, mediaCodecInfo.nChannels, mediaCodecInfo.nSampleRate);

			m_audioCacheFifo.pop_front();
		}
	}

	RecordReplayThreadPool->InsertIntoTask(nClient);
	return 0;
}

//更新录像回放速度
bool CNetClientFFmpegRecv::UpdateReplaySpeed(double dScaleValue, ABLRtspPlayerType rtspPlayerType)
{
	double dCalcSpeed = 40.00;
	dCalcSpeed = (dBaseSpeed / dScaleValue);
	nVidepSpeedTime = (int)dCalcSpeed;
	m_dScaleValue = dScaleValue;
	m_rtspPlayerType = rtspPlayerType;
	WriteLog(Log_Debug, "UpdateReplaySpeed 更新录像回放速度 dScaleValue = %.2f ,nClient = %llu ,dCalcSpeed = %.2f, nVidepSpeedTime = %d , m_rtspPlayerType = %d ", dScaleValue, nClient, dCalcSpeed, nVidepSpeedTime, m_rtspPlayerType);

	return true;
}

bool CNetClientFFmpegRecv::UpdatePauseFlag(bool bFlag)
{
	m_bPauseFlag = bFlag;
	WriteLog(Log_Debug, "UpdatePauseFlag 更新暂停播放标志 ,nClient = %llu ,m_bPauseFlag = %d  ", nClient, m_bPauseFlag);
	return true;
}

bool  CNetClientFFmpegRecv::ReaplyFileSeek(uint64_t nTimestamp)
{
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);
	if (m_bPauseFlag == true)
		return false;
	if (nTimestamp > duration)
	{
		WriteLog(Log_Debug, "ReaplyFileSeek 拖动时间戳超出文件最大时长 ,nClient = %llu ,nTimestamp = %llu ,duration = %d ", nClient, nTimestamp, duration);
		return false;
	}
	int nRet = av_seek_frame(pFormatCtx2, -1, nTimestamp * 1000000, AVSEEK_FLAG_BACKWARD);

	bRestoreVideoFrameFlag = bRestoreAudioFrameFlag = true; //因为有拖到播放，需要重新计算已经播放视频，音频帧总数 
	WriteLog(Log_Debug, "ReaplyFileSeek 拖动播放 ,nClient = %llu ,nTimestamp = %llu ,nRet = %d ", nClient, nTimestamp, nRet);
}

//追加adts信息头
void  CNetClientFFmpegRecv::AddADTSHeadToAAC(unsigned char* szData, int nAACLength)
{
	int len = nAACLength + 7;
	uint8_t profile = 2;
	uint8_t sampling_frequency_index = sample_index;
	uint8_t channel_configuration = mediaCodecInfo.nChannels;
	pAACBufferADTS[0] = 0xFF; /* 12-syncword */
	pAACBufferADTS[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
	pAACBufferADTS[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
	pAACBufferADTS[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
	pAACBufferADTS[4] = (uint8_t)(len >> 3);
	pAACBufferADTS[5] = ((len & 0x07) << 5) | 0x1F;
	pAACBufferADTS[6] = 0xFC | ((len / 1024) & 0x03);

	memcpy(pAACBufferADTS + 7, szData, nAACLength);
}

int CNetClientFFmpegRecv::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{

	return 0;
}

int CNetClientFFmpegRecv::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{

	return 0;
}

int CNetClientFFmpegRecv::SendVideo()
{

	return 0;
}

int CNetClientFFmpegRecv::SendAudio()
{

	return 0;
}

int CNetClientFFmpegRecv::SendFirstRequst()
{

	return 0;
}

bool CNetClientFFmpegRecv::RequestM3u8File()
{

	return true;
}

