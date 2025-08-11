/*
���ܣ�
    ʵ��rtmp�������˵Ľ���ģ�� ���ܽ���rtmp�ͻ�����������������������Ƶ����Ƶ 
	����rtmpЭ��ת����Ƶ����Ƶ�� 
����    2021-04-02
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetRtmpServerRecv.h"

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
#ifdef USE_BOOST
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
#else
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);

#endif


extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
int                                          SampleRateArray[] = { 96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350 };
static int NetRtmpServerRecvCallBackFLV(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags);
static int NetRtmpServerRec_MuxerFlv(void* flv, int type, const void* data, size_t bytes, uint32_t timestamp);
extern MediaServerPort                      ABL_MediaServerPort;
extern bool                                 AddClientToMapAddMutePacketList(uint64_t nClient);
extern bool                                 DelClientToMapFromMutePacketList(uint64_t nClient);
extern CMediaFifo                           pDisconnectMediaSource;      //�������ý��Դ 

static int rtmp_server_send(void* param, const void* header, size_t len, const void* data, size_t bytes)
{
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;

	if (pClient != NULL && pClient->bRunFlag.load())
	{
		if (len > 0 && header != NULL && pClient->bRunFlag.load())
		{
			pClient->nWriteRet = XHNetSDK_Write(pClient->nClient, (uint8_t*)header, len,ABL_MediaServerPort.nSyncWritePacket);
			if (pClient->nWriteRet != 0)
			{
				pClient->nWriteErrorCount ++;
				if (pClient->nWriteErrorCount >= 30)
				{
				   pClient->bRunFlag.exchange(false);
				   WriteLog(Log_Debug, "rtmp_server_send ����ʧ�ܣ����� nWriteErrorCount = %d ", pClient->nWriteErrorCount);
				   pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
  				}
			}
			else
				pClient->nWriteErrorCount = 0;
		}
		if (bytes > 0 && data != NULL && pClient->bRunFlag.load())
		{
			pClient->nWriteRet = XHNetSDK_Write(pClient->nClient, (uint8_t*)data, bytes, ABL_MediaServerPort.nSyncWritePacket);
			if (pClient->nWriteRet != 0)
			{
				pClient->nWriteErrorCount ++;
				WriteLog(Log_Debug,"rtmp_server_send ����ʧ�ܣ����� nWriteErrorCount = %d ", pClient->nWriteErrorCount);
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
			else
				pClient->nWriteErrorCount = 0;
		}
	}
	return len + bytes;
}

static int rtmp_server_onpublish(void* param, const char* app, const char* stream, const char* type)
{//��ӡ rtmp ��·�� ����·�� 
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if (pClient != NULL && pClient->bRunFlag.load())
	{
		WriteLog(Log_Debug,"CNetRtmpServerRecv=%X, nClient = %llu, rtmp_server_onpublish(%s, %s, %s)", pClient, pClient->nClient, app, stream, type);

		//����rtmp��������Ҫ����·�� 
		if ( !(strlen(app) > 0 && strlen(stream) > 0 ))
		{//����rtmp��������Ҫ����·�� 
			WriteLog(Log_Debug, "rtmp������ַ�����������ı�׼����Ҫ��������URL: rtmp://190.15.240.11:1935/live/Camera_00001 ");
			pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			return 0;
		}

		//����app ,stream ��ֹ���� RecordFileReplaySplitter 
		if (strstr(app, RecordFileReplaySplitter) != NULL || strstr(stream, RecordFileReplaySplitter) != NULL)
		{
			WriteLog(Log_Debug, " app ,stream ��ֹ���� __ReplayFMP4RecordFile__ �ַ��� ");
			pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			return 0;
		}

		//ȥ������������ַ���
		string strMP4Name = stream;
		char   szStream[string_length_512] = { 0 };
		strcpy(szStream, stream);
		int    nPos = strMP4Name.find("?", 0);
		if (nPos > 0 && nPos != string::npos && strlen(stream) > 0)
		{//������Ȩ����
			memcpy(pClient->szPlayParams, stream + (nPos + 1), strlen(stream) - nPos - 1);
			szStream[nPos] = 0x00;
		}

 		sprintf(pClient->szURL, "/%s/%s", app, szStream);
	
		auto pTempSource = GetMediaStreamSource(pClient->szURL,true);
		if (pTempSource != NULL)
		{//������ַ�Ѿ����� 
			WriteLog(Log_Debug, "--- ������ַ�Ѿ�����--- %s ",pClient->szURL );
			pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			return 0;
		}

		//���ƴ��������ṹ 
		strcpy(pClient->m_addStreamProxyStruct.app, app);
		strcpy(pClient->m_addStreamProxyStruct.stream, szStream);
		sprintf(pClient->m_addStreamProxyStruct.url, "rtmp://%s:%d/%s/%s", pClient->szClientIP, ABL_MediaServerPort.nRtmpPort, app, szStream);

		pClient->flvDemuxer = flv_demuxer_create(NetRtmpServerRecvCallBackFLV, pClient);
		pClient->netBaseNetType = NetBaseNetType_RtmpServerRecvPush;

 		pClient->bPushMediaSuccessFlag = true; //�ɹ����� 
		pClient->pMediaSource =  CreateMediaStreamSource(pClient->szURL,pClient->nClient, MediaSourceType_LiveMedia, 0, pClient->m_h265ConvertH264Struct);
		if (pClient->pMediaSource == NULL)
		{//������ַ�Ѿ����� 
			WriteLog(Log_Debug, "����ý��Դʧ�� %s ", pClient->szURL);
			pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			return 0;
		}

		pClient->pMediaSource->netBaseNetType = pClient->netBaseNetType;
		//�����������¼������¼���־
		if(pClient->pMediaSource && ABL_MediaServerPort.pushEnable_mp4 == 1)
 		   pClient->pMediaSource->enable_mp4 = true;
 		pClient->netBaseNetType = NetBaseNetType_RtmpServerRecvPush;//RTMP �����������տͻ��˵����� 
	}
	return 0;
}

//rtmp�㲥���ص� 
static int rtmp_server_onplay(void* param, const char* app, const char* stream, double start, double duration, uint8_t reset)
{
	WriteLog(Log_Debug, "rtmp_server_onplay(%s, %s, %f, %f, %d)", app, stream, start, duration, (int)reset);
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
 
    if(!pClient->bRunFlag.load())
		return -1 ;

	//ȥ������������ַ���
	string strMP4Name = stream;
	char   szStream[string_length_1024] = { 0 };
	strcpy(szStream, stream);
	int    nPos = strMP4Name.find("?", 0);
	if (nPos > 0 && nPos != string::npos && strlen(stream) > 0)
	{//������Ȩ����
		memcpy(pClient->szPlayParams, stream + (nPos + 1), strlen(stream) - nPos - 1);
		szStream[nPos] = 0x00;
	}

	char szTemp[string_length_1024] = { 0 };
	sprintf(szTemp, "/%s/%s", app, szStream);
	strcpy(pClient->szMediaSourceURL, szTemp);

#ifdef USE_BOOST

	boost::shared_ptr<CMediaStreamSource> pushClient = NULL;
#else

	std::shared_ptr<CMediaStreamSource> pushClient = NULL;
#endif
	//ȷ����������
	pClient->netBaseNetType = NetBaseNetType_RtmpServerSendPush;

	//�ж�Դ��ý���Ƿ����
	if (strstr(szTemp, RecordFileReplaySplitter) == NULL)
	{//�ۿ�ʵ��
		pushClient = GetMediaStreamSource(szTemp,true);

		if (pushClient == NULL || !(strlen(pushClient->m_mediaCodecInfo.szVideoName) > 0 || strlen(pushClient->m_mediaCodecInfo.szAudioName) > 0))
		{
			WriteLog(Log_Debug, "CNetRtmpServerRecv=%X, û����������ĵ�ַ %s nClient = %llu ", pClient, szTemp, pClient->nClient);
 			if (pClient)
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			return -1;
 		}
	}
	else
	{//¼��㲥
 	    //��ѯ�㲥��¼���Ƿ����
		if (pClient->QueryRecordFileIsExiting(szTemp) == false)
		{
			WriteLog(Log_Debug, "CNetRtmpServerRecv = %X, û�е㲥��¼���ļ� %s nClient = %llu ", pClient, szTemp, pClient->nClient);
			if (pClient)
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			return -1;
		}

		//����¼���ļ��㲥
		pushClient = pClient->CreateReplayClient(szTemp, &pClient->nReplayClient);
		if (pushClient == NULL)
		{
			WriteLog(Log_Debug, "CNetRtmpServerRecv = %X, ����¼���ļ��㲥ʧ�� %s nClient = %llu ", pClient, szTemp, pClient->nClient);
			if (pClient)
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			return -1;
		}
	}

	//����ý��Դ
	sprintf(pClient->szMediaSourceURL, "/%s/%s", app, szStream);
	strcpy(pClient->m_addStreamProxyStruct.app, app);
	strcpy(pClient->m_addStreamProxyStruct.stream, szStream);
	sprintf(pClient->m_addStreamProxyStruct.url, "rtmp://%s:%d/%s/%s", ABL_MediaServerPort.ABL_szLocalIP,ABL_MediaServerPort.nRtmpPort, app, szStream);
 
	//�ѿͻ��˼���ý�忽���߳� 
	if (pClient && pushClient)
	{
		strcpy(pClient->szRtmpName, szTemp);

 		pClient->flvMuxer = flv_muxer_create(NetRtmpServerRec_MuxerFlv, pClient);

		//�ѿͻ��˼��뷢���߳� 
		pClient->netBaseNetType = NetBaseNetType_RtmpServerSendPush;//RTMP ��������ת���ͻ��˵�������������

		pClient->m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
		pClient->m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);
		pushClient->AddClientToMap(pClient->nClient);
	}
 	return 0;
}
static int rtmp_server_onpause(void* param, int pause, uint32_t ms)
{
	WriteLog(Log_Debug, "rtmp_server_onpause(%d, %u)", pause, (unsigned int)ms);
	return 0;
}
static int rtmp_server_onseek(void* param, uint32_t ms)
{
	WriteLog(Log_Debug, "rtmp_server_onseek(%u)", (unsigned int)ms);
	return 0;
}
static int rtmp_server_ongetduration(void* param, const char* app, const char* stream, double* duration)
{
	*duration = 30 * 60;
	return 0;
}

//FLV ��� 
static int NetRtmpServerRec_MuxerFlv(void* flv, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)flv;

	int r;
	if (pClient && pClient->bRunFlag.load())
	{
		if (FLV_TYPE_VIDEO == type)
		{
			r = rtmp_server_send_video(pClient->rtmp, data, bytes, timestamp);
		}
		else if (FLV_TYPE_AUDIO == type)
		{
			r = rtmp_server_send_audio(pClient->rtmp, data, bytes, timestamp);
		}
	}
	return 0;
}

static int rtmp_server_onscript(void* param, const void* script, size_t bytes, uint32_t timestamp)
{//д�� script ���� 
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if (pClient != NULL && pClient->bRunFlag.load())
	  flv_demuxer_input(pClient->flvDemuxer, FLV_TYPE_SCRIPT, script, bytes, timestamp);

#ifdef  WriteFlvFileByDebug
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if (pClient != NULL && pClient->bRunFlag.load())
	   flv_writer_input(pClient->s_flv, FLV_TYPE_SCRIPT, script, bytes, timestamp);
#endif
	return 0;
}

static int rtmp_server_onvideo(void* param, const void* data, size_t bytes, uint32_t timestamp)
{//д����Ƶ 
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if (pClient != NULL && pClient->bRunFlag.load())
	{
		if (flv_demuxer_input(pClient->flvDemuxer, FLV_TYPE_VIDEO, data, bytes, timestamp) < 0)
		{
			if (!pClient->bDeleteRtmpPushH265Flag)
			{
				//pClient->bDeleteRtmpPushH265Flag = true;
				//WriteLog(Log_Debug, "��֧��rtmp���� H265��Ƶ��rtmp����ֻ֧����ƵH264����ƵAAC�������Ҫ����H265��Ƶ������ʹ��rtsp������׼��ɾ��rtmp�������� nClient = %llu ", pClient->nClient);
				//pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
			return 0;
		}
	}
#ifdef  WriteFlvFileByDebug
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if (pClient != NULL && pClient->bRunFlag.load())
      flv_writer_input(pClient->s_flv, FLV_TYPE_VIDEO, data, bytes, timestamp);
#endif
	return 0;
}

static int rtmp_server_onaudio(void* param, const void* data, size_t bytes, uint32_t timestamp)
{//д����Ƶ 
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if (pClient != NULL && pClient->bRunFlag.load())
		flv_demuxer_input(pClient->flvDemuxer, FLV_TYPE_AUDIO, data, bytes, timestamp);

#ifdef  WriteFlvFileByDebug
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if (pClient != NULL && pClient->bRunFlag.load())
		flv_writer_input(pClient->s_flv, FLV_TYPE_AUDIO, data, bytes, timestamp);
#endif
	return 0;
}

inline const char* ftimestamp(uint32_t t, char* buf)
{
	sprintf(buf, "%02u:%02u:%02u.%03u", t / 3600000, (t / 60000) % 60, (t / 1000) % 60, t % 1000);
	return buf;
}

inline size_t get_adts_length(const uint8_t* data, size_t bytes)
{
	assert(bytes >= 6);
	return ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] >> 5) & 0x07);
}

inline char flv_type(int type)
{
	switch (type)
	{
	case FLV_AUDIO_AAC: return 'A';
	case FLV_AUDIO_MP3: return 'M';
	case FLV_AUDIO_ASC: return 'a';
	case FLV_VIDEO_H264: return 'V';
	case FLV_VIDEO_AVCC: return 'v';
	case FLV_VIDEO_H265: return 'H';
	case FLV_VIDEO_HVCC: return 'h';
	default: return '*';
	}
	return '*';
}

static int NetRtmpServerRecvCallBackFLV(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
	CNetRtmpServerRecv* pClient = (CNetRtmpServerRecv*)param;
	if(!pClient->bRunFlag.load())
		return -1;

	static char s_pts[64], s_dts[64];
	static uint32_t v_pts = 0, v_dts = 0;
	static uint32_t a_pts = 0, a_dts = 0;

	//printf("[%c] pts: %s, dts: %s, %u, cts: %d, ", flv_type(codec), ftimestamp(pts, s_pts), ftimestamp(dts, s_dts), dts, (int)(pts - dts));
	
	if (FLV_AUDIO_AAC == codec)
	{
		a_pts = pts;
		a_dts = dts;

		if (strlen(pClient->pMediaSource->m_mediaCodecInfo.szAudioName) == 0 && bytes > 4 && data != NULL )
		{
			unsigned char* pAudioData = (unsigned char*)data;
 			strcpy(pClient->pMediaSource->m_mediaCodecInfo.szAudioName, "AAC");

			//����Ƶ�����ֻռ4λ��  8 7 6 5 4 3 2 1  �� 6 ~ 3 λ����4��λ������Ҫ��0x3c �����㣬�ѱ��λȫ����Ϊ0 ���������ƶ�2λ��
			unsigned char nSampleIndex = ((pAudioData[2] & 0x3c) >> 2) & 0x0F;  //�� pb[2] �л�ȡ����Ƶ�ʵ����
			if (nSampleIndex <= 12)
				pClient->pMediaSource->m_mediaCodecInfo.nSampleRate = SampleRateArray[nSampleIndex];

			//ͨ���������� pAVData[2]  ����2��λ�������2λ���� 0x03 �����㣬�õ���λ�����ƶ�2λ ���� �� �� pAVData[3] ��������2λ
			//pAVData[3] ������2λ��ȡ���� �� �� 0xc0 �����㣬������6λ��ΪʲôҪ����6λ����Ϊ��2λ�������λ������Ҫ���ұ��ƶ�6λ
			pClient->pMediaSource->m_mediaCodecInfo.nChannels = ((pAudioData[2] & 0x03) << 2) | ((pAudioData[3] & 0xc0) >> 6);
 		}

		if (pClient->pMediaSource)
			pClient->pMediaSource->PushAudio((unsigned char*)data, bytes, pClient->pMediaSource->m_mediaCodecInfo.szAudioName, pClient->pMediaSource->m_mediaCodecInfo.nChannels, pClient->pMediaSource->m_mediaCodecInfo.nSampleRate);
		
		//assert(bytes == get_adts_length((const uint8_t*)data, bytes));
#ifdef  WriteFlvToEsFileFlag
		if (pClient != NULL)
		{
 			fwrite(data, 1, bytes, pClient->fWriteAudio);
			fflush(pClient->fWriteAudio);
		}
#endif
	}
	else if (codec == FLV_AUDIO_MP3)
	{
#ifdef  WritMp3FileFlag 
		if (pClient->fWriteMp3File)
		{
			fwrite(data,1,bytes,pClient->fWriteMp3File);
			fflush(pClient->fWriteMp3File);
		}
#endif
		pClient->pMediaSource->PushAudio((unsigned char*)data, bytes, "MP3", 2, 44100);
	}
	else if (codec == FLV_AUDIO_G711A || codec == FLV_AUDIO_G711U )
	{
     	if (codec == FLV_AUDIO_G711A)
		  pClient->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_A", 1, 8000);
	   else if(codec == FLV_AUDIO_G711U)
		  pClient->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_U", 1, 8000);

#ifdef  WriteG711toPCMFileFlag 
		alaw_to_pcm16(bytes, (const char*)data, pClient->g711toPCM);
		if (pClient->fWriteG711)
		{
			fwrite(pClient->g711toPCM,1,640,pClient->fWriteG711);
			fflush(pClient->fWriteG711);
		}
#endif
	}
	else if (FLV_VIDEO_H264 == codec || FLV_VIDEO_H265 == codec)
	{
		//printf("diff: %03d/%03d %s", (int)(pts - v_pts), (int)(dts - v_dts), flags ? "[I]" : "");
		v_pts = pts;
		v_dts = dts;

		if (!pClient->bUpdateVideoFrameSpeedFlag)
		{//������ƵԴ��֡�ٶ�
			int nVideoSpeed = pClient->CalcFlvVideoFrameSpeed(pts,1000);
			if (nVideoSpeed > 0 && pClient->pMediaSource != NULL)
			{
				pClient->bUpdateVideoFrameSpeedFlag = true;
				WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pClient->nClient, pClient->pMediaSource->m_szURL, pClient->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pClient->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pClient->netBaseNetType);
			}
		}

		if (pClient->pMediaSource)
		{
			if(FLV_VIDEO_H264 == codec)
			   pClient->pMediaSource->PushVideo((unsigned char*)data, bytes,"H264");
			else 
			   pClient->pMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
		}

		//unsigned char* pVideoData = (unsigned char*)data;
	    //WriteLog(Log_Debug, "CNetRtspServer=%X ,nClient = %llu, rtmp ����ص� %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X , timeStamp = %d ,datasize = %d ", pClient, pClient->nClient, (unsigned char*)pVideoData[0], pVideoData[1], pVideoData[2], pVideoData[3], pVideoData[4], pVideoData[5], pVideoData[6], pVideoData[7], pVideoData[8], pVideoData[9], pVideoData[10], pVideoData[11], pVideoData[12],dts,bytes);

#ifdef  WriteFlvToEsFileFlag
		if (pClient != NULL)
		{
		  fwrite(data,1,bytes,pClient->fWriteVideo);
		  fflush(pClient->fWriteVideo);
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

CNetRtmpServerRecv::CNetRtmpServerRecv(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	bCheckRtspVersionFlag = false;
	bDeleteRtmpPushH265Flag = false;
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	strcpy(m_szShareMediaURL,szShareMediaURL);
	NetDataFifo.InitFifo(MaxNetDataCacheBufferLength);

	int r;
	memset(&handler, 0, sizeof(handler));
	handler.send = rtmp_server_send;
	//handler.oncreate_stream = rtmp_server_oncreate_stream;
	//handler.ondelete_stream = rtmp_server_ondelete_stream;
	handler.onplay = rtmp_server_onplay;
	handler.onpause = rtmp_server_onpause;
	handler.onseek = rtmp_server_onseek;
	handler.ongetduration = rtmp_server_ongetduration;

	handler.onpublish = rtmp_server_onpublish;
	handler.onscript = rtmp_server_onscript;
	handler.onvideo = rtmp_server_onvideo;
	handler.onaudio = rtmp_server_onaudio;
	pMediaSource = NULL;
	flvMuxer = NULL;
	flvDemuxer = NULL;
	nWriteRet = 0;
	nWriteErrorCount = 0;

#ifdef  WriteFlvFileByDebug
	char  szFlvFile[256] = { 0 };
	sprintf(szFlvFile,".\\%X_%d.flv", this, rand());
	s_flv = flv_writer_create(szFlvFile);
#endif
#ifdef  WriteFlvToEsFileFlag
	char    szVideoFile[256] = { 0 };
	char    szAudioFile[256] = { 0 };
	sprintf(szVideoFile, ".\\%X_%d.264", this, rand());
	sprintf(szAudioFile, ".\\%X_%d.aac", this, rand());
	fWriteVideo = fopen(szVideoFile, "wb");
	fWriteAudio = fopen(szAudioFile, "wb");
#endif

#ifdef  WriteG711toPCMFileFlag 
	char    szPcmFile[256] = { 0 };
	sprintf(szPcmFile, "D:\\%X_%d.pcm", this, rand());
	fWriteG711 = fopen(szPcmFile, "wb");
#endif
#ifdef  WritMp3FileFlag 
	char    szMp3File[256] = { 0 };
	sprintf(szMp3File, "E:\\rtmp_recv_%X_%d.mp3", this, rand());
	fWriteMp3File = fopen(szMp3File, "wb"); ;
#endif
	rtmp = rtmp_server_create(this, &handler);
	memset(szRtmpName, 0x00, sizeof(szRtmpName));
	bRunFlag.exchange(true);

	WriteLog(Log_Debug, "CNetRtmpServerRecv ���� =%X  nClient = %llu ",this, nClient);
}

CNetRtmpServerRecv::~CNetRtmpServerRecv()
{
	std::lock_guard<std::mutex> lock(NetRtmpServerLock);
	
	WriteLog(Log_Debug, "CNetRtmpServerRecv =%X  Step 1 nClient = %llu ",this, nClient);

	WriteLog(Log_Debug, "CNetRtmpServerRecv =%X  Step 3 nClient = %llu ",this, nClient);

	//��ý�忽���̳߳��Ƴ�
	WriteLog(Log_Debug, "CNetRtmpServerRecv =%X  Step 4 nClient = %llu ",this, nClient);

	rtmp_server_destroy(rtmp);
	WriteLog(Log_Debug, "CNetRtmpServerRecv =%X  Step 5 nClient = %llu ",this, nClient);

	if(flvDemuxer)
	  flv_demuxer_destroy(flvDemuxer);

	if (flvMuxer)
		flv_muxer_destroy(flvMuxer);
	WriteLog(Log_Debug, "CNetRtmpServerRecv =%X  Step 6 nClient = %llu ",this, nClient);

#ifdef  WriteFlvFileByDebug
	flv_writer_destroy(s_flv);
#endif
#ifdef  WriteFlvToEsFileFlag
 	fclose(fWriteVideo);
	fclose(fWriteAudio);
#endif

#ifdef  WriteG711toPCMFileFlag 
	if(fWriteG711)
 	 fclose(fWriteG711);
#endif
#ifdef  WritMp3FileFlag 
	if (fWriteMp3File)
		fclose(fWriteMp3File);
#endif
	NetDataFifo.FreeFifo();
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();

	//�Ӿ�������ɾ�� 
	if (bAddMuteFlag)
		DelClientToMapFromMutePacketList(nClient);

	//����ǽ������������ҳɹ����������ģ�����Ҫɾ��ý������Դ szURL ������ /Media/Camera_00001 
	if (bPushMediaSuccessFlag && netBaseNetType == NetBaseNetType_RtmpServerRecvPush && pMediaSource !=	NULL )
 	   pDisconnectMediaSource.push((unsigned char*)szURL, strlen(szURL));

	WriteLog(Log_Debug, "CNetRtmpServerRecv ���� = %X  nClient = %llu \r\n", this, nClient);
	malloc_trim(0);
}

int CNetRtmpServerRecv::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0;

	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	m_videoFifo.push(pVideoData, nDataLength);
	return 0;
}

int CNetRtmpServerRecv::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;

	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}
	m_audioFifo.push(pVideoData, nDataLength);
	return 0;
}
int CNetRtmpServerRecv::SendVideo()
{
	std::lock_guard<std::mutex> lock(NetRtmpServerLock);
	
 	if (nWriteErrorCount >= 30)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
	}

	//ֻ����Ƶ������������Ƶ
	if (ABL_MediaServerPort.nEnableAudio == 0 )
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (flvMuxer)
		{
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			{
			   if(nMediaSourceType == MediaSourceType_LiveMedia )
			     flv_muxer_avc(flvMuxer, pData , nLength , videoDts, videoDts);
			  else 
				 flv_muxer_avc(flvMuxer, pData + 4 , nLength - 4, videoDts, videoDts);
			}
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			{
				if (nMediaSourceType == MediaSourceType_LiveMedia)
					flv_muxer_hevc(flvMuxer, pData, nLength, videoDts, videoDts);
				else 
					flv_muxer_hevc(flvMuxer, pData + 4 , nLength - 4, videoDts, videoDts);
			}
		}

		videoDts += nVideoStampAdd ;

		m_videoFifo.pop_front();
	}

	if (nWriteErrorCount >= 30)
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));

	return 0;
}

int CNetRtmpServerRecv::SendAudio()
{
	std::lock_guard<std::mutex> lock(NetRtmpServerLock);
 
	if (nWriteErrorCount >= 30)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (flvMuxer)
		{
			if (nMediaSourceType == MediaSourceType_LiveMedia)
			{
			    if(strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
				  flv_muxer_aac(flvMuxer, pData, nLength, audioDts, audioDts);
				else if(strcmp(mediaCodecInfo.szAudioName, "MP3") == 0)
				  flv_muxer_mp3(flvMuxer, pData, nLength, audioDts, audioDts);
				else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
				{
				  flv_muxer_g711a(flvMuxer, pData, nLength, audioDts, audioDts);
				  audioDts += nLength / 8;
				}
				else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
				{
				  flv_muxer_g711u(flvMuxer, pData, nLength, audioDts, audioDts);
				  audioDts += nLength / 8;
				}
			}
			else
				flv_muxer_aac(flvMuxer, pData+4, nLength-4, audioDts, audioDts);
		}

		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0 || strcmp(mediaCodecInfo.szAudioName, "MP3") == 0)
		{
			if (bUserNewAudioTimeStamp == false)
				audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp;
			else
			{
				nUseNewAddAudioTimeStamp --;
				audioDts += nNewAddAudioTimeStamp;
				if (nUseNewAddAudioTimeStamp <= 0)
				{
					bUserNewAudioTimeStamp = false;
				}
			}
		}
 
		m_audioFifo.pop_front();
	    SyncVideoAudioTimestamp(); 
	}


	if (nWriteErrorCount >= 30)
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));

	return 0;
}

int CNetRtmpServerRecv::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	nRecvDataTimerBySecond = 0;
	NetDataFifo.push(pData, nDataLength);
	return 0;
}

int CNetRtmpServerRecv::ProcessNetData()
{
 	unsigned char* pData = NULL;
	int            nLength;

	pData = NetDataFifo.pop(&nLength);
	if(pData != NULL)
	{
#if   1  //�������� || (pData[0] == 0x03) ������obs����������
		if (!bCheckRtspVersionFlag)
		{
			bCheckRtspVersionFlag = true;
			if ( !((pData[0] == 0x03 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x00 && pData[4] == 0x00) || (pData[0] == 0x03)))
			{//�򵥼���ͷ�Ƿ�Ϸ����Ƿ����rtmpЭ��
			    WriteLog(Log_Debug, "CNetRtmpServerRecv = %X �Ƿ��� rtmp��ͷ ",this);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
				return -1;
			}
		}
#endif 
		if (nLength > 0)
		  rtmp_server_input(rtmp, pData, nLength);

 		NetDataFifo.pop_front();
	}
	return 0;
}

//���͵�һ������
int CNetRtmpServerRecv::SendFirstRequst()
{
	return 0;
}

//����m3u8�ļ�
bool  CNetRtmpServerRecv::RequestM3u8File()
{
	return true;
}