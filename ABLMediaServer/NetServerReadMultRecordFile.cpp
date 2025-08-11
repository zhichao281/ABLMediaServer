/*
���ܣ�
        ������ȡ���¼���ļ����γ�ý����
����    2024-04-14
����    �޼��ֵ�
QQ      79941308    
E-Mail  79941308@qq.com
*/
#include "stdafx.h"
#include "NetServerReadMultRecordFile.h"

extern CNetBaseThreadPool*                   RecordReplayThreadPool;//¼��ط��̳߳�
extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);

#ifdef USE_BOOST

extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);

#else
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);

#endif


extern bool                                  DeleteMediaStreamSource(char* szURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern char                                  ABL_MediaSeverRunPath[256] ; //��ǰ·��
extern  uint64_t                             GetCurrentSecondByTime(char* szDateTime);
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern int avpriv_mpeg4audio_sample_rates[];
extern CMediaFifo                            pDisconnectMediaSource;      //�������ý��Դ 

#ifdef OS_System_Windows
extern BOOL GBK2UTF8(char *szGbk, char *szUtf8, int Len);
#else
extern int GB2312ToUTF8(char* szSrc, size_t iSrcLen, char* szDst, size_t iDstLen);
#endif

//�ӻطŵ�¼�����ֻ�ȡ�㲥����url 
bool  CNetServerReadMultRecordFile::GetMediaShareURLFromFileName(char* szRecordFileName,char* szMediaURL)
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
		memcpy(szTempFileName, szRecordFileName + nPos+1, strlen(szRecordFileName) - nPos);
		szTempFileName[strlen(szTempFileName) - 4] = 0x00;
		sprintf(m_szShareMediaURL, "%s%s%s", szMediaURL, RecordFileReplaySplitter, szTempFileName);
		return true;
	}else 
 	  return false;
}

//������Ƶ����Ƶ��ʽ
int CNetServerReadMultRecordFile::open_codec_context(int *stream_idx,AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream *st;
	const AVCodec *dec = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		WriteLog(Log_Debug,"Could not find %s stream in input file '%s'\n",av_get_media_type_string(type), szFileNameUTF8);
		return ret;
	}
	else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec)
		{
			WriteLog(Log_Debug, "Failed to find %s codec\n",av_get_media_type_string(type));
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

CNetServerReadMultRecordFile::CNetServerReadMultRecordFile(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	memset(szCurrentDateTime, 0x00, sizeof(szCurrentDateTime));
    nStartSeekTime = GetTickCount64();//��ʼseekʱ���
	bSeekTimeState = false ;//������Seek״̬ʱ�䷶Χ��
	nSeekSecond = oldDTS = 0   ;
 	WriteLog(Log_Debug, "CNetServerReadMultRecordFile ���캯�� = %X ,nClient = %llu , m_szShareMediaURL = %s  ", this, hClient, szShareMediaURL);
	netBaseNetType = NetBaseNetType_NetServerReadMultRecordFile;
	nCurrrentPlayerOrder = 0;
	nLastMp4FileDuration = 1;
	nFirstTimestamp = -1;

	nClient = hClient;
	nServer = hServer;
	strcpy(szReadFileError, "Unknow Error .");
	bResponseHttpFlag = false;
	video_dec_ctx = NULL;
	audio_dec_ctx = NULL;
	video_stream = NULL;
	audio_stream = NULL;

	strcpy(m_szShareMediaURL, szShareMediaURL);
	memset(szFileNameUTF8, 0x00, sizeof(szFileNameUTF8));
	nWaitTime = OpenMp4FileToReadMultWaitMaxMilliSecond;
	stream_isVideo = -1;
	stream_isAudio = -1;
	buffersrc = NULL;
	bsf_ctx = NULL;
	sample_index = 8 ;
	m_audioCacheFifo.InitFifo(1024 * 256);
	nInputAudioDelay = 20;
	nInputAudioTime = nCurrentDateTime = GetTickCount64();

	nDownloadFrameCount = 0;
	SplitterAppStream(m_szShareMediaURL);

	m_rtspPlayerType = RtspPlayerType_RecordReplay;
	pMediaSource = NULL ;
	nClient      = hClient;
	pFormatCtx2  = NULL;
	packet2      = NULL;

#ifdef OS_System_Windows 
	GBK2UTF8(szIP, szFileNameUTF8, sizeof(szFileNameUTF8));
#else
	GB2312ToUTF8(szIP, strlen(szIP), szFileNameUTF8, sizeof(szFileNameUTF8));
#endif

#ifdef WriteAACFileFlag
	char aacFile[256] = { 0 };
	sprintf(aacFile, "%s%X.aac", ABL_MediaSeverRunPath, this);
	fWriteAAC = fopen(aacFile,"wb");
#endif 
 	RecordReplayThreadPool->InsertIntoTask(nClient);
}

bool  CNetServerReadMultRecordFile::StartReadMp4File()
{
	if (pFormatCtx2 != NULL)
		return true ;

	WriteLog(Log_Debug, "NetServerReadMultRecordFile =  %X ,nClient = %llu ��ʼ��ȡ¼���ļ� %s ", this, nClient, szFileNameUTF8);

	if (avformat_open_input(&pFormatCtx2, szFileNameUTF8, NULL, NULL) != 0)
	{
		strcpy(szReadFileError, "open file error ! ");
		WriteLog(Log_Debug, "NetServerReadMultRecordFile =  %X ,nClient = %llu ��ȡ�ļ�ʧ�� ", this, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return false ;
	}

	//ȷ���Ƿ���ý��Դ
	if (avformat_find_stream_info(pFormatCtx2, NULL) < 0)
	{
		strcpy(szReadFileError, "file is Not Media File ! ");
		avformat_close_input(&pFormatCtx2);
		WriteLog(Log_Debug, "NetServerReadMultRecordFile =  %X ,nClient = %llu �ļ��в�������Ƶ����Ƶ��  ", this, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return false ;
	}

	//���ҳ���ƵԴ
	if (open_codec_context(&stream_isVideo, &video_dec_ctx, pFormatCtx2, AVMEDIA_TYPE_VIDEO) >= 0)
	{
		video_stream = pFormatCtx2->streams[stream_isVideo];
		if (video_stream->codecpar->codec_id == AV_CODEC_ID_H264)
			strcpy(mediaCodecInfo.szVideoName, "H264");
		else if (video_stream->codecpar->codec_id == AV_CODEC_ID_H265)
			strcpy(mediaCodecInfo.szVideoName, "H265");
		else
		{
			strcpy(szReadFileError, "Video Codec Is Not Support ! ");
			WriteLog(Log_Debug, "NetServerReadMultRecordFile =  %X ,nClient = %llu ��video_stream->codecpar->codec_id = %d ��Ƶ��ʽ����H264��H265 ", this, nClient, video_stream->codecpar->codec_id);
			avformat_close_input(&pFormatCtx2);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return false ;
		}

		mediaCodecInfo.nWidth = video_dec_ctx->width;
		mediaCodecInfo.nHeight = video_dec_ctx->height;
		pix_fmt = video_dec_ctx->pix_fmt;
	}
	else
	{
		avformat_close_input(&pFormatCtx2);
		WriteLog(Log_Debug, "NetServerReadMultRecordFile =  %X ,nClient = %llu �ļ��в�������Ƶ����Ƶ��  ", this, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
	    return false ; 
	}

	//���ҳ���ƵԴ
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

		mediaCodecInfo.nSampleRate = audio_stream->codecpar->sample_rate; //����Ƶ��
#ifdef FFMPEG6
		mediaCodecInfo.nChannels = audio_stream->codecpar->ch_layout.nb_channels;
#else

		mediaCodecInfo.nChannels = audio_stream->codecpar->channels;
#endif // FFMPEG6

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
				buffersrc = (AVBitStreamFilter *)av_bsf_get_by_name("h264_mp4toannexb");
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				buffersrc = (AVBitStreamFilter *)av_bsf_get_by_name("hevc_mp4toannexb");
			ret = av_bsf_alloc(buffersrc, &bsf_ctx);
			avcodec_parameters_copy(bsf_ctx->par_in, codecpar);
			ret = av_bsf_init(bsf_ctx);
		}
	}

	//������ʱ��
	duration = pFormatCtx2->duration / 1000000;

	//ȷ��֡�ٶ�
	if (video_stream->avg_frame_rate.den != 0)
		mediaCodecInfo.nVideoFrameRate = 25;// video_stream->avg_frame_rate.num / video_stream->avg_frame_rate.den;

	strcpy(m_addStreamProxyStruct.url, szFileNameUTF8);

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

	bRestoreVideoFrameFlag = false;//�Ƿ���Ҫ�ָ���Ƶ֡����
	bRestoreAudioFrameFlag = false;//�Ƿ���Ҫ�ָ���Ƶ֡����

	mov_readerTime = GetTickCount64();

	//����ƫ�Ƶ�������
	char  szFirstRecordFileTime[string_length_256] = { 0 };
	char  szStartPlayerTime[string_length_256] = { 0 };
	char  szEndPlayerTime[string_length_256] = { 0 };

	sprintf(szFirstRecordFileTime, "%llu", nServer);
	memcpy(szStartPlayerTime, m_szShareMediaURL + (strlen(m_szShareMediaURL) - 29), 14);
	memcpy(szEndPlayerTime, m_szShareMediaURL + (strlen(m_szShareMediaURL) - 14), 14);
	uint64_t  nTime1, nTime2, nTime3;
	nTime1 = GetCurrentSecondByTime(szFirstRecordFileTime);
	nStartPlayerSecond = nTime2 = GetCurrentSecondByTime(szStartPlayerTime);
	nStopPlayerSecond = nTime3 = GetCurrentSecondByTime(szEndPlayerTime);
	duration = nTime3 - nTime2;
	nSeekSecond = nTime2 - nTime1;
	if (nSeekSecond > 0)
	{
		int seekret = av_seek_frame(pFormatCtx2, -1, nSeekSecond * AV_TIME_BASE + pFormatCtx2->start_time, AVSEEK_FLAG_BACKWARD);
		WriteLog(Log_Debug, "CNetServerReadMultRecordFile = %X ,nClient = %llu , Seek %d �� , ok %d , start_time %d ", this, nClient, nSeekSecond, seekret, pFormatCtx2->start_time);
	}

	WriteLog(Log_Debug, "CNetServerReadMultRecordFile ���ŵ�ǰ�ļ� [ %d/%d ] ,nClient = %llu , ReadNextRecordFile = %s  ", nCurrrentPlayerOrder + 1, mutliRecordPlayNameList.size(), nClient, szFileNameUTF8);

	return true;
}

CNetServerReadMultRecordFile::~CNetServerReadMultRecordFile() 
{
	if (!bResponseHttpFlag)
	{//�ظ�������������
		bResponseHttpFlag = true;
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Error : %s \",\"key\":%llu}", IndexApiCode_RequestFileNotFound, szReadFileError,hParent);
		ResponseHttp(nClient_http, szResponseBody, false);
	}
 
 	WriteLog(Log_Debug, "CNetServerReadMultRecordFile �������� = %X ,nClient = %llu ", this, nClient);
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);

	CloseRecordFile();
	if (packet2 != NULL)
	{
		av_packet_unref(packet2);
		av_packet_free(&packet2);
	}
	//ɾ���ַ�Դ
	if (strlen(m_szShareMediaURL) > 0)
 	  pDisconnectMediaSource.push((unsigned char*)m_szShareMediaURL, strlen(m_szShareMediaURL));

	if(hParent > 0 )
		pDisconnectBaseNetFifo.push((unsigned char*)&hParent,sizeof(hParent));

	m_audioCacheFifo.FreeFifo();
#ifdef WriteAACFileFlag
 	fclose(fWriteAAC);
#endif 
   malloc_trim(0);
}

int CNetServerReadMultRecordFile::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{

  return 0 ;	
}

int CNetServerReadMultRecordFile::ProcessNetData() 
{
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);
	nRecvDataTimerBySecond = 0;

	//��ʼ��ȡ mp4 �ļ�
	if (StartReadMp4File() == false )
		return 0; //��ȡ�ļ���������ɾ�� 

	//����¼��㲥ý��Դ 
	if (pMediaSource == NULL)
	{
		pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_ReplayMedia, duration, m_h265ConvertH264Struct);
		if (pMediaSource == NULL)
		{
			WriteLog(Log_Debug, "NetServerReadMultRecordFile ����ý��Դʧ�� =  %X ,nClient = %llu m_szShareMediaURL %s ", this, nClient, m_szShareMediaURL);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}
		pMediaSource->netBaseNetType = NetBaseNetType_NetServerReadMultRecordFile;
		strcpy(pMediaSource->m_mediaCodecInfo.szVideoName, mediaCodecInfo.szVideoName);
		strcpy(pMediaSource->m_mediaCodecInfo.szAudioName, mediaCodecInfo.szAudioName);
		pMediaSource->m_mediaCodecInfo.nSampleRate = mediaCodecInfo.nSampleRate; //����Ƶ��
		pMediaSource->m_mediaCodecInfo.nChannels = mediaCodecInfo.nChannels;
		pMediaSource->m_mediaCodecInfo.nVideoFrameRate = mediaCodecInfo.nVideoFrameRate;
		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			pMediaSource->m_mediaCodecInfo.nBaseAddAudioTimeStamp = nInputAudioDelay;

		strcpy(m_addStreamProxyStruct.app, pMediaSource->app);
		strcpy(m_addStreamProxyStruct.stream, pMediaSource->stream);

		SplitterAppStream(m_szShareMediaURL);
	}

	if (!bResponseHttpFlag && nReadVideoFrameCount >= 5)
	{//�ظ�������������
		bResponseHttpFlag = true;
		sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", hParent);
		ResponseHttp(nClient_http, szResponseBody, false);
	}

	nCurrentDateTime = GetTickCount64();
	if (m_bPauseFlag == true )
	{
		//Sleep(2);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
 		RecordReplayThreadPool->InsertIntoTask(nClient);
		return -1;
	}

	if (nWaitTime == OpenMp4FileToReadMultWaitMaxMilliSecond)
	{//��mp4�ļ�����Ҫ�ȴ�һ���¼��������ȡ�ļ���ʧ��
		if (nCurrentDateTime - mov_readerTime < nWaitTime)
		{
		//	Sleep(2);
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			RecordReplayThreadPool->InsertIntoTask(nClient);
			return 0;
		}
	}

    if(nCurrentDateTime - mov_readerTime >= nWaitTime && pFormatCtx2 )
	{
	   mov_readerTime = nCurrentDateTime ;
	   nReadRet = av_read_frame(pFormatCtx2, packet2);

	   if (packet2->stream_index == stream_isVideo)
	   {
 		   nAVType = AVType_Video;
		   if (bsf_ctx != NULL)
		   {//H264\H265 ת��
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
		WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", nClient, pMediaSource->m_szURL, pMediaSource->m_mediaCodecInfo.nVideoFrameRate, 25);
		pMediaSource->UpdateVideoFrameSpeed(mediaCodecInfo.nVideoFrameRate, netBaseNetType);
		pMediaSource->bUpdateVideoSpeed = true;
	}

	if (bSeekTimeState == true)
	{//������Seek״̬ 
		if (GetTickCount64() - nStartSeekTime >= 1000 * 3) //����3������Ϊ������Seek״̬ 
			bSeekTimeState = false;
	}

	if (nAVType == AVType_Video && packet2->size > 0 && nReadRet == 0)
	{//��ȡ��Ƶ
		if(packet2->dts > 0 )
	      oldDTS = packet2->dts;

		memcpy(s_packet, (unsigned char*)&nReadVideoFrameCount, sizeof(nReadVideoFrameCount));
		memcpy(s_packet + sizeof(nReadVideoFrameCount), packet2->data, packet2->size);
		if (abs(m_dScaleValue - 8.0) <= 0.01 || abs(m_dScaleValue - 16.0) <= 0.01)
		{//��֡
			if (m_rtspPlayerType == RtspPlayerType_RecordReplay)
			{//¼��ط�
				if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, packet2->data, packet2->size))
					pMediaSource->PushVideo(s_packet, packet2->size+4 , mediaCodecInfo.szVideoName);
			}
			else //¼������
				pMediaSource->PushVideo(s_packet, packet2->size+4, mediaCodecInfo.szVideoName);
		}
		else
			pMediaSource->PushVideo(s_packet, packet2->size+4, mediaCodecInfo.szVideoName);

		nReadVideoFrameCount ++;

		if (nFirstTimestamp == -1 && packet2)
			nFirstTimestamp = packet2->pts;

		if (nCurrrentPlayerOrder == mutliRecordPlayNameList.size() - 1 && (nReadVideoFrameCount / mediaCodecInfo.nVideoFrameRate) >= (duration + 3))
		{//���һ��mp4�ļ��������Ѿ��ﵽ���һ���ļ��Ĳ���ʱ�� 
			WriteLog(Log_Debug, "ProcessNetData ���һ��mp4�ļ��������Ѿ��ﵽ���һ���ļ��Ĳ���ʱ�� %d �룬��Ҫִ�йر� ,nClient = %llu ", nLastMp4FileDuration, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		} 

 		if ((abs(m_dScaleValue - 8.0) <= 0.01 || abs(m_dScaleValue - 16.0) <= 0.01))
		{//8��16���ٲ���Ҫ�ȴ� 
			if (m_rtspPlayerType == RtspPlayerType_RecordReplay)
			{//¼��ط�
				if (nAVType == AVType_Video && nOldAVType == AVType_Video)
				{
				  if (abs(m_dScaleValue - 8.0) <= 0.01)
 					nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 8;
 				  else if (abs(m_dScaleValue - 16.0) <= 0.01)
 					nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 16;
				 }
				 else 
					nWaitTime = 1;
 			}
			else//¼������
			{
			  if (abs(m_dScaleValue - 8.0) <= 0.01)
				 nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 8 ;
			  else if (abs(m_dScaleValue - 16.0) <= 0.01)
				  nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 16 ;
			}
		}
 		else if (abs(m_dScaleValue - 255.0) <= 0.01 )
		{//rtsp¼������
            nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 16 ;
 		}
		else if (abs(m_dScaleValue - 1.0) <= 0.01)
		{//1����
			if (((1000 / mediaCodecInfo.nVideoFrameRate)) > 0)
			{
#ifdef  OS_System_Windows
				nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) - 8;
#else 
				nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) ;
#endif   
			}
			else
				nWaitTime = 1;
 		}else if (abs(m_dScaleValue - 2.0) <= 0.01)
		{//2����
			if (nAVType == AVType_Video && nOldAVType == AVType_Video)
			{
#ifdef  OS_System_Windows
				nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 2 - 5 ;
 #else 
				nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 2 - 3;
#endif   
	        }
		    else
			  nWaitTime = 1;
 		}else if (abs(m_dScaleValue - 4.0) <= 0.01)
		{//4����
			if (nAVType == AVType_Video && nOldAVType == AVType_Video)
			{
#ifdef  OS_System_Windows
			 nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 4 - 3;
#else 
			 nWaitTime = ((1000 / mediaCodecInfo.nVideoFrameRate)) / 4 - 3;
#endif   
			}
			else
				nWaitTime = 1;
	    }
		else if ( m_dScaleValue >= 0.125 &&  m_dScaleValue < 1 )
		{//���ٲ���
			nWaitTime = (1000 / mediaCodecInfo.nVideoFrameRate) / m_dScaleValue ;
		}
		else 
		{//��ȡ��Ƶ��ʱ����δ������ҪSleep(2) ,����CPU�����
			  if ( !(abs(m_dScaleValue - 8.0) <= 0.01 || abs(m_dScaleValue - 16.0) <= 0.01) )
			   nWaitTime = 5 ; //8���١�16���٣�����ҪSleeep
 		}
 	}
	else if (nAVType == AVType_Audio && packet2->size > 0 && nReadRet == 0)
	{//��Ƶֱ�Ӷ�ȡ
		 nWaitTime = 1;
 		if (nAudioFirstPTS == 0)
			nAudioFirstPTS = packet2->pts;

		if (bRestoreAudioFrameFlag && mediaCodecInfo.nBaseAddAudioTimeStamp != 0)
		{//���¼�����Ƶ֡���� ����Ϊ�Ѿ��ϵ�����
			nReadAudioFrameCount = ((packet2->pts - nAudioFirstPTS) /mediaCodecInfo.nBaseAddAudioTimeStamp);
			bRestoreAudioFrameFlag = false;
		}
		else
			nReadAudioFrameCount++;

		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
		{
 			if (packet2->size > 0 && packet2->data != NULL)
			{
				if (packet2->data[0] == 0xff && packet2->data[1] == 0xf1)
				{//�Ѿ���ff f1 
					memcpy(audioBuffer, (unsigned char*)&nReadAudioFrameCount, sizeof(nReadAudioFrameCount));
					memcpy(audioBuffer + 4, packet2->data, packet2->size );
					m_audioCacheFifo.push(audioBuffer, 4 + packet2->size );
				}
				else
				{
					memcpy(audioBuffer, (unsigned char*)&nReadAudioFrameCount, sizeof(nReadAudioFrameCount));
					AddADTSHeadToAAC(packet2->data, packet2->size); //����ADTSͷ
#ifdef WriteAACFileFlag
					fwrite(pAACBufferADTS, 1, packet2->size + 7, fWriteAAC);
					fflush(fWriteAAC);
#endif 
					memcpy(audioBuffer + 4, pAACBufferADTS, packet2->size + 7);
					m_audioCacheFifo.push(audioBuffer, 4 + packet2->size + 7);
				}
				//��ȡAAC��Ƶʱ�������
				if (mediaCodecInfo.nBaseAddAudioTimeStamp == 0)
					mediaCodecInfo.nBaseAddAudioTimeStamp = pMediaSource->m_mediaCodecInfo.nBaseAddAudioTimeStamp;
			}
		}
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		{
			nInputAudioDelay = (packet2->size / 80) * 10;

			memcpy(audioBuffer, (unsigned char*)&nReadAudioFrameCount, sizeof(nReadAudioFrameCount));
			memcpy(audioBuffer + 4, packet2->data, packet2->size);
 			m_audioCacheFifo.push(audioBuffer, 4 + packet2->size);

			//g711 ʱ�������
			if (mediaCodecInfo.nBaseAddAudioTimeStamp == 0)
				mediaCodecInfo.nBaseAddAudioTimeStamp = 320;
		} 
	}
	av_packet_unref(packet2);

	if (nReadRet < 0)
	{//�ļ���ȡ���� 
 		//��ȡ��һ���ļ�
		if(nCurrrentPlayerOrder < mutliRecordPlayNameList.size() - 1 )
			nCurrrentPlayerOrder ++;

		if (ReadNextRecordFile())
		{
			RecordReplayThreadPool->InsertIntoTask(nClient);
			Sleep(1);
			return 0;
		}
		else
		{
			WriteLog(Log_Debug, "ProcessNetData �ļ���ȡ���� ,nClient = %llu ", nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -1;
		}
	}
	nOldAVType = nAVType;
	//Sleep(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));

	//������Ƶ
	if (nCurrentDateTime - nInputAudioTime >= nInputAudioDelay - 5)
	{
		nInputAudioTime = nCurrentDateTime ;
		unsigned char* pData = NULL;
		int            nLength = 0;
		int            nSize = 0 ;
 
		nSize = m_audioCacheFifo.GetSize();
		while (nSize >= 2)
		{
		  pData = m_audioCacheFifo.pop(&nLength);
		  if (pData != NULL && nLength > 0)
		  {
			pMediaSource->PushAudio(pData, nLength, mediaCodecInfo.szAudioName, mediaCodecInfo.nChannels, mediaCodecInfo.nSampleRate);
		 	m_audioCacheFifo.pop_front();
		  }
		  nSize = m_audioCacheFifo.GetSize();
		}
	} 

	RecordReplayThreadPool->InsertIntoTask(nClient);
    return 0 ;	
}

bool  CNetServerReadMultRecordFile::CloseRecordFile()
{
	if (pFormatCtx2 != NULL)
	{
		if (video_dec_ctx)
			avcodec_free_context(&video_dec_ctx);
		if (audio_dec_ctx)
			avcodec_free_context(&audio_dec_ctx);

		avformat_close_input(&pFormatCtx2);
		if (bsf_ctx != NULL)
			av_bsf_free(&bsf_ctx);

 		pFormatCtx2 = NULL;
 		video_dec_ctx = NULL;
		audio_dec_ctx = NULL;
		return true;
	}else 
	   return false ;
}

//��ȡ��һ��¼���ļ�
bool  CNetServerReadMultRecordFile::ReadNextRecordFile()
{
BeginReadFile:
	CloseRecordFile();
	nFirstTimestamp = -1;
	memset(szFileNameUTF8, 0x00, sizeof(szFileNameUTF8));
	memset(szCurrentFileName, 0x00, sizeof(szCurrentFileName));

	if (nCurrrentPlayerOrder >= mutliRecordPlayNameList.size() || nCurrrentPlayerOrder < 0 )
		return false;
 	strcpy(szCurrentFileName, mutliRecordPlayNameList[nCurrrentPlayerOrder].c_str());
	memcpy(szCurrentDateTime, szCurrentFileName + (strlen(szCurrentFileName) - 18), 14);
	nCurrentFileDateTime = GetCurrentSecondByTime(szCurrentDateTime);
	nReadVideoFrameCount = (nCurrentFileDateTime - nStartPlayerSecond) * mediaCodecInfo.nVideoFrameRate;
 	WriteLog(Log_Debug, "CNetServerReadMultRecordFile ���ŵ�ǰ�ļ� [ %d/%d ] ,nClient = %llu ,��ǰ�ļ������� %llu �� ��ǰ��Ƶ֡���� %d, ReadNextRecordFile = %s  ", nCurrrentPlayerOrder + 1, mutliRecordPlayNameList.size(), nClient, nCurrentFileDateTime, nReadVideoFrameCount, szCurrentFileName);

#ifdef OS_System_Windows 
	GBK2UTF8(szCurrentFileName, szFileNameUTF8, sizeof(szFileNameUTF8));
#else
	GB2312ToUTF8(szCurrentFileName, strlen(szCurrentFileName), szFileNameUTF8, sizeof(szFileNameUTF8));
#endif
	pFormatCtx2 = NULL;
	if (avformat_open_input(&pFormatCtx2, szFileNameUTF8, NULL, NULL) != 0)
	{
		strcpy(szReadFileError, "open file error ! ");
		if (nCurrrentPlayerOrder >= mutliRecordPlayNameList.size() - 1)
		{
 		   pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		   return false ;
		}
		else
		{//��ǰ�ļ���ȡ������д��һ��
			nCurrrentPlayerOrder ++;
			goto BeginReadFile;
		}
	}

	//ȷ���Ƿ���ý��Դ
	if (avformat_find_stream_info(pFormatCtx2, NULL) < 0)
	{
		if (nCurrrentPlayerOrder >= mutliRecordPlayNameList.size() - 1)
		{
  		   pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		   return false ;
 		}
		else
		{//��ǰ�ļ���ȡ������д��һ��
 			nCurrrentPlayerOrder ++;
			goto BeginReadFile;
		}
	}

	//���ҳ���ƵԴ
	if (open_codec_context(&stream_isVideo, &video_dec_ctx, pFormatCtx2, AVMEDIA_TYPE_VIDEO) >= 0)
	{
		video_stream = pFormatCtx2->streams[stream_isVideo];
		if (video_stream->codecpar->codec_id == AV_CODEC_ID_H264)
			strcpy(mediaCodecInfo.szVideoName, "H264");
		else if (video_stream->codecpar->codec_id == AV_CODEC_ID_H265)
			strcpy(mediaCodecInfo.szVideoName, "H265");
		else
		{//��ǰ�ļ���ȡ������д��һ��
 			avformat_close_input(&pFormatCtx2);
			nCurrrentPlayerOrder++;
			goto BeginReadFile;
		}
	}
	else
	{
		avformat_close_input(&pFormatCtx2);
		WriteLog(Log_Debug, "NetServerReadMultRecordFile =  %X ,nClient = %llu �ļ��в�������Ƶ����Ƶ��  ", this, nClient);
	    pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient)); //������ѵ����� 
		return false ;
	}

	//���²���ת����
	if (stream_isVideo >=0 && pFormatCtx2->streams[stream_isVideo]->codecpar->extradata_size > 0)
	{
		int ret;
		codecpar = pFormatCtx2->streams[stream_isVideo]->codecpar;
		if (codecpar != NULL)
		{
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				buffersrc = (AVBitStreamFilter *)av_bsf_get_by_name("h264_mp4toannexb");
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				buffersrc = (AVBitStreamFilter *)av_bsf_get_by_name("hevc_mp4toannexb");
			ret = av_bsf_alloc(buffersrc, &bsf_ctx);
			avcodec_parameters_copy(bsf_ctx->par_in, codecpar);
			ret = av_bsf_init(bsf_ctx);
		}
	}

	return true;
}

//����¼��ط��ٶ�
bool CNetServerReadMultRecordFile::UpdateReplaySpeed(double dScaleValue, ABLRtspPlayerType rtspPlayerType)
{
	double dCalcSpeed = 40.00;
	dCalcSpeed = (dBaseSpeed / dScaleValue);
	nVidepSpeedTime = (int)dCalcSpeed;
	m_dScaleValue = dScaleValue;
	m_rtspPlayerType = rtspPlayerType;
	WriteLog(Log_Debug, "UpdateReplaySpeed ����¼��ط��ٶ� dScaleValue = %.2f ,nClient = %llu ,dCalcSpeed = %.2f, nVidepSpeedTime = %d , m_rtspPlayerType = %d ", dScaleValue, nClient, dCalcSpeed, nVidepSpeedTime, m_rtspPlayerType);

	return true;
}

bool CNetServerReadMultRecordFile::UpdatePauseFlag(bool bFlag)
{
	m_bPauseFlag = bFlag;
	WriteLog(Log_Debug, "UpdatePauseFlag ������ͣ���ű�־ ,nClient = %llu ,m_bPauseFlag = %d  ", nClient, m_bPauseFlag);
	return true;
}

/*
 ���ܣ�������ק�������� ����λ���ĸ��ļ���ţ��ص�����ǰ�ļ���������
 ������
    int        nSeekSecond,           ��ק����������
	uint32_t&  nSecondOfCurrentFile   ������Ҫ��ק�����ļ���ָ���������� 
	bool       bSeekFlag              �Ƿ���ҪSeek���� 
 ����ֵ��
    int                               ��Ҫ��ק�����ļ����   
*/
int  CNetServerReadMultRecordFile::GetFileOrderBySecond(int nSeekSecond, uint32_t& nSecondOfCurrentFile,bool& bSeekFlag)
{
	int    nSize = mutliRecordPlayNameList.size();
	int    nOrder = -1;
	char   szTempFileName[string_length_512] = { 0 };
 
	bSeekFlag = false;
	//������ʼʱ�䡢����ʱ��
	if (nStartPlayerSecond + nSeekSecond > nStopPlayerSecond)
		return nOrder; 

	nSecondOfCurrentFile = 0; 
	for (int i = 0; i < nSize; i++)
	{
		memcpy(szTempFileName, mutliRecordPlayNameList[i].c_str() + (mutliRecordPlayNameList[i].size() - 18), 14);

		if ((nStartPlayerSecond + nSeekSecond) >= GetCurrentSecondByTime(szTempFileName) &&
			(nStartPlayerSecond + nSeekSecond) <= GetCurrentSecondByTime(szTempFileName) + ABL_MediaServerPort.fileSecond)
		{
			nOrder = i;
			bSeekFlag = true;

			if (nOrder != nSize - 1)
			{//�������һ���ļ�
			   if(nStartPlayerSecond > GetCurrentSecondByTime(szTempFileName))
			      nSecondOfCurrentFile = nStartPlayerSecond  - GetCurrentSecondByTime(szTempFileName) ;
			  else 
				  nSecondOfCurrentFile = (nStartPlayerSecond + nSeekSecond) - GetCurrentSecondByTime(szTempFileName);
			}
			else
				nSecondOfCurrentFile = (nStartPlayerSecond + nSeekSecond ) -  GetCurrentSecondByTime(szTempFileName) ;

 			break;
		}
	}

	//�Ҳ�����׼��mp4�ļ��������������mp4�ļ�
	if (nOrder == -1)
	{
		for (int i = 0; i < nSize; i++)
		{
			memcpy(szTempFileName, mutliRecordPlayNameList[i].c_str() + (mutliRecordPlayNameList[i].size() - 18), 14);

			if (GetCurrentSecondByTime(szTempFileName) >= (nStartPlayerSecond + nSeekSecond))
			{//
				nOrder = i;
				nSecondOfCurrentFile = GetCurrentSecondByTime(szTempFileName) - nStartPlayerSecond;
				bSeekFlag = false;
				break;
			}
		}
	}

	return nOrder;
}

bool  CNetServerReadMultRecordFile::ReaplyFileSeek(uint64_t nTimestamp)
{
	std::lock_guard<std::mutex> lock(readRecordFileInputLock);
	if ( m_bPauseFlag == true || pFormatCtx2 == NULL )
 		return false;

	uint32_t       nCurrentFileSeconds = 0;
	bool           bSeekFlag = false;
	int nTempOrder = GetFileOrderBySecond(nTimestamp, nCurrentFileSeconds,bSeekFlag);
	if (nTempOrder >= mutliRecordPlayNameList.size() || nTempOrder <  0 )
	{
		WriteLog(Log_Debug, "ReaplyFileSeek �϶�ʱ��������ļ����ʱ�� ,nClient = %llu ,nTimestamp = %llu ,duration = %d ", nClient, nTimestamp, ABL_MediaServerPort.fileSecond * mutliRecordPlayNameList.size());
		return false;
	}
	if (nCurrrentPlayerOrder != nTempOrder)
	{
	  nCurrrentPlayerOrder = nTempOrder ;
	  ReadNextRecordFile(); 
 	}
	int  nSeek = nCurrentFileSeconds;
	oldDTS = 0;
	nStartSeekTime = GetTickCount64();//��ʼseekʱ���
	bSeekTimeState = true ;//������Seek״̬ʱ�䷶Χ��

    //�Ѿ�����֡����
	if(bSeekFlag )
   	   nReadVideoFrameCount = nTimestamp * mediaCodecInfo.nVideoFrameRate;

	int nRet = 0;
	if (pFormatCtx2 != NULL && bSeekFlag == true)
	{
		if (nCurrrentPlayerOrder == 0)
			nRet = av_seek_frame(pFormatCtx2, -1, (nSeekSecond + nTimestamp - 1) * AV_TIME_BASE + pFormatCtx2->start_time, AVSEEK_FLAG_BACKWARD);
		else
	        nRet = av_seek_frame(pFormatCtx2, -1, (nSeek ) * AV_TIME_BASE + pFormatCtx2->start_time , AVSEEK_FLAG_BACKWARD);
	}
  
	bRestoreVideoFrameFlag = bRestoreAudioFrameFlag = true; //��Ϊ���ϵ����ţ���Ҫ���¼����Ѿ�������Ƶ����Ƶ֡���� 
	WriteLog(Log_Debug, "ReaplyFileSeek �϶����� ,nClient = %llu ,nTimestamp = %llu ,nRet = %d ,��ǰmp4�ļ���� nCurrrentPlayerOrder = %d ,nSeek = %d ��", nClient, nTimestamp, nRet, nCurrrentPlayerOrder, nSeek);
}

//׷��adts��Ϣͷ
void  CNetServerReadMultRecordFile::AddADTSHeadToAAC(unsigned char* szData, int nAACLength)
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

int CNetServerReadMultRecordFile::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)  
{

  return 0 ;	
}

int CNetServerReadMultRecordFile::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)  
{

  return 0 ;	
}

int CNetServerReadMultRecordFile::SendVideo() 
{

  return 0 ;	
}

int CNetServerReadMultRecordFile::SendAudio() 
{

  return 0 ;	
}

int CNetServerReadMultRecordFile::SendFirstRequst() 
{

  return 0 ;	
}

bool CNetServerReadMultRecordFile::RequestM3u8File() 
{
 
  return true ;	
}

//�������һ��MP4�ļ�ʵ����Ҫ���ŵ�ʱ�������������һ���ļ�ȫ��������� 
int  CNetServerReadMultRecordFile::CalcLastMp4FileDuration()
{
	nLastMp4FileDuration = 1;
	int64_t  nTime_start, nTime_end, nTime_last_file;
	int      nPos = 0;
	char     szLastFileName[string_length_512] = { 0 };
	// ȡ���һ���ļ����� nTime1

	std::string lastFilePath = mutliRecordPlayNameList.back();
#ifdef OS_System_Windows
	nPos = lastFilePath.rfind("\\", lastFilePath.size());
#else 
	nPos = lastFilePath.rfind("/", lastFilePath.size());
#endif
	if (nPos > 0)
 		memcpy(szLastFileName, lastFilePath.c_str() + nPos + 1, lastFilePath.size() - nPos - 1 - 4);
 
	nTime_last_file = GetCurrentSecondByTime(szLastFileName);

	nTime_end = GetCurrentSecondByTime(m_queryRecordListStruct.endtime);

	nTime_start = GetCurrentSecondByTime(m_queryRecordListStruct.starttime);

	// ������ʱ�� 
	if (mutliRecordPlayNameList.size() > 1) {
		nLastMp4FileDuration = nTime_end - nTime_last_file;
	}
	else {
		// ���ļ�����Ϊ1ʱ��������ʱ��Ϊ����ʱ��-��ʼʱ�䣬����pts ���Ӻ�ֻ��ȡ nLastMp4FileDuration ����
		nLastMp4FileDuration = nTime_end - nTime_start;
	}

	WriteLog(Log_Debug, "CalcLastMp4FileDuration() ��������һ��mp4�ļ�ֻ�貥�� %d �뼴�� ,nClient = %llu ", nLastMp4FileDuration, nClient );

	return nLastMp4FileDuration;
}
