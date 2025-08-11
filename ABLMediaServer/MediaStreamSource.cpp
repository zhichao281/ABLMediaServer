/*
���ܣ�
    ʵ��ÿ�����緢�Ͷ���CNetRlvSend CNetRtspSend �ȵȵ�ý�����ݿ������������� ��ֻ�ܿ���
����    2021-03-29
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/
#include "stdafx.h"
#include "NetRecvBase.h"
#include "MediaStreamSource.h"
#ifdef USE_BOOST
extern boost::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase> GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern bool                            DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                      pDisconnectBaseNetFifo; //������ѵ����� 
extern MediaServerPort                 ABL_MediaServerPort;
extern char                            ABL_wwwMediaPath[256] ; //www ��·��
extern char                            ABL_MediaSeverRunPath[256]; //��ǰ·��
extern int64_t                         nTestRtmpPushID;
extern boost::shared_ptr<CNetRevcBase> CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern boost::shared_ptr<CRecordFileSource>  CreateRecordFileSource(char* app, char* stream);
extern boost::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
extern uint64_t                        GetCurrentSecond();
extern boost::shared_ptr<CPictureFileSource> CreatePictureFileSource(char* app, char* stream);
#else
extern std::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase> GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern bool                            DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                      pDisconnectBaseNetFifo; //������ѵ����� 
extern MediaServerPort                 ABL_MediaServerPort;
extern char                            ABL_wwwMediaPath[256] ; //www ��·��
extern char                            ABL_MediaSeverRunPath[256]; //��ǰ·��
extern int64_t                         nTestRtmpPushID;
extern std::shared_ptr<CNetRevcBase> CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern std::shared_ptr<CRecordFileSource>  CreateRecordFileSource(char* app, char* stream);
extern std::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
extern uint64_t                        GetCurrentSecond();
extern std::shared_ptr<CPictureFileSource> CreatePictureFileSource(char* app, char* stream);
#endif


int                                    CMediaStreamSource::nConvertObjectCount = 0;
int32_t                                CMediaStreamSource::mediaSourceCount = 0;
extern bool 	                       ABL_bCudaFlag ;
extern int                             ABL_nCudaCount ;
extern CMediaFifo                      pMessageNoticeFifo;          //��Ϣ֪ͨFIFO
extern char                            ABL_szLocalIP[128];
uint8_t NALU_START_CODE[]              = { 0x00, 0x00, 0x01 };
uint8_t SLICE_START_CODE[]             = { 0X00, 0X00, 0X00, 0X01 };

#ifdef OS_System_Windows
extern ABL_cudaDecode_Init cudaCodec_Init;
extern ABL_cudaDecode_GetDeviceGetCount  cudaCodec_GetDeviceGetCount;
extern ABL_cudaDecode_GetDeviceName cudaCodec_GetDeviceName;
extern ABL_cudaDecode_GetDeviceUse cudaCodec_GetDeviceUse;
extern ABL_CreateVideoDecode cudaCodec_CreateVideoDecode;
extern ABL_CudaVideoDecode cudaCodec_CudaVideoDecode;
extern ABL_DeleteVideoDecode cudaCodec_DeleteVideoDecode;
extern ABL_GetCudaDecodeCount cudaCodec_GetCudaDecodeCount;
extern ABL_VideoDecodeUnInit cudaCodec_UnInit;
#else


extern ABL_cudaDecode_Init cudaCodec_Init ;
extern ABL_cudaDecode_GetDeviceGetCount  cudaCodec_GetDeviceGetCount ;
extern ABL_cudaDecode_GetDeviceName cudaCodec_GetDeviceName ;
extern ABL_cudaDecode_GetDeviceUse cudaCodec_GetDeviceUse ;
extern ABL_CreateVideoDecode cudaCodec_CreateVideoDecode ;
extern ABL_CudaVideoDecode cudaCodec_CudaVideoDecode ;
extern ABL_DeleteVideoDecode cudaCodec_DeleteVideoDecode ;
extern ABL_GetCudaDecodeCount cudaCodec_GetCudaDecodeCount;
extern ABL_VideoDecodeUnInit cudaCodec_UnInit ;


extern ABL_cudaEncode_Init cudaEncode_Init ;
extern ABL_cudaEncode_GetDeviceGetCount cudaEncode_GetDeviceGetCount ;
extern ABL_cudaEncode_GetDeviceName cudaEncode_GetDeviceName ;
extern ABL_cudaEncode_CreateVideoEncode cudaEncode_CreateVideoEncode ;
extern ABL_cudaEncode_DeleteVideoEncode cudaEncode_DeleteVideoEncode ;
extern ABL_cudaEncode_CudaVideoEncode cudaEncode_CudaVideoEncode ;
extern ABL_cudaEncode_UnInit cudaEncode_UnInit ;
 
//����·��Ȩ�� 
extern void  ABL_SetPathAuthority(char* szPath);
#endif

CMediaStreamSource::CMediaStreamSource(char* szURL, uint64_t nClientTemp, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct)
{
	initiative = false;	
	mediaSourceCount ++;
	nG711ConvertAAC = ABL_MediaServerPort.nG711ConvertAAC;
	videoFileFormat = ABL_MediaServerPort.videoFileFormat;
	fileKeepMaxTime = ABL_MediaServerPort.fileKeepMaxTime;
	bEnableFlag = true ;
	memset(sim, 0x00, sizeof(sim));
	memset(szM3u8Name, 0x00, sizeof(szM3u8Name));
	memset(szHLSPath, 0x00, sizeof(szHLSPath));
	memset(szSnapPicturePath, 0x00, sizeof(szSnapPicturePath));
	iFrameArriveNoticCount = 0;
	m_bNoticeOnPublish = false;
	m_mediaCodecInfo.nVideoFrameRate = 25;
	m_bPauseFlag = false;
#ifdef WriteCudaDecodeYUVFlag
	nWriteYUVCount = 0;
	char szCudaYUVName[256] = { 0 };
	sprintf(szCudaYUVName, "%s%X_%d.YUV", ABL_MediaSeverRunPath, this, rand());
	fCudaWriteYUVFile = fopen(szCudaYUVName, "wb");
#endif
#ifdef  WriteInputVdideoFlag
	char szFileName[256] = { 0 };
	sprintf(szFileName, "%s_%X.264", ABL_MediaSeverRunPath, this);
	fWriteInputVideo = fopen(szFileName,"wb") ;
#endif
#ifdef WriteInputVideoFileFlag
	char szFileName[256] = { 0 };
	sprintf(szFileName, "%s_%X.264", ABL_MediaSeverRunPath, this);
	fWriteInputVideoFile = fopen(szFileName, "wb");
#endif
	memset(pSPSPPSBuffer, 0x00, sizeof(pSPSPPSBuffer));
	nSPSPPSLength = 0;

	nSrcWidth = nSrcHeight = 0;
	nEncodeBufferLengthCount = nCudaDecodeFrameCount  = 0;
	memcpy((char*)&m_h265ConvertH264Struct, (char*)&h265ConvertH264Struct, sizeof(H265ConvertH264Struct));

	//����ⲿָ��ת��Ŀ������·��Ƶִ��ת��
	if ((m_h265ConvertH264Struct.convertOutWidth > 0 && m_h265ConvertH264Struct.convertOutHeight > 0)|| m_h265ConvertH264Struct.convertOutWidth == -1 && m_h265ConvertH264Struct.convertOutHeight == -1)
	{
		m_h265ConvertH264Struct.H265ConvertH264_enable = 1;
	}
	else
	{//ʹ��Ĭ�ϵĿ���
		if (ABL_MediaServerPort.H265ConvertH264_enable == 1)
		{
			m_h265ConvertH264Struct.convertOutWidth = ABL_MediaServerPort.convertOutWidth;
			m_h265ConvertH264Struct.convertOutHeight = ABL_MediaServerPort.convertOutHeight;
		}
	}

	//��������ļ�ָ��ת�룬��������Ƶ������ת��
	if (ABL_MediaServerPort.H265ConvertH264_enable == 1)
		m_h265ConvertH264Struct.H265ConvertH264_enable = 1;

	if (m_h265ConvertH264Struct.H265ConvertH264_enable == 1 && m_h265ConvertH264Struct.convertOutWidth > 0 && m_h265ConvertH264Struct.convertOutHeight > 0)
	{//ȷ��ת�������������С
		if (m_h265ConvertH264Struct.convertOutWidth <= 352)
			m_h265ConvertH264Struct.convertOutBitrate = 768;
		else if (m_h265ConvertH264Struct.convertOutWidth > 352 && m_h265ConvertH264Struct.convertOutWidth <= 640)
			m_h265ConvertH264Struct.convertOutBitrate = 1024;
		else if (m_h265ConvertH264Struct.convertOutWidth > 640 && m_h265ConvertH264Struct.convertOutWidth <= 960)
			m_h265ConvertH264Struct.convertOutBitrate = 1680;
		else if (m_h265ConvertH264Struct.convertOutWidth > 960 && m_h265ConvertH264Struct.convertOutWidth <= 1280)
			m_h265ConvertH264Struct.convertOutBitrate = 2048;
		else
			m_h265ConvertH264Struct.convertOutBitrate = 3048;
	}
	else
		m_h265ConvertH264Struct.convertOutBitrate = ABL_MediaServerPort.convertOutBitrate;

	//�������ļ���H264��Ƶ���룬ת�븲�Ǳ�·��Ƶ��ת�����
	if(m_h265ConvertH264Struct.H264DecodeEncode_enable == 0)
	  m_h265ConvertH264Struct.H264DecodeEncode_enable = ABL_MediaServerPort.H264DecodeEncode_enable;
 		
	pFFVideoFilter = NULL ;
	memset(szCreateTSDateTime, 0x00, sizeof(szCreateTSDateTime));
	GetCreateTSDateTime();
	memset((char*)&hevc, 0x00, sizeof(hevc));
	enable_hls = ABL_MediaServerPort.nHlsEnable;

	bNoticeClientArriveFlag = false;
	nVideoBitrate = 0 ;//��Ƶ����
	 nAudioBitrate = 0 ;//��Ƶ����
    nEncodeCudaChan  = 0;
 	nCudaDecodeChan = 0;
	memset(m_szURL, 0x00, sizeof(m_szURL));
	strcpy(m_szURL, szURL);
	nClient = nClientTemp;//��¼ý��Դ����ID
	s_dts = -1;
	nTsFileCount = 0;
	nTsFileOrder = 0;
	audioDts = 0;
	videoDts = 0;
	nVideoOrder = 0;
	nAudioOrder = 0;
	nTsCutFileCount = 0;
	bUpdateVideoSpeed = false;
	nMediaSourceType = nSourceType; //ý��Դ���ͣ�ʵ�����ţ�¼��㲥
	nMediaDuration = nDuration;     //ý��Դʱ������λ�룬��¼��㲥ʱ��Ч

	track_aac = -1;
	track_265 = -1;

	szVideoFrameHead[0] = 0x00;
	szVideoFrameHead[1] = 0x00;
	szVideoFrameHead[2] = 0x00;
	szVideoFrameHead[3] = 0x01;

	memset(pSPSPPSBuffer,0x00,sizeof(pSPSPPSBuffer));
	nSPSPPSLength = 0 ;
	pTsFileCacheBuffer = NULL;

	nVideoStampAdd = 0 ;
	nAsyncAudioStamp = -1;

	enable_mp4 = false ;//�Ƿ�¼��mp4�ļ�
    recordMP4 = 0 ;

	bInitHlsResoureFlag = false;
	InitHlsResoure();

	memset(szDataM3U8, 0x00, sizeof(szDataM3U8));

	for (int i = 0; i < MaxStoreTsFileCount; i++)
		nTsFileSizeArray[i] = 0;

	tsCreateTime = GetTickCount();
	tsPacketHandle = NULL;
	fTSFileWrite = NULL;
	fTSFileWriteByteCount = 0;
	nMaxTsFileCacheBufferSize = Default_TS_MediaFileByteCount ; //��ǰpTsFileCacheBuffer �ֽڴ�С 

	hlsFMP4 = NULL;
	extra_data_sizeH265 = 0;
	pH265Buffer = NULL;
	nFmp4SPSPPSLength = 0; 
	nExtenAudioDataLength = 0;
	hls_init_segmentFlag = false;

	//��ȡapp ,stream ;
	memset(app, 0x00, sizeof(app));
	memset(stream, 0x00, sizeof(stream));
	string strURL2 = szURL;
	int    nPos2 = 0;
	nPos2 = strURL2.rfind("/", strlen(szURL));
	if (nPos2 > 0)
	{
		memcpy(app, szURL + 1, nPos2 - 1);
		memcpy(stream, szURL + nPos2 + 1, strlen(szURL) - nPos2 - 1);
	}

	//���ۿ�ʱ��
	nLastWatchTime = nRecordLastWatchTime = nLastWatchTimeDisconect = GetCurrentSecond();

#if  0
	AddClientToMap(nTestRtmpPushID);
#endif

	nG711ToPCMCacheLength = 0;
	nAACEncodeLength = 2048;
	if (nG711ConvertAAC == 1)
	{
		aacEnc.InitAACEncodec(64000, 8000, 1, &nAACEncodeLength);
	}
	nG711CacheLength = 0;

	netBaseNetType = 0;

	tCopyVideoTime = tsCreateTime = nCalcBitrateTimestamp = nCreateDateTime = GetTickCount64();
	pOutEncodeBuffer = NULL;
	H265ConvertH264_enable = false ;
	
	//add by zxt
	nIDRFrameLengh = 0;//���µ�һ��I֡���� 
	if (ABL_MediaServerPort.ForceSendingIFrame == 1)
	{//���������һ��Gop������Ƶ֡
		pVideoGopFrameBuffer.InitFifo(IDRFrameMaxBufferLength); 
		pCopyVideoGopFrameBuffer.InitFifo(IDRFrameMaxBufferLength);
		pCacheAudioFifo.InitFifo(1024 * 256);
		pCopyCacheAudioFifo.InitFifo(1024 * 256);
	}
#ifdef  OS_System_Windows
	if(ABL_MediaServerPort.picturePath[strlen(ABL_MediaServerPort.picturePath) - 1] == '\\')
	  sprintf(szSnapPicturePath, "%s%s\\%s\\", ABL_MediaServerPort.picturePath, app, stream);
	else 
	  sprintf(szSnapPicturePath, "%s\\%s\\%s\\", ABL_MediaServerPort.picturePath, app, stream);
#else
	if (ABL_MediaServerPort.picturePath[strlen(ABL_MediaServerPort.picturePath) - 1] == '/')
	  sprintf(szSnapPicturePath, "%s%s/%s", ABL_MediaServerPort.picturePath, app, stream);
	else 
	  sprintf(szSnapPicturePath, "%s/%s/%s", ABL_MediaServerPort.picturePath, app, stream);
#endif

	WriteLog(Log_Debug, "CMediaStreamSource ���� %X , ��ý��Դ %s ��nClient = %llu \r\n",this, szURL, nClient);
}

void   CMediaStreamSource::GetCreateTSDateTime()
{
#ifdef OS_System_Windows
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(szCreateTSDateTime, "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
	time_t now;
	time(&now);
	struct tm *local;
	local = localtime(&now);
	sprintf(szCreateTSDateTime, "%04d-%02d-%02d %02d:%02d:%02d", szRecordPath, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
#endif
}

void  CMediaStreamSource::InitHlsResoure()
{
	if (enable_hls == 1 && !bInitHlsResoureFlag)
	{
		bInitHlsResoureFlag = true;
		//��ȡTS�ļ��Ķ�������·��
		string strURL = m_szURL;
		int    nPos = 0;
		nPos = strURL.rfind("/", strlen(m_szURL));
		if (nPos > 0)
		{
			memset(szTSFileSubPath, 0x00, sizeof(szTSFileSubPath));
			memcpy(szTSFileSubPath, m_szURL + nPos + 1, strlen(m_szURL) - nPos - 1);
		}

		//����HLSĿ¼
		memset(szM3u8Name, 0x00, sizeof(szM3u8Name));
		if (ABL_MediaServerPort.nHLSCutType == 1)
		{
			CreateSubPathByURL(m_szURL);
			sprintf(szM3u8Name, "%shls.m3u8", szHLSPath);
		}

		if (ABL_MediaServerPort.nHLSCutType == 2)
		{//��Ƭ���ڴ�
			pTsFileCacheBuffer = new unsigned char[Default_TS_MediaFileByteCount];
			for (int i = 0; i < MaxStoreTsFileCount; i++)
				mediaFileBuffer[i].InitFifo(Default_TS_MediaFileByteCount);
		}
	}
}

//ɾ������������
void  CMediaStreamSource::addClientToDisconnectFifo()
{
	std::lock_guard<std::mutex> lock(mediaSendMapLock);
	uint64_t nSendClient = 0;
	for (auto it = mediaSendMap.begin(); it != mediaSendMap.end();)
	{
		nSendClient = (*it).first;
		mediaSendMap.erase(it++);
		pDisconnectBaseNetFifo.push((unsigned char*)&nSendClient, sizeof(nSendClient));
	}
	mediaSendMap.clear();
}

CMediaStreamSource::~CMediaStreamSource()
{
	WriteLog(Log_Debug, "ɾ��ý��Դ��ַ %s , nClient = %llu ", m_szURL, nClient);
	std::lock_guard<std::mutex> lock(mediaSendMapLock);
 
	if(H265ConvertH264_enable)
	   nConvertObjectCount --;

	videoDecode.stopDecode();

	if (true)
	{
		if (true)
		{//��Ƭ��Ӳ��
			if (fTSFileWrite != NULL)
			{
				fclose(fTSFileWrite);
				fTSFileWrite = NULL;
			}

 			//���ɾ���ļ���
			char           szDelName[256] = { 0 };
			if (tsFileNameFifo.GetSize() > 0)
			{
				unsigned char* pData = NULL;
				int            nLength = 0;
				while (tsFileNameFifo.GetSize() > 0 && (pData = tsFileNameFifo.pop(&nLength)) != NULL)
				{
					memset(szDelName, 0x00, sizeof(szDelName));
					memcpy(szDelName, pData, nLength);

					ABLDeleteFile(szDelName);
					WriteLog(Log_Debug, "ý��Դ����ʱɾ��TS��Ƭ�ļ� ��m_szURL = %s ִ��ɾ���ļ���%s  ", m_szURL, szDelName);

					tsFileNameFifo.pop_front();
				}
			}

			//ɾ��M3U8 �ļ� 
			if (strlen(szM3u8Name) > 0)
			{
				ABLDeleteFile(szM3u8Name);
				WriteLog(Log_Debug, "ý��Դ����ʱɾ��m3u8�ļ� ��m_szURL = %s ִ��ɾ��M3U8�ļ���%s  ", m_szURL, szM3u8Name);
			}

			//��ѭ��ɾ���쳣����ļ�
#if OS_System_Windows
			if (strlen(szHLSPath) > 0 && ABL_MediaServerPort.nHLSCutType == 1)
			{
				memset(szDelName, 0x00, sizeof(szDelName));
				strcpy(szDelName, szHLSPath);
				strcat(szDelName, "*.*");
				ABLDeletePath(szDelName, szHLSPath);

				//ɾ��HLS ��Ƭ·�� 
				RemoveDirectory(szHLSPath);
			}
#else 
			if (strlen(szHLSPath) > 0 && ABL_MediaServerPort.nHLSCutType == 1)
			{
			  ABLDeletePath(szHLSPath, szHLSPath);
		      rmdir(szHLSPath);
 			}
#endif
		}

		//ɾ��TS��Ƭ���
		if (tsPacketHandle != NULL)
		{
			mpeg_ts_destroy(tsPacketHandle);
			tsPacketHandle = NULL;
		}

		//ɾ��fmp4��Ƭ���
		if (hlsFMP4 != NULL)
		{
			hls_fmp4_destroy(hlsFMP4);
			hlsFMP4 = NULL;
		}

		if (true)
		{//��Ƭ���ڴ�
			SAFE_ARRAY_DELETE(pTsFileCacheBuffer);
 
			for (int i = 0; i < MaxStoreTsFileCount; i++)
				mediaFileBuffer[i].FreeFifo();

			SAFE_ARRAY_DELETE(pH265Buffer);
 		}
 
		tsFileNameFifo.FreeFifo();
		m3u8FileFifo.FreeFifo();
  	}

	//�ر�¼��洢
	if (enable_mp4 && recordMP4 > 0)
		pDisconnectBaseNetFifo.push((unsigned char*)&recordMP4, sizeof(recordMP4));

	//��ý��Դ �ṩ�� ����ɾ������ 
	auto pClient = GetNetRevcBaseClientNoLock(nClient);
	if (pClient)
	{
		if (pClient->netBaseNetType == NetBaseNetType_addStreamProxyControl || pClient->netBaseNetType == NetBaseNetType_addPushProxyControl)
 		{//������������������ ,��Ҫ��������������� �����ļ��Ĵ˴Σ��ŶϿ������������� 
			if(pClient->nReConnectingCount > ABL_MediaServerPort.nReConnectingCount)
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
 		}else
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
	}

	SAFE_ARRAY_DELETE(pOutEncodeBuffer);
	pVideoGopFrameBuffer.FreeFifo();
	pCacheAudioFifo.FreeFifo();
	pCopyVideoGopFrameBuffer.FreeFifo();
	pCopyCacheAudioFifo.FreeFifo();

#ifdef OS_System_Windows
	if (nCudaDecodeChan > 0)
	{
		cudaCodec_DeleteVideoDecode(nCudaDecodeChan);
		nCudaDecodeChan = 0;
	}
#else
	if (nCudaDecodeChan > 0)
	{
		cudaCodec_DeleteVideoDecode(nCudaDecodeChan);
		nCudaDecodeChan = 0;
	}
	if(nEncodeCudaChan > 0)
	{
		cudaEncode_DeleteVideoEncode(nEncodeCudaChan);
		nEncodeCudaChan = 0;
	}	
#endif 
	avFrameSWS.DeleteAVFrameSws();
    SAFE_DELETE(pFFVideoFilter);
	mediaSourceCount -- ;
	if (mediaSourceCount < 0)
		mediaSourceCount = 0;

	malloc_trim(0);

	//ɾ��ץ��ͼƬ 
#if OS_System_Windows
	if (ABL_MediaServerPort.deleteSnapPicture == 1 && strlen(szSnapPicturePath) > 0)
	{
		char szDeleteFileTemp[string_length_512] = { 0 };
		strcpy(szDeleteFileTemp, szSnapPicturePath);
 		strcat(szDeleteFileTemp, "*.*");
		ABLDeletePath(szDeleteFileTemp,szSnapPicturePath);

		//ɾ��ץ��·�� 
		RemoveDirectory(szSnapPicturePath);
	}
#else 
	if (ABL_MediaServerPort.deleteSnapPicture == 1 && strlen(szSnapPicturePath) > 0)
	{
		ABLDeletePath(szSnapPicturePath, szSnapPicturePath);
		rmdir(szSnapPicturePath);
	}
#endif

#ifdef WriteCudaDecodeYUVFlag
	fclose(fCudaWriteYUVFile);
	fCudaWriteYUVFile = NULL;
#endif
#ifdef  WriteInputVdideoFlag
	if(fWriteInputVideo)
 	  fclose(fWriteInputVideo);
#endif
#ifdef WriteInputVideoFileFlag
	if(fWriteInputVideoFile)
      fclose(fWriteInputVideoFile);
#endif
	WriteLog(Log_Debug, "CMediaStreamSource ���� %X ��� nClient = %llu \r\n", this , nClient);
}

static void* ts_alloc(void* param, size_t bytes)
{
	CMediaStreamSource* pThis = (CMediaStreamSource*)param;
 	assert(bytes <= sizeof(pThis->s_bufferH264TS));
	return pThis->s_bufferH264TS;
}

static void ts_free(void* param, void* /*packet*/)
{
	return;
}

static int ts_write(void* param, const void* packet, size_t bytes)
{
	if (param != NULL)
	{
		CMediaStreamSource* handle = (CMediaStreamSource*)param;
		if (handle != NULL)
		{
			if (ABL_MediaServerPort.nHLSCutType == 1)
			{//��Ƭ��Ӳ��
				if (handle->fTSFileWrite != NULL)
				{
					handle->fTSFileWriteByteCount += bytes;
					return 1 == fwrite(packet, bytes, 1, (FILE*)handle->fTSFileWrite) ? 0 : ferror((FILE*)handle->fTSFileWrite);
				}
				else
					return 0;
			}
			else if (ABL_MediaServerPort.nHLSCutType == 2)
			{//��Ƭ���ڴ� Default_TS_MediaFileByteCount
				if (handle->nMaxTsFileCacheBufferSize - handle->fTSFileWriteByteCount < bytes)
				{//�ռ䲻������Ҫ����
					std::lock_guard<std::mutex> lock(handle->mediaTsMp4CutLock);//��ס

					unsigned char * pTempData = handle->pTsFileCacheBuffer;
					WriteLog(Log_Debug, "CMediaStreamSource= %X ,ý��Դ = %s ,���洢�ռ�,��Ҫ���� nMaxTsFileCacheBufferSize = %d�������Ѿ��洢 fTSFileWriteByteCount = %d��ʣ�� %d �ֽ� ", handle, handle->m_szURL, handle->nMaxTsFileCacheBufferSize, handle->fTSFileWriteByteCount, handle->nMaxTsFileCacheBufferSize - handle->fTSFileWriteByteCount);

					handle->nMaxTsFileCacheBufferSize += (1024 * 1024 * 2);//����2��
					handle->pTsFileCacheBuffer = new unsigned char[handle->nMaxTsFileCacheBufferSize];
 
					memcpy(handle->pTsFileCacheBuffer , pTempData, handle->fTSFileWriteByteCount);
					delete pTempData;
				}

				if (handle->nMaxTsFileCacheBufferSize - handle->fTSFileWriteByteCount >= bytes)
				{
					memcpy(handle->pTsFileCacheBuffer + handle->fTSFileWriteByteCount, packet, bytes);
					handle->fTSFileWriteByteCount += bytes;
				}
 
				return 0;
			}
	   }
	}
	return 0;
}

int CMediaStreamSource::ts_stream(void* ts, int codecid)
{
	std::map<int, int>::const_iterator it = streamsTS.find(codecid);
	if (streamsTS.end() != it)
		return it->second;

	int i = mpeg_ts_add_stream(ts, codecid, NULL, 0);
	streamsTS[codecid] = i;
	return i;
}

//H264��Ƭ
bool  CMediaStreamSource::H264H265FrameToTSFile(unsigned char* szVideo, int nLength)
{
	if (szVideo == NULL || nLength <= 0)
		return false;

	if (strcmp(m_mediaCodecInfo.szVideoName, "H264") == 0)
		avtype = PSI_STREAM_H264;
	else if (strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0)
		avtype = PSI_STREAM_H265;

	if (CheckVideoIsIFrame(m_mediaCodecInfo.szVideoName,szVideo, nLength) == true)
		flags = 1;
	else
		flags = 0;

	ptsVideo = videoDts * 90;
	if (strcmp(m_mediaCodecInfo.szVideoName, "H264") == 0 || strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0)
		mpeg_ts_write(tsPacketHandle, ts_stream(tsPacketHandle, avtype), flags, ptsVideo, ptsVideo, szVideo, nLength);
	else
	{

	}
	nVideoOrder ++;

	if (nVideoOrder % (ABL_MediaServerPort.hlsCutTime * 25 ) == 0)
	{//1����Ƭ1��
		SaveTsMp4M3u8File();
 	}
	return true;
}

//ͳһ����TS��MP4��M3u8 �ļ� 
void   CMediaStreamSource::SaveTsMp4M3u8File()
{
	//����m3u8�ļ������TS�ļ�
	if (tsPacketHandle != NULL && (strcmp(m_mediaCodecInfo.szVideoName, "H264") == 0 || (strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 1)))
	{
		sprintf(szOutputName, "%d.ts", nTsFileOrder - 1);//��һ���ļ�
		m3u8FileFifo.push((unsigned char*)szOutputName, strlen(szOutputName));
	}
	else if (hlsFMP4 != NULL && strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 2)
	{
		sprintf(szOutputName, "%d.mp4", nTsFileOrder - 1);//��һ���ļ�
		if(nTsFileOrder > 1)//��Ƭmp4�ķ�ʽ������ɾ�� 0.mp4�ļ� 
		   m3u8FileFifo.push((unsigned char*)szOutputName, strlen(szOutputName));
	}
	else
		return;

 	if (m3u8FileFifo.GetSize() > 3)
		m3u8FileFifo.pop_front();//ɾ�������ϵ�һ���ļ�����
	if (m3u8FileFifo.GetSize() >= 3)
	{
		sprintf(szOutputName, "%shls.m3u8", szHLSPath);

		if (1)
		{
			memset(szH264TempBuffer, 0x00, sizeof(szH264TempBuffer));
			memset(szM3u8Buffer, 0x00, sizeof(szM3u8Buffer));

			if (tsPacketHandle != NULL && (strcmp(m_mediaCodecInfo.szVideoName, "H264") == 0 || (strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 1)))
			  sprintf(szH264TempBuffer, "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%d\n#EXT-X-MEDIA-SEQUENCE:%d\n#EXT-X-ALLOW-CACHE:NO\n", ABL_MediaServerPort.hlsCutTime, m3u8FileOrder);
			else if (hlsFMP4 != NULL && strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 2)
			  sprintf(szH264TempBuffer, "#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-TARGETDURATION:%d\n#EXT-X-MEDIA-SEQUENCE:%d\n#EXT-X-ALLOW-CACHE:NO\n#EXT-X-MAP:URI=\"%s/0.mp4\",\n", ABL_MediaServerPort.hlsCutTime, m3u8FileOrder, szTSFileSubPath);

			FILE* m3u8File = NULL;
			if (ABL_MediaServerPort.nHLSCutType == 1)
			{
				m3u8File = fopen(szOutputName, "w+b");
				if (m3u8File)
					fprintf(m3u8File, szH264TempBuffer);
			}
			strcpy(szM3u8Buffer, szH264TempBuffer);

			unsigned char* pData;
			int            nLength;
			for (int i = 0; i < 3; i++)
			{
				pData = m3u8FileFifo.pop(&nLength);
				if (pData)
				{
					memset(szOutputName, 0x00, sizeof(szOutputName));
					memcpy(szOutputName, pData, nLength);
					sprintf(szH264TempBuffer, "#EXTINF:%d.000,\n%s/%s\n", ABL_MediaServerPort.hlsCutTime, szTSFileSubPath, szOutputName);

					if (ABL_MediaServerPort.nHLSCutType == 1)
					{
						if (m3u8File)
							fprintf(m3u8File, szH264TempBuffer);
					}

					strcat(szM3u8Buffer, szH264TempBuffer);

					m3u8FileFifo.pop_front();
					m3u8FileFifo.push((unsigned char*)szOutputName, strlen(szOutputName));//����TS�ļ�����
				}
			}

			if (ABL_MediaServerPort.nHLSCutType == 1)
			{
				if (m3u8File)
					fclose(m3u8File);
			}
		}

		//����m3u8����
		CopyM3u8Buffer(szM3u8Buffer);

		m3u8FileOrder++;
	}

	nTsFileSizeArray[nTsFileOrder % MaxStoreTsFileCount] = fTSFileWriteByteCount;

	if (ABL_MediaServerPort.nHLSCutType == 1)
	{
		if (fTSFileWrite != NULL)
		{
			fclose(fTSFileWrite);
			fTSFileWrite = NULL;

			if (ABL_MediaServerPort.hook_enable == 1)
			{//��Ƭ���һ��TS�ļ�֪ͨ
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = NetBaseNetType_HttpClient_on_record_ts;
				sprintf(msgNotice.szMsg, "{\"eventName\":\"on_record_ts\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu,\"createDateTime\":\"%s\",\"currentFileDuration\":%d,\"fileName\":\"%s\"}", app, stream, ABL_MediaServerPort.mediaServerID, netBaseNetType, nClient, szCreateTSDateTime, ABL_MediaServerPort.hlsCutTime, szHookTSFileName);
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			}
		}
	}
	else if (ABL_MediaServerPort.nHLSCutType == 2)
	{
		int nOrder = nTsFileOrder % MaxStoreTsFileCount;
		if (mediaFileBuffer[nOrder].GetFifoLength() < fTSFileWriteByteCount)
		{
			WriteLog(Log_Debug, "CMediaStreamSource= %X ,ý��Դ = %s ,TS��MP4 ��Ƭ�ļ���ԭ������ %d ����,���ڳ��� %d����Ҫ���·��� ", this, m_szURL, mediaFileBuffer[nOrder].GetFifoLength(), fTSFileWriteByteCount);
			mediaFileBuffer[nOrder].FreeFifo();
			mediaFileBuffer[nOrder].InitFifo(fTSFileWriteByteCount);
		}

		//�洢¼��
		mediaFileBuffer[nOrder].Reset();
		std::lock_guard<std::mutex> lock(mediaTsMp4CutLock);//��ס
		  mediaFileBuffer[nOrder].push(pTsFileCacheBuffer, fTSFileWriteByteCount);
	}

	if ((nTsCutFileCount % (25 * 120)) == 0 )
	{
		if (tsPacketHandle != NULL)
			WriteLog(Log_Debug, "CMediaStreamSource= %X ,ý��Դ = %s ,HLS ��Ƶ���� TS ��Ƭ,�ļ����ڳ��� fTSFileWriteByteCount = %d , nMaxTsFileCacheBufferSize = %d ", this, m_szURL, fTSFileWriteByteCount, nMaxTsFileCacheBufferSize);
		else if (hlsFMP4 != NULL)
			WriteLog(Log_Debug, "CMediaStreamSource= %X ,ý��Դ = %s ,HLS ��Ƶ���� MP4 ��Ƭ,�ļ����ڳ��� fTSFileWriteByteCount = %d ,nMaxTsFileCacheBufferSize = %d ", this, m_szURL, fTSFileWriteByteCount, nMaxTsFileCacheBufferSize);
	}
	fTSFileWriteByteCount = 0;
	nTsFileOrder ++;
	nTsCutFileCount ++;

	if (tsPacketHandle != NULL && (strcmp(m_mediaCodecInfo.szVideoName, "H264") == 0 || (strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 1)))
		sprintf(szOutputName, "%s%d.ts", szHLSPath, nTsFileOrder);
	else if (hlsFMP4 != NULL && strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 2)
		sprintf(szOutputName, "%s%d.mp4", szHLSPath, nTsFileOrder);

	memset(szHookTSFileName, 0x00, sizeof(szHookTSFileName));
	strcpy(szHookTSFileName, szOutputName);
 
	if (ABL_MediaServerPort.nHLSCutType == 1)
	{//��Ƭ��Ӳ��
		GetCreateTSDateTime();
		fTSFileWrite = fopen(szOutputName, "w+b");
		tsFileNameFifo.push((unsigned char*)szOutputName, strlen(szOutputName));

		//����ɾ����ʷTS�ļ�
		if (true)
		{
			unsigned char* pData;
			int           nLength;
			while (tsFileNameFifo.GetSize() > ABL_MediaServerPort.nMaxTsFileCount)
			{
				memset(szOutputName, 0x00, sizeof(szOutputName));
				pData = tsFileNameFifo.pop(&nLength);
				if (pData != NULL)
				{
					memcpy(szOutputName, pData, nLength);
					ABLDeleteFile(szOutputName);

					tsFileNameFifo.pop_front();
				}
			}
 		}
	}
	else if (ABL_MediaServerPort.nHLSCutType == 2)
	{//��Ƭ���ڴ�

	}
}

static int hls_init_segment(hls_fmp4_t* hls, void* param)
{
	CMediaStreamSource* pMediaSource = (CMediaStreamSource*)param;
	if (pMediaSource == NULL)
		return 0;

	int bytes = hls_fmp4_init_segment(hls, pMediaSource->s_packet, 1024*16);

	//WriteLog(Log_Debug, "CMediaStreamSource= %X ,ý��Դ = %s ,mp4��Ƭ�ļ������ڳ��� fMP4FileLength = %d ", pMediaSource, pMediaSource->m_szURL, bytes);

	sprintf(pMediaSource->szOutputName, "%s%d.mp4", pMediaSource->szHLSPath, pMediaSource->nTsFileOrder);
	pMediaSource->fTSFileWriteByteCount = pMediaSource->nFmp4SPSPPSLength = bytes;
	if (bytes > 0 && bytes < 1024 * 128)
		memcpy(pMediaSource->pFmp4SPSPPSBuffer, pMediaSource->s_packet, bytes);

	strcpy(pMediaSource->szHookTSFileName, pMediaSource->szOutputName);
 
	if (ABL_MediaServerPort.nHLSCutType == 1)
	{
		FILE* fp = fopen(pMediaSource->szOutputName, "wb");
		if (fp != NULL)
		{
		  fwrite(pMediaSource->s_packet, 1, bytes, fp);
		  fclose(fp);
		}
	}

	//����hls_init_segment ��ʼ����ɲ���д��Ƶ����Ƶ�Σ��ڻص�������������־
	pMediaSource->hls_init_segmentFlag = true;
 
	return 0;
}

static int hls_segment(void* param, const void* data, size_t bytes, int64_t pts, int64_t dts, int64_t duration)
{	
	CMediaStreamSource* pMediaSource = (CMediaStreamSource*)param;
	if (pMediaSource == NULL)
		return 0;

	//����m3u8�ļ� 
	pMediaSource->SaveTsMp4M3u8File();

	if (ABL_MediaServerPort.nHLSCutType == 1)
	{//��Ƭ��Ӳ��
		if (pMediaSource->fTSFileWrite != NULL)
		{
			pMediaSource->fTSFileWriteByteCount += bytes;
		    fwrite((char*)data,1, bytes, (FILE*)pMediaSource->fTSFileWrite) ? 0 : ferror((FILE*)pMediaSource->fTSFileWrite);
		}
    }
	else if (ABL_MediaServerPort.nHLSCutType == 2)
	{//��Ƭ���ڴ� Default_TS_MediaFileByteCount
		if (pMediaSource->nMaxTsFileCacheBufferSize - pMediaSource->fTSFileWriteByteCount < bytes)
		{//�ռ䲻������Ҫ����
			std::lock_guard<std::mutex> lock(pMediaSource->mediaTsMp4CutLock);//��ס

			unsigned char * pTempData = pMediaSource->pTsFileCacheBuffer;
			WriteLog(Log_Debug, "CMediaStreamSource= %X ,ý��Դ = %s ,���洢�ռ�,��Ҫ���� nMaxTsFileCacheBufferSize = %d�������Ѿ��洢 fTSFileWriteByteCount = %d��ʣ�� %d �ֽ� ", pMediaSource, pMediaSource->m_szURL, pMediaSource->nMaxTsFileCacheBufferSize, pMediaSource->fTSFileWriteByteCount, pMediaSource->nMaxTsFileCacheBufferSize - pMediaSource->fTSFileWriteByteCount);

			pMediaSource->nMaxTsFileCacheBufferSize = bytes + (1024*1024*2) ;
			pMediaSource->pTsFileCacheBuffer = new unsigned char[pMediaSource->nMaxTsFileCacheBufferSize];

			memcpy(pMediaSource->pTsFileCacheBuffer, pTempData, pMediaSource->fTSFileWriteByteCount);
			SAFE_ARRAY_DELETE(pTempData);
		}

		if (pMediaSource->nMaxTsFileCacheBufferSize - pMediaSource->fTSFileWriteByteCount >= bytes)
		{
			memcpy(pMediaSource->pTsFileCacheBuffer + pMediaSource->fTSFileWriteByteCount, data, bytes);
			pMediaSource->fTSFileWriteByteCount += bytes;
		}
  	}

 	//WriteLog(Log_Debug, "CMediaStreamSource= %X ,ý��Դ = %s ,mp4��Ƭ�ļ������ڳ��� fMP4FileLength = %d ", pMediaSource, pMediaSource->m_szURL, bytes);

#if 0
	pMediaSource->nTsFileOrder ++;
 	sprintf(pMediaSource->szOutputName, "%s%d.mp4", pMediaSource->szHLSPath, pMediaSource->nTsFileOrder);

	FILE* fp = fopen(pMediaSource->szOutputName, "w+b");
	fwrite(data, 1, bytes, fp);
	fclose(fp);
#endif  

	return 0;
}

//H265 ת�� H264
bool  CMediaStreamSource::H265ConvertH264(unsigned char* szVideo, int nLength, char* szVideoCodec)
{
	//Mod by ZXT \ ¼��طŽ�������������ת�� 
	if (m_h265ConvertH264Struct.H265ConvertH264_enable == 0 || nMediaSourceType == MediaSourceType_ReplayMedia)
		return true;

	//�����������ˮӡ��H264Ҳ��Ҫ�����ٱ���
	if (strcmp(szVideoCodec, "H264") == 0 && m_h265ConvertH264Struct.H264DecodeEncode_enable == 0)
		return true;

	//ǿ�ư�H265�޸�ΪH264
	if (strlen(m_mediaCodecInfo.szVideoName) != 0 && strcmp(m_mediaCodecInfo.szVideoName, "H264") != 0)
		strcpy(m_mediaCodecInfo.szVideoName, "H264");
	
	nCudaDecodeFrameCount = 0 ;
	if (ABL_MediaServerPort.H265DecodeCpuGpuType == 0)
	{
		nCudaDecodeFrameCount = 1 ;//��⡢�����ÿ�ζ���1֡ 

 		if (!videoDecode.m_bInitDecode)
			videoDecode.startDecode(szVideoCodec, 1920, 1080);

		if (videoDecode.DecodeYV12Image(szVideo, nLength) < 0)
			return false;
		
		//����ʧ��
		if (!(videoDecode.m_nWidth > 0 && videoDecode.m_nHeight > 0))
			return false;

		 //�޸�ת����� Ϊ -1 ���� ���ڵ���ԭ�ߴ磬��ԭ�ߴ��С���
		if ( (m_h265ConvertH264Struct.convertOutWidth == -1 && m_h265ConvertH264Struct.convertOutHeight == -1 ) )
		{
			m_h265ConvertH264Struct.convertOutWidth  =  videoDecode.m_nWidth;
			m_h265ConvertH264Struct.convertOutHeight =  videoDecode.m_nHeight;
		}

		if (m_h265ConvertH264Struct.convertOutWidth > 0 && m_h265ConvertH264Struct.convertOutHeight > 0 && m_h265ConvertH264Struct.convertOutWidth != videoDecode.m_nWidth &&  m_h265ConvertH264Struct.convertOutHeight != videoDecode.m_nHeight)
		{//��Ҫ����
			if (!avFrameSWS.bInitFlag)
			{
				if (videoDecode.m_nWidth > 0 && videoDecode.m_nHeight > 0 && videoDecode.pDPicture->data[0] != NULL)
				{
					avFrameSWS.CreateAVFrameSws(false, (AVPixelFormat)videoDecode.pDPicture->format, videoDecode.m_nWidth, videoDecode.m_nHeight, AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, SWS_BICUBIC);
				}
			}
			if (avFrameSWS.bInitFlag)
			{
				avFrameSWS.AVFrameSWS(videoDecode.pDPicture);
				if (!videoEncode.m_bInitFlag)
				{
					pOutEncodeBuffer = new unsigned char[((m_h265ConvertH264Struct.convertOutWidth * m_h265ConvertH264Struct.convertOutHeight) * 3) / 2];
					if (videoEncode.StartEncode("libx264", AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, 25, m_h265ConvertH264Struct.convertOutBitrate))
					{//libopenh264
						nConvertObjectCount++;
						H265ConvertH264_enable = true;
						WriteLog(Log_Debug, " CMediaStreamSource  = %X ,��ǰý���� /%s/%s ִ��ת�� ������ %d ·����ת�� ", this, app, stream, nConvertObjectCount);
					}
				}
#ifdef WriteCudaDecodeYUVFlag
				nWriteYUVCount++;
				if (nWriteYUVCount <= 40)
				{
					fwrite(avFrameSWS.szDestData, 1, avFrameSWS.numBytes2, fCudaWriteYUVFile);
					fflush(fCudaWriteYUVFile);
				}
#endif
				return videoEncode.EncodecYUV(avFrameSWS.szDestData, avFrameSWS.numBytes2, pOutEncodeBuffer, &nOutLength);
			}
			else
				return false;
		}
		else if (m_h265ConvertH264Struct.convertOutWidth == videoDecode.m_nWidth && m_h265ConvertH264Struct.convertOutHeight == videoDecode.m_nHeight)
		{//ԭ�ߴ����
			if (!videoEncode.m_bInitFlag)
			{
				if (videoEncode.StartEncode("libx264", (AVPixelFormat)videoDecode.pDPicture->format, videoDecode.m_nWidth, videoDecode.m_nHeight, 25, m_h265ConvertH264Struct.convertOutBitrate))
				{//libopenh264
					pOutEncodeBuffer = new unsigned char[((m_h265ConvertH264Struct.convertOutWidth * m_h265ConvertH264Struct.convertOutHeight) * 3) / 2];
					nConvertObjectCount ++;
					H265ConvertH264_enable = true;
					WriteLog(Log_Debug, " CMediaStreamSource  = %X ,��ǰý���� /%s/%s ִ��ת�� ������ %d ·����ת�� ", this, app, stream, nConvertObjectCount);
				}
			}
			return videoEncode.EncodecAVFrame(videoDecode.pDPicture, pOutEncodeBuffer, &nOutLength);
		}
		else
			return false;
	}
	else if (ABL_MediaServerPort.H265DecodeCpuGpuType == 1)
	{//cudaӲ��
#ifdef OS_System_Windows 
		if (nCudaDecodeChan == 0 && cudaCodec_Init != NULL )
		{
			if (CheckVideoIsIFrame(szVideoCodec, szVideo, nLength))
			{
				if (GetVideoWidthHeight(szVideoCodec, szVideo, nLength))
				{
					if (strcmp(szVideoCodec, "H264") == 0)
						cudaCodec_CreateVideoDecode(cudaCodecVideo_H264, cudaCodecVideo_YV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, nCudaDecodeChan);
					else if (strcmp(szVideoCodec, "H265") == 0)
						cudaCodec_CreateVideoDecode(cudaCodecVideo_HEVC, cudaCodecVideo_YV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, nCudaDecodeChan);

					//�޸�ת����� ���� ת������Ŀ��� ����ԭʼ��Ƶ���� ��ǿ��Ϊԭ�ߴ����
					if ((m_h265ConvertH264Struct.convertOutWidth == -1 && m_h265ConvertH264Struct.convertOutHeight == -1))
					{//ԭ�ߴ����
						m_h265ConvertH264Struct.convertOutWidth = nSrcWidth;
						m_h265ConvertH264Struct.convertOutHeight = nSrcHeight;
					}
 				}
 		    }
 		}
		 
		if (nCudaDecodeChan > 0)
		{
			pCudaDecodeYUVFrame = cudaCodec_CudaVideoDecode(nCudaDecodeChan, szVideo, nLength, nCudaDecodeFrameCount, nCudeDecodeOutLength);
			if (nCudeDecodeOutLength > 0 && pCudaDecodeYUVFrame != NULL)
			{
				if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
				{//��Ҫ����
					if (!avFrameSWS.bInitFlag && m_h265ConvertH264Struct.convertOutWidth > 0 && m_h265ConvertH264Struct.convertOutHeight > 0)
					{
						avFrameSWS.CreateAVFrameSws(true, (AVPixelFormat)AV_PIX_FMT_YUV420P, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, SWS_BICUBIC);
					}

					if (avFrameSWS.bInitFlag)
					{
						if (!videoEncode.m_bInitFlag)
						{
							pOutEncodeBuffer = new unsigned char[CudaDecodeH264EncodeH264FIFOBufferLength];
							if (videoEncode.StartEncode("libx264", AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, 25, m_h265ConvertH264Struct.convertOutBitrate))
							{//libopenh264
								nConvertObjectCount++;
								H265ConvertH264_enable = true;
								WriteLog(Log_Debug, " CMediaStreamSource  = %X ,��ǰý���� /%s/%s ִ��ת�� ������ %d ·����ת�� ", this, app, stream, nConvertObjectCount);
							}
						}
					}
				}
				else if (m_h265ConvertH264Struct.convertOutWidth == nSrcWidth && m_h265ConvertH264Struct.convertOutHeight == nSrcHeight)
				{//ԭ�ߴ����
					if (pOutEncodeBuffer == NULL)
					{
						pOutEncodeBuffer = new unsigned char[CudaDecodeH264EncodeH264FIFOBufferLength];
						if (videoEncode.StartEncode("libx264", AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, 25, m_h265ConvertH264Struct.convertOutBitrate))
						{//libopenh264
							nConvertObjectCount++;
							H265ConvertH264_enable = true;
							WriteLog(Log_Debug, " CMediaStreamSource  = %X ,��ǰý���� /%s/%s ִ��ת�� ������ %d ·����ת�� ", this, app, stream, nConvertObjectCount);
						}
					}
				}

				nEncodeBufferLengthCount = nOutLength = 0;
				for (int i = 0; i < nCudaDecodeFrameCount; i++)
				{
					if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
						avFrameSWS.AVFrameSWSYUV(pCudaDecodeYUVFrame, nCudeDecodeOutLength);

					if (nCudaDecodeFrameCount == 1)//ֻ��1֡
					{
						if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
							videoEncode.EncodecYUV(avFrameSWS.szDestData, avFrameSWS.numBytes2, pOutEncodeBuffer, &nOutLength);
						else if (m_h265ConvertH264Struct.convertOutWidth == nSrcWidth && m_h265ConvertH264Struct.convertOutHeight == nSrcHeight)
							videoEncode.EncodecYUV(pCudaDecodeYUVFrame, nCudeDecodeOutLength, pOutEncodeBuffer, &nOutLength);
					}
					else
					{//��֡ 
						if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
							videoEncode.EncodecYUV(avFrameSWS.szDestData, avFrameSWS.numBytes2, pOutEncodeBuffer + (nEncodeBufferLengthCount + sizeof(int)), &nOutLength);
						else if (m_h265ConvertH264Struct.convertOutWidth == nSrcWidth && m_h265ConvertH264Struct.convertOutHeight == nSrcHeight)
							videoEncode.EncodecYUV(pCudaDecodeYUVFrame, nCudeDecodeOutLength, pOutEncodeBuffer + (nEncodeBufferLengthCount + sizeof(int)), &nOutLength);

						if (nOutLength > 0 && (CudaDecodeH264EncodeH264FIFOBufferLength - nEncodeBufferLengthCount) > (nOutLength + sizeof(nOutLength)))
						{
							memcpy(pOutEncodeBuffer + nEncodeBufferLengthCount, (unsigned char*)&nOutLength, sizeof(nOutLength));
							nEncodeBufferLengthCount += nOutLength + sizeof(nOutLength);
						}
						else
							nEncodeBufferLengthCount = 0;
					}

#ifdef WriteCudaDecodeYUVFlag
					nWriteYUVCount++;
					if (nWriteYUVCount <= 40 )
					{
						fwrite(pCudaDecodeYUVFrame[i], 1, nCudeDecodeOutLength, fCudaWriteYUVFile);
						fflush(fCudaWriteYUVFile);
					}
#endif
				}

				return true;
			}
		}
		else
			return false;
#else
		if (nCudaDecodeChan == 0 && cudaCodec_Init != NULL )
		{
			if (CheckVideoIsIFrame(szVideoCodec, szVideo, nLength))
			{
				if (GetVideoWidthHeight(szVideoCodec, szVideo, nLength))
				{
					if(m_mediaCodecInfo.nWidth >= 1280)
					{
					  if (strcmp(szVideoCodec, "H264") == 0)
						cudaCodec_CreateVideoDecode(cudaCodecVideo_H264, cudaCodecVideo_NV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, nCudaDecodeChan);
					 else if (strcmp(szVideoCodec, "H265") == 0)
						cudaCodec_CreateVideoDecode(cudaCodecVideo_HEVC, cudaCodecVideo_NV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, nCudaDecodeChan);
					}

					//�޸�ת����� ���� ת������Ŀ��� ����ԭʼ��Ƶ���� ��ǿ��Ϊԭ�ߴ����
					if ((m_h265ConvertH264Struct.convertOutWidth == -1 && m_h265ConvertH264Struct.convertOutHeight == -1))
					{//ԭ�ߴ����
						m_h265ConvertH264Struct.convertOutWidth = nSrcWidth;
						m_h265ConvertH264Struct.convertOutHeight = nSrcHeight;
					}
				}
 		    }
			
	        /* 
		     //��ʱ���� Linux ƽ̨�Եͷֱ���265��ת��  
			if(m_mediaCodecInfo.nWidth > 0 && m_mediaCodecInfo.nWidth < 1280 )
			{//Linux ƽ̨Ӳ��ת��704 X 576 ��ɱ�����������Щ�ֱ��ʶ���ת�룬ֱ�������������ɱ���
 				H265ConvertH264_enable = true ;
				
				nCudaDecodeFrameCount = 1;
				nOutLength = nLength ;
				if (pOutEncodeBuffer == NULL)
				{
					nConvertObjectCount ++;
 					pOutEncodeBuffer = new unsigned char[CudaDecodeH264EncodeH264FIFOBufferLength];
				}
				memcpy(pOutEncodeBuffer,szVideo,nLength);
				return true ;
			}*/
 		}
		 
		if (nCudaDecodeChan > 0)
		{
			pCudaDecodeYUVFrame = cudaCodec_CudaVideoDecode(nCudaDecodeChan, szVideo, nLength, nCudaDecodeFrameCount, nCudeDecodeOutLength);
			if (nCudeDecodeOutLength > 0 && pCudaDecodeYUVFrame != NULL )
			{
				if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
				{//��Ҫ����
					if (!avFrameSWS.bInitFlag && m_h265ConvertH264Struct.convertOutWidth > 0 && m_h265ConvertH264Struct.convertOutHeight > 0)
					{
						avFrameSWS.CreateAVFrameSws(true, (AVPixelFormat)AV_PIX_FMT_NV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, SWS_BICUBIC);
					}

					if (avFrameSWS.bInitFlag)
					{
						if (nEncodeCudaChan == 0)
						{
							pOutEncodeBuffer = new unsigned char[CudaDecodeH264EncodeH264FIFOBufferLength];
							if (cudaEncode_CreateVideoEncode((cudaEncodeVideo_enum)cudaEncodeVideo_H264, (cudaEncodeVideo_enum)cudaEncodeVideo_YUV420, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, nEncodeCudaChan))
							{//cuda Ӳ����
								nConvertObjectCount++;
								H265ConvertH264_enable = true;
								WriteLog(Log_Debug, " CMediaStreamSource  = %X ,��ǰý���� /%s/%s ִ��ת�� ������ %d ·����ת�� ", this, app, stream, nConvertObjectCount);
							}
						}
					}
				}
				else if (m_h265ConvertH264Struct.convertOutWidth == nSrcWidth && m_h265ConvertH264Struct.convertOutHeight == nSrcHeight)
				{//ԭ�ߴ����
					if (nEncodeCudaChan == 0)
					{
						pOutEncodeBuffer = new unsigned char[CudaDecodeH264EncodeH264FIFOBufferLength];
						if (cudaEncode_CreateVideoEncode((cudaEncodeVideo_enum)cudaEncodeVideo_H264, (cudaEncodeVideo_enum)cudaEncodeVideo_NV12, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, nEncodeCudaChan))
						{//cuda Ӳ����
							nConvertObjectCount++;
							H265ConvertH264_enable = true;
							WriteLog(Log_Debug, " CMediaStreamSource  = %X ,��ǰý���� /%s/%s ִ��ת�� ������ %d ·����ת�� ", this, app, stream, nConvertObjectCount);
						}
						if (ABL_MediaServerPort.filterVideo_enable == 1 && !avFrameSWS.bInitFlag && m_h265ConvertH264Struct.convertOutWidth > 0 && m_h265ConvertH264Struct.convertOutHeight > 0)
						{
							avFrameSWS.CreateAVFrameSws(true, (AVPixelFormat)AV_PIX_FMT_NV12, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, SWS_BICUBIC);
						}

					}
				}
				else
					return false;

				nEncodeBufferLengthCount = nOutLength = 0;
				for (int i = 0; i < nCudaDecodeFrameCount; i++)
				{
					if (pOutEncodeBuffer != NULL)
					{
						if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
						   avFrameSWS.AVFrameSWSYUV(pCudaDecodeYUVFrame, nCudeDecodeOutLength);

						if (m_h265ConvertH264Struct.H265ConvertH264_enable == 1 && ABL_MediaServerPort.filterVideo_enable == 1)
						{
							if (pFFVideoFilter == NULL)
							{
								pFFVideoFilter = new CFFVideoFilter();
								if (pFFVideoFilter)
								{
								   if (m_h265ConvertH264Struct.convertOutWidth == nSrcWidth && m_h265ConvertH264Struct.convertOutHeight == nSrcHeight)
									  pFFVideoFilter->StartFilter(AV_PIX_FMT_NV12, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, 25, ABL_MediaServerPort.nFilterFontSize, ABL_MediaServerPort.nFilterFontColor, ABL_MediaServerPort.nFilterFontAlpha, ABL_MediaServerPort.nFilterFontLeft, ABL_MediaServerPort.nFilterFontTop);
									else 
									  pFFVideoFilter->StartFilter(AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, 25, ABL_MediaServerPort.nFilterFontSize, ABL_MediaServerPort.nFilterFontColor, ABL_MediaServerPort.nFilterFontAlpha, ABL_MediaServerPort.nFilterFontLeft, ABL_MediaServerPort.nFilterFontTop);
								}
							}
							if (pFFVideoFilter)
							{//��ӡˮӡ
								if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
								{
									if (pFFVideoFilter->FilteringFrame(avFrameSWS.pFrameDest))
										pFFVideoFilter->CopyYUVData(avFrameSWS.szDestData);
								}
								else
								{
									memcpy(avFrameSWS.szDestData, pCudaDecodeYUVFrame, nCudeDecodeOutLength);//����Ӳ�����YUV��Ŀ��AVFrame 
									if (pFFVideoFilter->FilteringFrame(avFrameSWS.pFrameDest)) //ִ��ˮӡ
										pFFVideoFilter->CopyYUVData(pCudaDecodeYUVFrame); //�Ѵ�����ˮӡ������Ӳ�����YUV
								}
							}
						}
 					
						if (nCudaDecodeFrameCount == 1)
						{
							if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
								nOutLength = cudaEncode_CudaVideoEncode(nEncodeCudaChan, (unsigned char*)avFrameSWS.szDestData, avFrameSWS.numBytes2, (char*)pOutEncodeBuffer);
							else if (m_h265ConvertH264Struct.convertOutWidth == nSrcWidth && m_h265ConvertH264Struct.convertOutHeight == nSrcHeight)
								nOutLength = cudaEncode_CudaVideoEncode(nEncodeCudaChan, pCudaDecodeYUVFrame, nCudeDecodeOutLength, (char*)pOutEncodeBuffer);
						}
						else
						{
							if (m_h265ConvertH264Struct.convertOutWidth != nSrcWidth && m_h265ConvertH264Struct.convertOutHeight != nSrcHeight)
								nOutLength = cudaEncode_CudaVideoEncode(nEncodeCudaChan, (unsigned char*)avFrameSWS.szDestData, avFrameSWS.numBytes2, (char*)pOutEncodeBuffer + (nEncodeBufferLengthCount + sizeof(int)));
							else if (m_h265ConvertH264Struct.convertOutWidth == nSrcWidth && m_h265ConvertH264Struct.convertOutHeight == nSrcHeight)
								nOutLength = cudaEncode_CudaVideoEncode(nEncodeCudaChan, pCudaDecodeYUVFrame, nCudeDecodeOutLength, (char*)pOutEncodeBuffer + (nEncodeBufferLengthCount + sizeof(int)));

 							if (nOutLength > 0 && (CudaDecodeH264EncodeH264FIFOBufferLength - nEncodeBufferLengthCount) > (nOutLength + sizeof(nOutLength))  )
							{
 								memcpy(pOutEncodeBuffer + nEncodeBufferLengthCount, (unsigned char*)&nOutLength, sizeof(nOutLength));
								nEncodeBufferLengthCount += nOutLength + sizeof(nOutLength);
#ifdef WriteEncodeDataFlag 
								if (writeEncodeFile)
								{
									fwrite(pOutEncodeBuffer, 1, nOutLength, writeEncodeFile);
									fflush(writeEncodeFile);
								}
#endif					   
							}else
							   nEncodeBufferLengthCount = 0;
						}
 					}
				}

				return true;
			}
			else
				return false;
		}	
#endif
    }
	else if (ABL_MediaServerPort.H265DecodeCpuGpuType == 2)
	{//AMD�Կ�Ӳ��

	}
	else
		return true;
}

bool CMediaStreamSource::PushVideo(unsigned char* szVideo, int nLength, char* szVideoCodec)
{//ֱ�ӿ�����ÿ�����緢�Ͷ��� 
	std::lock_guard<std::mutex> lock(mediaSendMapLock);
 
	if (!(strcmp(szVideoCodec, "H264") == 0 || strcmp(szVideoCodec, "H265") == 0) || !bEnableFlag)
		return false;

#ifdef WriteInputVideoFileFlag
	if (fWriteInputVideoFile && nLength > 0)
	{
		fwrite(szVideo, 1, nLength, fWriteInputVideoFile);
		fflush(fWriteInputVideoFile);
	}
#endif

	//�����¼�֪ͨ�����ڷ�����Ȩ
	if (ABL_MediaServerPort.hook_enable == 1  && !m_bNoticeOnPublish)
	{
		auto pClient = GetNetRevcBaseClient(nClient);
		if (pClient)
		{
			m_bNoticeOnPublish = true;
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_on_publish;
 			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_publish\",\"id\":\"%d\",\"app\":\"%s\",\"stream\":\"%s\",\"sourceURL\":\"%s\",\"schema\":\"%d\",\"mediaServerId\":\"%s\",\"mediaSourceCount\":%d,\"networkType\":%d,\"key\":%llu,\"ip\":\"%s\" ,\"port\":%d,\"params\":\"%s\",\"vhost\":\"__defaultVhost__\"}", pClient->nClient, app, stream,sourceURL,pClient->netBaseNetType, ABL_MediaServerPort.mediaServerID, mediaSourceCount, pClient->netBaseNetType, pClient->nClient, pClient->szClientIP, pClient->nClientPort, pClient->szPlayParams);
 			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		}
  	}
	else 
	{//Ҫ��֤ �����¼����ٷ��������ﵽ�¼� 
		if (!m_bNoticeOnPublish && ABL_MediaServerPort.hook_enable == 1)
			m_bNoticeOnPublish = true;
	}

	//I ֡����֪ͨ
	if (ABL_MediaServerPort.hook_enable == 1 && iFrameArriveNoticCount < ABL_MediaServerPort.iframeArriveNoticCount )
	{
		if (CheckVideoIsIFrame(szVideoCodec, szVideo, nLength) == true)
		{//ΪI֡
			iFrameArriveNoticCount ++;
			auto pClient = GetNetRevcBaseClient(nClient);
			if (pClient)
			{
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = NetBaseNetType_HttpClient_on_iframe_arrive;
				sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_iframe_arrive\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu,\"ip\":\"%s\" ,\"port\":%d,\"NotificationNumber\":%d}", app, stream, ABL_MediaServerPort.mediaServerID, pClient->netBaseNetType, pClient->nClient, pClient->szClientIP, pClient->nClientPort, iFrameArriveNoticCount);
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			}
		}
   }

#ifdef  WriteInputVdideoFlag
	if (fWriteInputVideo)
	{
		fwrite(szVideo,1,nLength,fWriteInputVideo);
		fflush(fWriteInputVideo);
	}
#endif

	//������Ƶ����
	if (GetTickCount64() - nCalcBitrateTimestamp >= 10000)
	{
		nCalcBitrateTimestamp = GetTickCount64();
		m_mediaCodecInfo.nVideoBitrate = nVideoBitrate / 1000;
		m_mediaCodecInfo.nAudioBitrate = nAudioBitrate / 1000;
		nVideoBitrate = 0;
		nAudioBitrate = 0;
	}
	nVideoBitrate += nLength;

	if (nVideoStampAdd == 0)
	{
	   nVideoStampAdd = 1000 / m_mediaCodecInfo.nVideoFrameRate;
	   if (CreatePictureFileSource(app, stream))
		 WriteLog(Log_Debug, " ����ͼƬ�ļ�Դ�ɹ� app = %s ,stream = %s  ", app, stream);
 	}

	//Mod
	if (nMediaSourceType == MediaSourceType_LiveMedia && nConvertObjectCount < ABL_MediaServerPort.convertMaxObject && ((m_h265ConvertH264Struct.H265ConvertH264_enable == 1 && strcmp(szVideoCodec, "H265") == 0) || (m_h265ConvertH264Struct.H264DecodeEncode_enable == 1 && strcmp(szVideoCodec, "H264") == 0) ))
	{//ִ��ת��
 		if (!H265ConvertH264(szVideo, nLength, szVideoCodec))
			return false;
		if (H265ConvertH264_enable == true && (m_mediaCodecInfo.nWidth > 0 && m_mediaCodecInfo.nHeight > 0) && !(m_mediaCodecInfo.nWidth == m_h265ConvertH264Struct.convertOutWidth && m_mediaCodecInfo.nHeight == m_h265ConvertH264Struct.convertOutHeight))
		{//���Ŀ��� 
			m_mediaCodecInfo.nWidth = m_h265ConvertH264Struct.convertOutWidth;
			m_mediaCodecInfo.nHeight = m_h265ConvertH264Struct.convertOutHeight;
		}
	}
	else if (H265ConvertH264_enable == true && nMediaSourceType == MediaSourceType_LiveMedia)
	{//�Ѿ��ɹ����䵽ת����Դ��
		if (!H265ConvertH264(szVideo, nLength, szVideoCodec))
			return false;
		if ((m_mediaCodecInfo.nWidth > 0 && m_mediaCodecInfo.nHeight > 0) && !(m_mediaCodecInfo.nWidth == m_h265ConvertH264Struct.convertOutWidth && m_mediaCodecInfo.nHeight == m_h265ConvertH264Struct.convertOutHeight))
		{//���Ŀ��� 
			m_mediaCodecInfo.nWidth = m_h265ConvertH264Struct.convertOutWidth;
			m_mediaCodecInfo.nHeight = m_h265ConvertH264Struct.convertOutHeight;
		}
	}
	else if (H265ConvertH264_enable == false)
	{//û�з��䵽ת����Դ�ģ���Ҫ�ָ�ԭ����ý���ʽ 
		if(strcmp(m_mediaCodecInfo.szVideoName, szVideoCodec) != 0)
		   strcpy(m_mediaCodecInfo.szVideoName, szVideoCodec);
	}

	//��ȡ��Ƶ����
	if (H265ConvertH264_enable)
	{ //��¼��Ƶ��ʽ
	   if (strlen(m_mediaCodecInfo.szVideoName) == 0)
		 strcpy(m_mediaCodecInfo.szVideoName,"H264");

	   if (pOutEncodeBuffer && nOutLength > 0)
		   GetVideoWidthHeight("H264",pOutEncodeBuffer, nOutLength);
	   else
		   return false ;
	}
	else
	{
		//�������Ը�·����ת��ʱ�����ǳ��Դ���ת����������ת�룬��������������Ƶ�����ʽ
		if (!(nConvertObjectCount < ABL_MediaServerPort.convertMaxObject && m_h265ConvertH264Struct.H265ConvertH264_enable == 1))
		{
			if (strlen(m_mediaCodecInfo.szVideoName) == 0)
			  strcpy(m_mediaCodecInfo.szVideoName, szVideoCodec);
		}

		if (nMediaSourceType == MediaSourceType_LiveMedia)
	       GetVideoWidthHeight(m_mediaCodecInfo.szVideoName,szVideo, nLength);
		else 
		   GetVideoWidthHeight(m_mediaCodecInfo.szVideoName, szVideo + 4, nLength - 4);
	}

	if (enable_hls == 1 && m_mediaCodecInfo.nWidth > 0 && m_mediaCodecInfo.nHeight > 0 )
	{
		InitHlsResoure();
		tsFileNameFifo.InitFifo(1024 * 512);
		m3u8FileFifo.InitFifo(1025 * 256);

		if (strcmp(m_mediaCodecInfo.szVideoName, "H264") == 0 || (strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 1) )
		{//h264 ��ƬΪ TS ,���� H265 ѡ����ƬΪTS 
			if (tsPacketHandle == NULL)
			{
 				tshandler.alloc = ts_alloc;
				tshandler.write = ts_write;
				tshandler.free = ts_free;

				srand(GetTickCount());
				sprintf(szOutputName, "%s%d.ts", szHLSPath, nTsFileOrder);
				if(ABL_MediaServerPort.nHLSCutType == 1) //��Ƭ��Ӳ��
				  fTSFileWrite = fopen(szOutputName, "w+b");

				strcpy(szHookTSFileName, szOutputName);
 
 				tsPacketHandle = mpeg_ts_create(&tshandler, (void*)this);

				if (ABL_MediaServerPort.nHLSCutType == 1) //��Ƭ��Ӳ��
				  tsFileNameFifo.push((unsigned char*)szOutputName, strlen(szOutputName));
 			}
		} 
		else if (strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 2 && hlsFMP4 == NULL)
		{//H265 ��ƬΪ mp4 
			   sprintf(szOutputName, "%s%d.mp4", szHLSPath, nTsFileOrder);
			   if (ABL_MediaServerPort.nHLSCutType == 1) //��Ƭ��Ӳ��
				   fTSFileWrite = fopen(szOutputName, "w+b");

			   if(ABL_MediaServerPort.hlsCutTime >= 1 && ABL_MediaServerPort.hlsCutTime <= 10 )
				   hlsFMP4 = hls_fmp4_create(ABL_MediaServerPort.hlsCutTime * 1000, hls_segment, this);
			   else 
				   hlsFMP4 = hls_fmp4_create(1 * 1000, hls_segment, this);
		}
	}

	if (enable_hls == 1 && m_mediaCodecInfo.nWidth > 0 && m_mediaCodecInfo.nHeight > 0)
	{
		if (tsPacketHandle != NULL && (strcmp(m_mediaCodecInfo.szVideoName, "H264") == 0 || (strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 1)))
		{//H264
			if (H265ConvertH264_enable)
			{
			    if(nCudaDecodeFrameCount == 1) //ֻ��1֡
				  H264H265FrameToTSFile(pOutEncodeBuffer, nOutLength);
				else 
				{
					if (pOutEncodeBuffer != NULL)
					{
						nOneFrameLength = nGetFrameCountLength = 0 ;
 						for(int i=0;i<nCudaDecodeFrameCount;i++)
						{//��֡
							memcpy((char*)&nOneFrameLength, pOutEncodeBuffer + nGetFrameCountLength, sizeof(nOneFrameLength));
							H264H265FrameToTSFile(pOutEncodeBuffer + ( nGetFrameCountLength + sizeof(nOneFrameLength)), nOneFrameLength);
 							nGetFrameCountLength += nOneFrameLength + sizeof(nOneFrameLength);
 						}
					}
				}
			}
			else
			{
				if (nMediaSourceType == MediaSourceType_LiveMedia)
			      H264H265FrameToTSFile(szVideo, nLength);
				else 
				  H264H265FrameToTSFile(szVideo + 4, nLength - 4);
			}
  		}
		else if (hlsFMP4 != NULL && strcmp(m_mediaCodecInfo.szVideoName, "H265") == 0 && ABL_MediaServerPort.nH265CutType == 2)
		{//H265
			if (H265ConvertH264_enable)
			{
				if (nCudaDecodeFrameCount == 1)
				  H265FrameToFMP4File(pOutEncodeBuffer, nOutLength);
				else
				{
					if (pOutEncodeBuffer != NULL)
					{
						nOneFrameLength = nGetFrameCountLength = 0;
						for (int i = 0; i<nCudaDecodeFrameCount; i++)
						{//��֡
							memcpy((char*)&nOneFrameLength, pOutEncodeBuffer + nGetFrameCountLength, sizeof(nOneFrameLength));
							H265FrameToFMP4File(pOutEncodeBuffer + (nGetFrameCountLength + sizeof(nOneFrameLength)), nOneFrameLength);
							nGetFrameCountLength += nOneFrameLength + sizeof(nOneFrameLength);
						}
					}
				}
			}
			else
			{
				if (nMediaSourceType == MediaSourceType_LiveMedia)
 			      H265FrameToFMP4File(szVideo, nLength);
				else 
				  H265FrameToFMP4File(szVideo + 4 , nLength - 4);
			}
  		}

		videoDts += nVideoStampAdd;
 	}

	//����¼��MP4����
	if (enable_mp4 == true && recordMP4 == 0)
	{
		//������·�� 
#ifdef  OS_System_Windows
		sprintf(szRecordPath, "%s%s", ABL_MediaServerPort.recordPath, app);
		::CreateDirectory(szRecordPath,NULL);

		sprintf(szRecordPath, "%s%s\\%s\\", ABL_MediaServerPort.recordPath, app,stream);
		::CreateDirectory(szRecordPath,NULL);
#else
		sprintf(szRecordPath, "%s%s", ABL_MediaServerPort.recordPath, app);
		umask(0);
		mkdir(szRecordPath, 777);
		ABL_SetPathAuthority(szRecordPath);
		sprintf(szRecordPath, "%s%s/%s/", ABL_MediaServerPort.recordPath, app, stream);
		mkdir(szRecordPath, 777);
		ABL_SetPathAuthority(szRecordPath);
#endif
#ifdef USE_BOOST

		boost::shared_ptr<CRecordFileSource> recordFileSource = NULL;
#else
		std::shared_ptr<CRecordFileSource> recordFileSource = NULL;
#endif
	
		if ((recordFileSource = CreateRecordFileSource(app, stream)) != NULL )
 			WriteLog(Log_Debug, " �����ļ�Դ�ɹ� app = %s ,stream = %s  ", app, stream );
		 else
 			recordFileSource = GetRecordFileSource(m_szURL);
		if (recordFileSource)
		{
			recordFileSource->fileKeepMaxTime = fileKeepMaxTime; 
			WriteLog(Log_Debug, "�������� app = %s ,stream = %s ¼�񱣴�ʱ��Ϊ %llu Сʱ ", app, stream, fileKeepMaxTime);
		}

 		recordMP4 = XHNetSDK_GenerateIdentifier();
#ifdef USE_BOOST

		boost::shared_ptr<CNetRevcBase> mp4Client = NULL;
#else
		std::shared_ptr<CNetRevcBase> mp4Client = NULL;
#endif
	
		if(videoFileFormat == 1)//fmp4
		   mp4Client = CreateNetRevcBaseClient(NetBaseNetType_RecordFile_FMP4, 0, recordMP4, "", 0, m_szURL);
		else if(videoFileFormat == 2)//mp4
		   mp4Client = CreateNetRevcBaseClient(NetBaseNetType_RecordFile_MP4, 0, recordMP4, "", 0, m_szURL);
		else if (videoFileFormat == 3)//ts
			mp4Client = CreateNetRevcBaseClient(NetBaseNetType_RecordFile_TS, 0, recordMP4, "", 0, m_szURL);
		if (mp4Client)
		{
			WriteLog(Log_Debug, "����¼��MP4����ɹ� app = %s ,stream = %s , recordMP4 = %llu ,szRecordPath = %s ", app, stream, recordMP4, szRecordPath);
			//��¼��·��������mp4����
			strcpy(mp4Client->szRecordPath, szRecordPath);

			//����ý����Ϣ
			memcpy((char*)&mp4Client->mediaCodecInfo, (char*)&m_mediaCodecInfo, sizeof(m_mediaCodecInfo));
			strcpy(mp4Client->app, app);
			strcpy(mp4Client->stream, stream);
			mp4Client->key = nClient;

			//����ý�忽��
			mediaSendMap.insert(std::make_pair(recordMP4, mp4Client));
 		}
	}

	//����I֡
	if (ABL_MediaServerPort.ForceSendingIFrame == 1)
	{
		if (H265ConvertH264_enable)
		{//ת��
			if (CheckVideoIsIFrame(m_mediaCodecInfo.szVideoName, pOutEncodeBuffer, nOutLength) && nOutLength > 0 && nOutLength > 256 && nOutLength <= IDRFrameMaxBufferLength)
			{
				pVideoGopFrameBuffer.Reset();
				pCacheAudioFifo.Reset();
			}

			pVideoGopFrameBuffer.push(pOutEncodeBuffer, nOutLength);
  		}
		else
		{//��ת��
			if (CheckVideoIsIFrame(m_mediaCodecInfo.szVideoName, szVideo, nLength) && nLength <= IDRFrameMaxBufferLength)
			{
				pVideoGopFrameBuffer.Reset();
				pCacheAudioFifo.Reset();
			}

			pVideoGopFrameBuffer.push(szVideo, nLength);
		}
	}

	if (mediaSendMap.size() <= 0)
	{
		//����Ƕ�ȡ¼���ļ� 120 �����˹ۿ���ִ��ɾ������ֹ��ѯ¼����ȣ���ɶ�ȡӲ��¼����� 
		if (netBaseNetType == NetBaseNetType_NetServerReadMultRecordFile && (GetCurrentSecond() - nRecordLastWatchTime >= 120 ))
		{
			WriteLog(Log_Debug, "��ѯ������¼���� /%s/%s ������ 120 �����˹ۿ�������ִ��ɾ�� nClient = %llu ", app, stream, nClient);
 			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));

			return false;
		}

		//���˹ۿ���Ϣ,30����֪ͨһ��
		if (ABL_MediaServerPort.hook_enable == 1  && GetCurrentSecond() - nLastWatchTime >= ABL_MediaServerPort.noneReaderDuration)
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_None_reader;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_none_reader\",\"app\":\"%s\",\"stream\":\"%s\",\"sourceURL\":\"%s\",\"noneReaderDuration\":%d,\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu}", app, stream,sourceURL, GetCurrentSecond() - nLastWatchTimeDisconect, ABL_MediaServerPort.mediaServerID, netBaseNetType, nClient);
 			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));

			nLastWatchTime = GetCurrentSecond();
 		}
		
		//���˹ۿ����ʱ��������ر�
		if ((GetCurrentSecond() - nLastWatchTimeDisconect) >= (ABL_MediaServerPort.maxTimeNoOneWatch * 60))
		{
			WriteLog(Log_Debug, "app = %s ,stream = %s  ���˹ۿ��Ѿ��ﵽ %d �� ������ִ��ɾ�� ", app, stream, (GetCurrentSecond() - nLastWatchTimeDisconect));
			nLastWatchTimeDisconect = GetCurrentSecond(); //��ֹ2��ɾ�� 
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		}
 
 		return false;
	}

	//ÿ��3��������ۿ�ʱ��
	if (GetTickCount64() - tCopyVideoTime >= 1000 * 3)
	{
		nLastWatchTime = nRecordLastWatchTime = nLastWatchTimeDisconect = GetCurrentSecond();
		tCopyVideoTime = GetTickCount64();
	}

	
	uint64_t               nClient;
	for (auto it = mediaSendMap.begin(); it != mediaSendMap.end(); )
	{
		auto pClient  = (*it).second;
		if (pClient != NULL)
		{
			//���Ͳ����¼�֪ͨ�����ڲ��ż�Ȩ
			if (ABL_MediaServerPort.hook_enable == 1 && pClient->bOn_playFlag == false)
			{
				pClient->bOn_playFlag = true;
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = NetBaseNetType_HttpClient_on_play;
				sprintf(msgNotice.szMsg, "{\"eventName\":\"on_play\",\"app\":\"%s\",\"stream\":\"%s\",\"readerCount\": %d,\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu,\"ip\":\"%s\" ,\"port\":%d,\"params\":\"%s\"}", app, stream, mediaSendMap.size(), ABL_MediaServerPort.mediaServerID, netBaseNetType, (*it).first,pClient->szClientIP,pClient->nClientPort,pClient->szPlayParams);
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
 			}

			if (pClient->nMediaSourceType != nMediaSourceType)
				pClient->nMediaSourceType = nMediaSourceType;//����ý��Դ����

			//��Ƶ�ȵ���
			if (strlen(pClient->mediaCodecInfo.szVideoName) == 0 && strlen(m_mediaCodecInfo.szVideoName) > 0 )
				memcpy(&pClient->mediaCodecInfo, (char*)&m_mediaCodecInfo, sizeof(MediaCodecInfo));

			//�޸Ŀ���
			if(pClient->mediaCodecInfo.nWidth == 0  && pClient->mediaCodecInfo.nHeight ==  0 && m_mediaCodecInfo.nWidth > 0 && m_mediaCodecInfo.nHeight > 0)
			{
			   pClient->mediaCodecInfo.nWidth = m_mediaCodecInfo.nWidth;
			   pClient->mediaCodecInfo.nHeight = m_mediaCodecInfo.nHeight;
			}

			if (m_bPauseFlag)
			{//��ͣ���� 
				if (!pClient->m_bPauseFlag)
					pClient->m_bPauseFlag = true;
  			    
				it++;
				continue;
			}
			else
			{//�ָ�����
				if (pClient->m_bPauseFlag)
					pClient->m_bPauseFlag = false;
			}

			//���ȼ���SPS��PPS 
			if (pClient->netBaseNetType != NetBaseNetType_WebRtcServerWhepPlayer && pClient->bPushSPSPPSFrameFlag == false && H265ConvertH264_enable == false )
			{
				if (nSPSPPSLength > 0)
				{
					pClient->PushVideo(pSPSPPSBuffer, nSPSPPSLength, m_mediaCodecInfo.szVideoName);
					pClient->bPushSPSPPSFrameFlag = true;
					pClient->SendVideo();
				}
			}
 			
			if (ABL_MediaServerPort.ForceSendingIFrame == 1 && pClient->bSendFirstIDRFrameFlag == false)
			{//��δ��������I֡ 
				//����
				CopyVideoGopFrameBufer();

				unsigned char* pData;
				int            nPopLength = 0;
				while ((pData = pCopyVideoGopFrameBuffer.pop(&nPopLength)) != NULL)
				{
					if(nPopLength > 0)
						pClient->PushVideo(pData, nPopLength, m_mediaCodecInfo.szVideoName);

					pClient->SendVideo();
					pCopyVideoGopFrameBuffer.pop_front();
 				}
 				   
				pClient->bSendFirstIDRFrameFlag = true;
				it++; //���һ֡������ǰ֡���Ѿ��������
				continue;
 			}

			if (H265ConvertH264_enable)
			{//ת��
				if (nCudaDecodeFrameCount == 1) //ֻ��1֡
				{
					pClient->PushVideo(pOutEncodeBuffer, nOutLength, m_mediaCodecInfo.szVideoName);
					pClient->SendVideo();
				}
				else
				{
					if (pOutEncodeBuffer != NULL && nEncodeBufferLengthCount > 0)
					{
						nOneFrameLength = nGetFrameCountLength = 0;
						for (int i = 0; i < nCudaDecodeFrameCount; i++)
						{//��֡
							if (nGetFrameCountLength < CudaDecodeH264EncodeH264FIFOBufferLength)
							{
								memcpy((char*)&nOneFrameLength, pOutEncodeBuffer + nGetFrameCountLength, sizeof(nOneFrameLength));
								pClient->PushVideo(pOutEncodeBuffer + (nGetFrameCountLength + sizeof(nOneFrameLength)), nOneFrameLength, m_mediaCodecInfo.szVideoName);
								pClient->SendVideo();

								nGetFrameCountLength += nOneFrameLength + sizeof(nOneFrameLength);
							}
						}
					}
				}
			}
			else //��ת��
			{
				pClient->PushVideo(szVideo, nLength, m_mediaCodecInfo.szVideoName);
				pClient->SendVideo();
			}
  			it++;
		}
		else
		{//����ʧ�ܣ�֤���������Ѿ��Ͽ� 

			mediaSendMap.erase(it++);
  		}
	}

	return false;
}

bool CMediaStreamSource::PushAudio(unsigned char* szAudio, int nLength, char* szAudioCodec, int nChannels, int SampleRate)
{//ֱ�ӿ�����ÿ�����緢�Ͷ���
	std::lock_guard<std::mutex> lock(mediaSendMapLock);

	if ( !bEnableFlag || ABL_MediaServerPort.nEnableAudio == 0 || !(strcmp(szAudioCodec,"AAC") == 0 || strcmp(szAudioCodec, "MP3") == 0 || strcmp(szAudioCodec, "G711_A") == 0 || strcmp(szAudioCodec, "G711_U") == 0))
		return false;

	//�����ﵽ֪ͨ,ֻ����Ƶ����Ҳ��Ҫ֪ͨ ���� strlen(m_mediaCodecInfo.szVideoName) == 0  ֻ����Ƶ��û����Ƶ ��,��Ҫ�ȴ���Ƶ��ʽ������ ��strlen(m_mediaCodecInfo.szAudioName) > 0��
	if (ABL_MediaServerPort.hook_enable == 1 && strlen(m_mediaCodecInfo.szVideoName) == 0  &&  strlen(m_mediaCodecInfo.szAudioName) > 0 && bNoticeClientArriveFlag == false && (GetTickCount64() - nCreateDateTime > 1000) )
	{
		auto pClient = GetNetRevcBaseClient(nClient);
 
		if (pClient && m_bNoticeOnPublish == false)
		{
			m_bNoticeOnPublish = true;
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_on_publish;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_publish\",\"app\":\"%s\",\"stream\":\"%s\",\"sourceURL\":\"%s\",\"mediaServerId\":\"%s\",\"mediaSourceCount\":%d,\"networkType\":%d,\"key\":%llu,\"ip\":\"%s\" ,\"port\":%d,\"params\":\"%s\"}", app, stream,sourceURL,ABL_MediaServerPort.mediaServerID, mediaSourceCount, pClient->netBaseNetType, pClient->nClient, pClient->szClientIP, pClient->nClientPort, pClient->szPlayParams);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		}
 
		if (pClient != NULL)
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_arrive;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_arrive\",\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"mediaSourceCount\":%d,\"networkType\":%d,\"status\":%s,\"enable_hls\":%s,\"transcodingStatus\":%s,\"sourceURL\":\"%s\",\"networkType\":%d,\"readerCount\":%d,\"noneReaderDuration\":%d,\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"videoBitrate\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"audioBitrate\":%d,\"url\":{\"rtsp\":\"%s://%s:%d/%s/%s\",\"rtmp\":\"%s://%s:%d/%s/%s\",\"http-flv\":\"%s://%s:%d/%s/%s.flv\",\"ws-flv\":\"%s://%s:%d/%s/%s.flv\",\"http-mp4\":\"%s://%s:%d/%s/%s.mp4\",\"http-hls\":\"%s://%s:%d/%s/%s.m3u8\",\"webrtc\":\"%s://%s:%d/rtc/v1/whep/?app=%s&stream=%s\"}}", nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, ABL_MediaServerPort.mediaServerID, mediaSourceCount, netBaseNetType, enable_mp4 == true ? "true" : "false", enable_hls == true ? "true" : "false", H265ConvertH264_enable == true ? "true" : "false",sourceURL, pClient->netBaseNetType, mediaSendMap.size(), (int)0,
				m_mediaCodecInfo.szVideoName, m_mediaCodecInfo.nVideoFrameRate, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, m_mediaCodecInfo.nVideoBitrate, m_mediaCodecInfo.szAudioName, m_mediaCodecInfo.nChannels, m_mediaCodecInfo.nSampleRate, m_mediaCodecInfo.nAudioBitrate,
				ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nHlsPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream);
  
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			bNoticeClientArriveFlag = true;
		}
	}
  
	//������Ƶ����
	nAudioBitrate += nLength;

	//��¼��Ƶ��ʽ
	if (strlen(m_mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(m_mediaCodecInfo.szAudioName, szAudioCodec);
		m_mediaCodecInfo.nChannels = nChannels;
		m_mediaCodecInfo.nSampleRate = SampleRate;
	}

 	if (nG711ConvertAAC == 0 && strcmp(m_mediaCodecInfo.szAudioName, szAudioCodec) != 0)
	{//��ת��
		strcpy(m_mediaCodecInfo.szAudioName, szAudioCodec);
		m_mediaCodecInfo.nChannels = nChannels;
		m_mediaCodecInfo.nSampleRate = SampleRate;
	}
	else if (nG711ConvertAAC == 1)
	{//�޸�g711a��g711u Ϊ��AAC
		if (strcmp(m_mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(m_mediaCodecInfo.szAudioName, "G711_U") == 0)
			strcpy(m_mediaCodecInfo.szAudioName, "AAC");
	}

	if (m_mediaCodecInfo.nBaseAddAudioTimeStamp == 0)
	{//����aac��Ƶʱ��������� 
		if (m_mediaCodecInfo.nSampleRate == 48000)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 21;
		else if (m_mediaCodecInfo.nSampleRate == 44100)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 23;
		else if (m_mediaCodecInfo.nSampleRate == 32000)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 32;
		else if (m_mediaCodecInfo.nSampleRate == 24000)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 42;
		else if (m_mediaCodecInfo.nSampleRate == 22050)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 49;
		else if (m_mediaCodecInfo.nSampleRate == 16000)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 64;
		else if (m_mediaCodecInfo.nSampleRate == 12000)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 85;
		else if (m_mediaCodecInfo.nSampleRate == 11025)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 92;
		else if (m_mediaCodecInfo.nSampleRate == 8000)
			m_mediaCodecInfo.nBaseAddAudioTimeStamp = 128;
	}
	
 
	//תg711 Ϊ aac 
	if (nG711ConvertAAC == 1 && strcmp(szAudioCodec ,"G711_A") == 0)
	{
		if (ConvertG711ToAAC(FLV_AUDIO_G711A, szAudio, nLength, pOutAACData, nOutAACDataLength) == false)
			return  false;
		pCacheAudioFifo.push(pOutAACData, nOutAACDataLength);
	}
	else if (nG711ConvertAAC == 1 && strcmp(szAudioCodec, "G711_U") == 0)
	{
		if (ConvertG711ToAAC(FLV_AUDIO_G711U, szAudio, nLength, pOutAACData, nOutAACDataLength) == false)
			return false;
		pCacheAudioFifo.push(pOutAACData, nOutAACDataLength);
	}else
 		pCacheAudioFifo.push(szAudio, nLength);
 
	if (enable_hls == true && strcmp(m_mediaCodecInfo.szAudioName,"AAC") == 0 )
	{
		if (nAsyncAudioStamp == -1)
			nAsyncAudioStamp = GetTickCount();

		avtype = PSI_STREAM_AAC;

 		if (strcmp(m_mediaCodecInfo.szAudioName, "AAC") == 0 && tsPacketHandle != NULL )
		{
			if(strcmp(szAudioCodec,"AAC" )== 0)
			  mpeg_ts_write(tsPacketHandle, ts_stream(tsPacketHandle, avtype), 0, audioDts * 90, audioDts * 90, szAudio, nLength);
			else
			  mpeg_ts_write(tsPacketHandle, ts_stream(tsPacketHandle, avtype), 0, audioDts * 90, audioDts * 90, pOutAACData, nOutAACDataLength);
		}
		else if (strcmp(m_mediaCodecInfo.szAudioName, "AAC") == 0 && hlsFMP4 != NULL && track_265 >= 0 && ABL_MediaServerPort.nH265CutType == 2)
		{
			if (track_aac == -1)
			{		
				if (strcmp(szAudioCodec, "AAC") == 0)
				  nAACLength = mpeg4_aac_adts_frame_length(szAudio, nLength);
				else
				  nAACLength = mpeg4_aac_adts_frame_length(pOutAACData, nOutAACDataLength);

				if (nAACLength < 0)
				  return false ;

				if (strcmp(szAudioCodec, "AAC") == 0)
					mpeg4_aac_adts_load(szAudio, nLength, &aacHandle);
				else
					mpeg4_aac_adts_load(pOutAACData, nOutAACDataLength, &aacHandle);

				  nExtenAudioDataLength = mpeg4_aac_audio_specific_config_save(&aacHandle, szExtenAudioData, sizeof(szExtenAudioData));
				  if (nExtenAudioDataLength > 0)
				  {
					  track_aac = hls_fmp4_add_audio(hlsFMP4, MOV_OBJECT_AAC, nChannels,16, SampleRate, szExtenAudioData, nExtenAudioDataLength);
				  }
 			}

			//����hls_init_segment ��ʼ����ɲ���д��Ƶ�Σ��ڻص�������������־ 
			if (track_aac >= 0 && hls_init_segmentFlag)
			{
  				if (strcmp(szAudioCodec, "AAC") == 0)
				{
					if (nMediaSourceType == MediaSourceType_LiveMedia)
					   hls_fmp4_input(hlsFMP4, track_aac, szAudio + 7, nLength - 7, audioDts, audioDts, 0);
					else //¼���������Ƶ���ݣ�ǰ����4���ֽڵ�֡���
					   hls_fmp4_input(hlsFMP4, track_aac, szAudio + 7 + 4 , nLength - 7 - 4, audioDts, audioDts, 0);
				}
				else
					hls_fmp4_input(hlsFMP4, track_aac, pOutAACData + 7, nOutAACDataLength - 7, audioDts, audioDts, 0);
 			}
 		}

 		audioDts += m_mediaCodecInfo.nBaseAddAudioTimeStamp;

		//500����ͬ��һ�� 
		if (GetTickCount() - nAsyncAudioStamp >= 500)
		{
			if (videoDts / 1000 > audioDts / 1000)
			{
				nVideoStampAdd = (1000 / m_mediaCodecInfo.nVideoFrameRate) - 10 ;
			}
			else if (videoDts / 1000 < audioDts / 1000 )
			{
				nVideoStampAdd = (1000 / m_mediaCodecInfo.nVideoFrameRate) + 10 ;
			}else
				nVideoStampAdd = 1000 / m_mediaCodecInfo.nVideoFrameRate;

			nAsyncAudioStamp = GetTickCount();

			//WriteLog(Log_Debug, "CMediaStreamSource = %X videoDts = %d ,audioDts = %d ", this, videoDts / 1000 , audioDts / 1000);
		} 
	}

	if (mediaSendMap.size() <= 0)
	{
		//����Ƶ�������������������ҲҪ�������˹ۿ��¼�֪ͨ 
		if (ABL_MediaServerPort.hook_enable == 1 && strlen(m_mediaCodecInfo.szVideoName) == 0  && GetCurrentSecond() - nLastWatchTime >= ABL_MediaServerPort.noneReaderDuration)
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_None_reader;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_none_reader\",\"app\":\"%s\",\"stream\":\"%s\",\"sourceURL\":\"%s\",\"noneReaderDuration\":%d,\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu}", app, stream,sourceURL, GetCurrentSecond() - nLastWatchTimeDisconect, ABL_MediaServerPort.mediaServerID, netBaseNetType, nClient);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));

			nLastWatchTime = GetCurrentSecond();
		}
		return false;
	}

	
	uint64_t   nClient;
	for (auto it = mediaSendMap.begin(); it != mediaSendMap.end();)
	{
		auto pClient = (*it).second;
		if (pClient != NULL)
		{
			//��Ƶ�ȵ���
			if (strlen(pClient->mediaCodecInfo.szAudioName) == 0 && strlen(m_mediaCodecInfo.szAudioName) > 0 )
  				memcpy(&pClient->mediaCodecInfo,(char*)&m_mediaCodecInfo,sizeof(MediaCodecInfo));
 
			if (pClient->nMediaSourceType != nMediaSourceType)
				pClient->nMediaSourceType = nMediaSourceType;//����ý��Դ����

 			if (m_bPauseFlag)
			{//��ͣ���� 
				if(!pClient->m_bPauseFlag)
				   pClient->m_bPauseFlag = true;
  			    
				it++;
				continue;
			}
			else
			{//�ָ�����
				if (pClient->m_bPauseFlag)
					pClient->m_bPauseFlag = false ;
			}

			if (!pClient->m_bSendCacheAudioFlag && strlen(m_mediaCodecInfo.szVideoName) > 0 )
			{
				//������Ƶ
				CopyAudioFrameBufer();

				unsigned char* pData;
				int            nPopLength = 0;
				while ((pData = pCopyCacheAudioFifo.pop(&nPopLength)) != NULL)
				{
					if (nPopLength > 0)
						pClient->PushAudio(pData, nPopLength, m_mediaCodecInfo.szAudioName, nChannels, SampleRate);
					pClient->SendAudio();
					pCopyCacheAudioFifo.pop_front();
				}
				pClient->m_bSendCacheAudioFlag = true;

				it++; 
				continue;
			}

			if (strcmp(szAudioCodec, "AAC") == 0 || strcmp(szAudioCodec, "MP3") == 0)
			{
				pClient->PushAudio(szAudio, nLength, m_mediaCodecInfo.szAudioName, nChannels, SampleRate);
				pClient->SendAudio();
			}else
			{//G711A ��G711U 
				if (nG711ConvertAAC == 1)
				{
					if(strcmp(szAudioCodec, "G711_A") == 0 || strcmp(szAudioCodec, "G711_U") == 0)
					   pClient->PushAudio(pOutAACData, nOutAACDataLength, m_mediaCodecInfo.szAudioName, nChannels, SampleRate);
					pClient->SendAudio();
				}
				else
				{
 					if (((strcmp(szAudioCodec, "G711_A") == 0 || strcmp(szAudioCodec, "G711_U") == 0) && nLength == 320)  ||
   					   ((strcmp(szAudioCodec, "G711_A") == 0 || strcmp(szAudioCodec, "G711_U") == 0) && nMediaSourceType == MediaSourceType_ReplayMedia) )
					{
				         pClient->PushAudio(szAudio, nLength, m_mediaCodecInfo.szAudioName, nChannels, SampleRate);
						 pClient->SendAudio();
					}
					else
					{//nLength ���� 320 ����Ҫƴ��Ϊ320���ȣ���Ϊrtp���ʱ�̶�Ϊ320�ֽڵ�ʱ���
						memcpy(g711CacheBuffer + nG711CacheLength, szAudio, nLength);
						nG711CacheLength += nLength;
					    nG711CacheProcessLength = nG711CacheLength ;//�и�ǰ�ܳ��� 
						nG711SplittePos = 0; //�и��ƶ�λ�� 
 						while (nG711CacheLength >= 320)
						{
							pClient->PushAudio(g711CacheBuffer + nG711SplittePos, 320, m_mediaCodecInfo.szAudioName, nChannels, SampleRate);
							pClient->SendAudio();
							nG711SplittePos  += 320;
							nG711CacheLength -= 320;
 						}

						if (nG711SplittePos > 0 && (nG711CacheProcessLength - nG711SplittePos) > 0)
						{//�Ѷ����g711a\g711u ��ǰ�ƶ� 
							memmove(g711CacheBuffer, g711CacheBuffer + nG711SplittePos, nG711CacheProcessLength - nG711SplittePos);
							nG711CacheLength = nG711CacheProcessLength - nG711SplittePos ;
						}
 					}
				}
 			}
			it++;
		}
		else
		{//����ʧ�ܣ�֤���������Ѿ��Ͽ� 

			mediaSendMap.erase(it++);
  		}
	}
	return false;
}

bool CMediaStreamSource::AddClientToMap(NETHANDLE nClient)
{
	std::lock_guard<std::mutex> lock(mediaSendMapLock);

    if(!bEnableFlag)
	{
		WriteLog(Log_Debug, "׼��ɾ�� %s �������ӷ��Ͷ��� %llu ",m_szURL, nClient);
		return false;
	}


	auto it = mediaSendMap.find(nClient);
	if (it != mediaSendMap.end())
	{
		WriteLog(Log_Debug, "�ͻ��� %llu �Ѿ�����ý����Դ %s �����߳��� ", nClient,m_szURL);
		return false;
	}

	nLastWatchTime = nLastWatchTimeDisconect = GetCurrentSecond();

	auto getClient = GetNetRevcBaseClient(nClient);
	if (getClient)
	{
		mediaSendMap.insert(std::make_pair(nClient, getClient));
	    WriteLog(Log_Debug, "��һ���ͻ��� nClient = %llu ���뵽ý����Դ %s �����߳��� ", nClient, m_szURL);
		return true;
	}
	else
	{
		WriteLog(Log_Debug, "�Ҳ����ͻ��� nClient = %llu ���뵽ý����Դ %s ʧ�� ", nClient, m_szURL);
		return false;
	}
 }

bool CMediaStreamSource::DeleteClientFromMap(NETHANDLE nClient)
{
 	std::lock_guard<std::mutex> lock(mediaSendMapLock);

	bool       bRet = false;

	auto it = mediaSendMap.find(nClient);
	if (it != mediaSendMap.end())
	{
		if((*it).second->netBaseNetType == NetBaseNetType_WebRtcServerWhepPlayer)
		{//webrtc��http���ӻ��Լ��Ͽ�������ֱ��ɾ�� 
			//if((*it).second->nWebRTC_Comm_State == WebRTC_Comm_State_Delete)
			//{//webrtc ����ֹͣʱ�Ŵӷ����̳߳��Ƴ�
 		//       mediaSendMap.erase(it);
		 //      bRet = true;
 		//       WriteLog(Log_Debug, "��һ���ͻ��� %llu ��ý����Դ�����߳��Ƴ� ", nClient);
			//}
		}else 
		{
 		   mediaSendMap.erase(it);
		   bRet = true;
 		   WriteLog(Log_Debug, "��һ���ͻ��� %llu ��ý����Դ�����߳��Ƴ� ", nClient);
		}
	}else
	   bRet = false  ;

	return bRet;
}

//����URL /Media/Camera_00001 ������·��  
void  CMediaStreamSource::CreateSubPathByURL(char* szMediaURL)
{
	memset(szHLSPath, 0x00, sizeof(szHLSPath));

	strcpy(szHLSPath, ABL_wwwMediaPath);

	string strMediaURL = szMediaURL;
	int               nPos = 1, nFind;
	char              szTemp[256] = { 0 };

	while (true)
	{
		memset(szTemp, 0x00, sizeof(szTemp));
		nFind = strMediaURL.find("/", nPos + 1);
		if (nFind > 0 && nFind != string::npos)
		{
			memcpy(szTemp, szMediaURL + nPos, nFind - nPos);
			nPos = nFind ;

#ifdef OS_System_Windows
			strcat(szHLSPath, "\\");
			strcat(szHLSPath, szTemp);
			::CreateDirectory(szHLSPath, NULL);
#else 
			strcat(szHLSPath, "/");
			strcat(szHLSPath, szTemp);
			umask(0);
			mkdir(szHLSPath, 777);
		    ABL_SetPathAuthority(szHLSPath);
#endif

			WriteLog(Log_Debug, "����HLS��·�� %s ", szHLSPath);
		}
		else
		{
			memcpy(szTemp, szMediaURL + nPos+1, strlen(szMediaURL) - nPos);
#ifdef OS_System_Windows
			strcat(szHLSPath, "\\");
			strcat(szHLSPath, szTemp);
			::CreateDirectory(szHLSPath, NULL);
#else 
			strcat(szHLSPath, "/");
			strcat(szHLSPath, szTemp);
			umask(0);
			mkdir(szHLSPath, 777);
			ABL_SetPathAuthority(szHLSPath);
#endif
			WriteLog(Log_Debug, "����HLS��·�� %s ", szHLSPath);
			break;
		}
	}
#ifdef OS_System_Windows
	strcat(szHLSPath, "\\");
#else 
	strcat(szHLSPath, "/");
#endif
}

//ɾ��һ��Ŀ¼���������ļ�
void CMediaStreamSource::ABLDeletePath(char* szDeletePath, char* srcPath)
{
	char               szDeleteFile[string_length_512];
#ifdef OS_System_Windows
	HANDLE  hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA    pNextInfo;
	bool               bFind = true;

	hFile = FindFirstFile(szDeletePath, &pNextInfo);
	while (bFind)
	{
		if (pNextInfo.cFileName[0] != '.')
		{
			sprintf(szDeleteFile, "%s%s", srcPath, pNextInfo.cFileName);
			ABLDeleteFile(szDeleteFile);
			WriteLog(Log_Debug, "ɾ���ļ��� %s ", szDeleteFile);
 		}

		bFind = FindNextFile(hFile, &pNextInfo);
	}
	if(hFile != INVALID_HANDLE_VALUE)
	  FindClose(hFile);
#else
	struct dirent * filename;    // return value for readdir()
	DIR * dir;                   // return value for opendir()
	dir = opendir(srcPath);
	if (NULL == dir)
 		return ;
 
	/* read all the files in the dir  */
	while ((filename = readdir(dir)) != NULL)
	{
		if (strcmp(filename->d_name, ".") == 0 ||
			strcmp(filename->d_name, "..") == 0)
			continue;

		sprintf(szDeleteFile, "%s/%s", srcPath, filename->d_name);
		ABLDeleteFile(szDeleteFile);
		WriteLog(Log_Debug, "ɾ���ļ��� %s ", szDeleteFile);
	}
	closedir(dir);
#endif
}

//����m3u8
void CMediaStreamSource::CopyM3u8Buffer(char* szM3u8Buffer)
{
	std::lock_guard<std::mutex> lock(mediaTsFileLock);
	memset(szDataM3U8, 0x00, sizeof(szDataM3U8));
	strcpy(szDataM3U8, szM3u8Buffer);
}

//�ⲿ��ȡm3u8����
bool   CMediaStreamSource::ReturnM3u8Buffer(char* szOutM3u8)
{
	std::lock_guard<std::mutex> lock(mediaTsFileLock);
	if (strlen(szDataM3U8) == 0)
		return false;
	strcpy(szOutM3u8, szDataM3U8);
	return true;
}

//����TS�ļ���Ż�ȡ���ļ����ֽڴ�С���������Ͳ�����Ҫ��Ӳ���л�ȡTS�ļ���С 
int CMediaStreamSource::GetTsFileSizeByOrder(int64_t nTsFileNameOrder)
{
	if (nTsFileNameOrder < 0)
		return 0;
	if (nTsFileNameOrder == 0)
	{
		if (nFmp4SPSPPSLength > 0 && ABL_MediaServerPort.nH265CutType == 2)
			return nFmp4SPSPPSLength;//FMP4��Ƭ ,sps pps �ĳ��� 
		else
			return 0;
	}
	else
	   return nTsFileSizeArray[nTsFileNameOrder % MaxStoreTsFileCount];
}

bool CMediaStreamSource::CopyTsFileBuffer(int64_t nTsFileNameOrder, unsigned char* pOutTsBuffer)
{
	std::lock_guard<std::mutex> lock(mediaTsFileLock);

	int nOrder = nTsFileNameOrder % MaxStoreTsFileCount;
	int nLength;
	unsigned char* pData;
	pData = mediaFileBuffer[nOrder].pop(&nLength);
	if (nLength > 0 && pData != NULL)
	{
		memcpy(pOutTsBuffer, pData, nLength);
		return true;
	}
	else
		return false;
}

/*
�����Ƶ�Ƿ���I֡
*/
bool  CMediaStreamSource::CheckVideoIsIFrame(char* szVideoCodecName, unsigned char* szPVideoData, int nPVideoLength)
{
	int nPos = 0;
	bool bVideoIsIFrameFlag = false;
	unsigned char  nFrameType = 0x00;
	int nPosSPS = -1, nPosIDR = -1;
	int nNaluType = 1;
	int nAddStep = 3;

	for (int i = 0; i < nPVideoLength; i++)
	{
		nNaluType = -1;
		if (memcmp(szPVideoData + i, (unsigned char*)NALU_START_CODE, sizeof(NALU_START_CODE)) == 0)
		{//�г��� 00 00 01 ��naluͷ��־
			nNaluType = 1;
			nAddStep = sizeof(NALU_START_CODE);
		}
		else if (memcmp(szPVideoData + i, (unsigned char*)SLICE_START_CODE, sizeof(SLICE_START_CODE)) == 0)
		{//�г��� 00 00 00 01 ��naluͷ��־
			nNaluType = 2; 
			nAddStep = sizeof(SLICE_START_CODE);
		}

		if(nNaluType >= 1)
		{//�ҵ�֡Ƭ��
			if (strcmp(szVideoCodecName, "H264") == 0)
			{
				nFrameType = (szPVideoData[i+ nAddStep] & 0x1F);
				if (nFrameType == 7 || nFrameType == 8 || nFrameType == 5)
				{//SPS   PPS   IDR 
					if (nSPSPPSLength == 0)
					{
						if(nFrameType == 7 && nPosSPS == -1)
						   nPosSPS = i;

						if (nFrameType == 5 && nPosIDR == -1)
							nPosIDR = i;

						if (nPosSPS >= 0 && nPosIDR >= 0 && nPosIDR < 4096 )
						{
							memcpy(pSPSPPSBuffer, szPVideoData, nPosIDR);
							nSPSPPSLength = nPosIDR;
							bVideoIsIFrameFlag = true;
							break;
					     }
					}
					else
					{
 						bVideoIsIFrameFlag = true;
						break;
  					}
 				}
 			}
			else if (strcmp(szVideoCodecName, "H265") == 0)
			{
				nFrameType = (szPVideoData[i+ nAddStep] & 0x7E) >> 1;
				//WriteLog(Log_Debug, " nFrameType = %d ", nFrameType);
				if ((nFrameType >= 16 && nFrameType <= 23) || (nFrameType >= 32 && nFrameType <= 34))
				{//SPS   PPS   IDR 
					if (nSPSPPSLength == 0)
					{
						if (nFrameType >= 32 && nFrameType <= 34 && nPosSPS == -1)
							nPosSPS = i;

						if ((nFrameType >= 16 && nFrameType <= 23) && nPosIDR == -1)
							nPosIDR = i;

						if (nPosSPS >= 0 && nPosIDR >= 0 && nPosIDR < 4096)
						{
							memcpy(pSPSPPSBuffer, szPVideoData, nPosIDR);
							nSPSPPSLength = nPosIDR;
							bVideoIsIFrameFlag = true;

							break;
						}
					}
					else
					{
						if (nFrameType >= 16 && nFrameType <= 23)
						{
							bVideoIsIFrameFlag = true;
							break;
						}
 					}
				}
			}

			i += nAddStep; 
		}
	
		//����Ҫȫ�������ϣ��Ϳ����ж�һ֡����
		if (i >= 512)
			return false;
	}

	if (nSPSPPSLength == 0)
	{
		if (nPosSPS >= 0 && nPosIDR < 0 && nPVideoLength <= 4096)
		{
			memcpy(pSPSPPSBuffer, szPVideoData, nPVideoLength);
			nSPSPPSLength = nPVideoLength;
			return true;
		}
	}

	return bVideoIsIFrameFlag; 
}

bool  CMediaStreamSource::GetRtspSDPContent(RtspSDPContentStruct* sdpContent)
{
	if (strlen(rtspSDPContent.szSDPContent) > 0)
	{
		memcpy((char*)sdpContent, (char*)&rtspSDPContent, sizeof(RtspSDPContentStruct));
		return true;
	}
	else
		return false;
}

//����SPS���ֵ�λ��
unsigned  int  CMediaStreamSource::FindSpsPosition(char* szVideoCodeName, unsigned char* szVideoBuffer, int nBufferLength, bool &bFind)
{
	unsigned int nPos = 0;
	unsigned char H265HeadFlag[4] = { 0x00,0x00,0x00,0x01 };
	unsigned char nFrameTypeH265, nTempFrame;
	bFind = false;
	for (int i = 0; i < nBufferLength; i++)
	{
		if (memcmp(H265HeadFlag, szVideoBuffer + i, 4) == 0)
		{
			if (strcmp(szVideoCodeName, "H264") == 0)
			{
				nTempFrame = (szVideoBuffer[i + 4] & 0x1F);
				if (nTempFrame == 7 )
				{
					nPos = i;
					bFind = true;
					break;
				}
			}
			else if (strcmp(szVideoCodeName, "H265") == 0)
			{
				nTempFrame = szVideoBuffer[i + 4];
				nFrameTypeH265 = (nTempFrame & 0x7E) >> 1;
				if (nFrameTypeH265 >= 32 && nFrameTypeH265 <= 34)
				{//SPS֡
					nPos = i;
					bFind = true;
					break;
				}
			}
		}
	}

	return nPos + 4;
}

bool CMediaStreamSource::FFMPEGGetWidthHeight(unsigned char * videooutdata, int videooutdatasize, char* videoName, int * outwidth, int * outheight)
{
	bool ret = false;
	AVCodecParserContext *parservideo = NULL;
	AVCodecContext *cvideo = NULL;
	AVCodec *codecvideo = NULL;
	AVPacket * outpkt = NULL;

	if (videooutdatasize <= 0 || videooutdata == NULL || !(strcmp(videoName, "H264") == 0 || strcmp(videoName, "H265") == 0))
	{
		return false;
	}

	outpkt = av_packet_alloc();
	if (!outpkt)
	{
		return false;
	}

	if (strcmp(videoName, "H264") == 0)
	{
		codecvideo = (AVCodec*)avcodec_find_decoder(AV_CODEC_ID_H264);
	}
	else if (strcmp(videoName, "H265") == 0)
	{
		codecvideo = (AVCodec*)avcodec_find_decoder(AV_CODEC_ID_H265);
	}

	if (!codecvideo)
	{
		av_packet_free(&outpkt);
		return false;
	}

	parservideo = av_parser_init(codecvideo->id);
	if (!parservideo)
	{
		av_packet_free(&outpkt);
		return false;
	}

	cvideo = avcodec_alloc_context3(codecvideo);
	if (!cvideo)
	{
		av_parser_close(parservideo);
		av_packet_free(&outpkt);
		return false;
	}

	if (avcodec_open2(cvideo, codecvideo, NULL) < 0)
	{
		avcodec_free_context(&cvideo);
		av_parser_close(parservideo);
		av_packet_free(&outpkt);
		return false;
	}

	ret = av_parser_parse2(parservideo, cvideo, &outpkt->data, &outpkt->size,
		(uint8_t *)videooutdata, videooutdatasize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
	if (ret > 0)
	{
		ret = av_parser_parse2(parservideo, cvideo, &outpkt->data, &outpkt->size,
			NULL, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
	}

	if (outpkt->size >0)
	{
		if (parservideo->width > 0 && parservideo->height > 0)
		{
			*outwidth = parservideo->width;
			*outheight = parservideo->height;
			ret = true;
		}
		else
		{
			*outwidth = parservideo->width;
			*outheight = parservideo->height;
			ret = false;
		}
	}
	av_parser_close(parservideo);
	avcodec_free_context(&cvideo);
	av_packet_free(&outpkt);

	return ret;
}

bool  CMediaStreamSource::H265FrameToFMP4File(unsigned char* szVideoData, int nLength)
{
	if (track_265 < 0 )
	{
		if (!(m_mediaCodecInfo.nWidth > 0 && m_mediaCodecInfo.nHeight > 0))
			return false;

		if(pH265Buffer == NULL)
		    pH265Buffer = new unsigned char[MediaStreamSource_VideoFifoLength];
		int n = h265_annexbtomp4(&hevc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);

		if (track_265 < 0)
		{
			if (hevc.numOfArrays < 1)
			{
 				return false; // waiting for vps/sps/pps
			}

			memset(szExtenVideoData,0x00, sizeof(szExtenVideoData));
			extra_data_sizeH265 = mpeg4_hevc_decoder_configuration_record_save(&hevc, szExtenVideoData, sizeof(szExtenVideoData));
			if (extra_data_sizeH265 <= 0)
			{
 				return false;
			}

			if (extra_data_sizeH265 > 0)
			{
  			   track_265 = hls_fmp4_add_video(hlsFMP4, MOV_OBJECT_HEVC, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, szExtenVideoData, extra_data_sizeH265);
 			}
		}
  	}

	if (track_265 >= 0)
	{
		if (CheckVideoIsIFrame(m_mediaCodecInfo.szVideoName,szVideoData, nLength) == true)
			flags = 1;
		else
			flags = 0;

		vcl = 0;
		update = 0;
		nMp4BufferLength =  h265_annexbtomp4(&hevc, szVideoData, nLength, pH265Buffer, MediaStreamSource_VideoFifoLength, &vcl, &update);

		//����Ƶ��� ������ �ȴ���Ƶ����30֡ʱ����û������Ƶ���֤��������û����Ƶ 
	    if (nMp4BufferLength > 0 && (ABL_MediaServerPort.nEnableAudio == 0 || track_aac >= 0 || (videoDts / 40 > 30 )))
		{
			if (hls_init_segmentFlag == false )
			{
			    hls_init_segment(hlsFMP4, this);
 			}

			//����hls_init_segment ��ʼ����ɲ���д��Ƶ�Σ��ڻص�������������־ 
			if(hls_init_segmentFlag == true )
	           hls_fmp4_input(hlsFMP4, track_265, pH265Buffer, nMp4BufferLength, videoDts, videoDts, (flags == 1) ? MOV_AV_FLAG_KEYFREAME : 0);
 		}

	}
 
	return true;
}

//������Ƶ֡�ٶ�
void   CMediaStreamSource::UpdateVideoFrameSpeed(int nVideoSpeed,int netType)
{
 	netBaseNetType = netType ;

	//�쳣����Ƶ֡�ٶ�
	if (nVideoSpeed <= 0)
		return;

	m_mediaCodecInfo.nVideoFrameRate = nVideoSpeed;

	bUpdateVideoSpeed = true;
}

bool  CMediaStreamSource::GetVideoWidthHeight(char* szVideoCodeName, unsigned char* pVideoData, int nDataLength)
{
	//�����ﵽ֪ͨ
	if (ABL_MediaServerPort.hook_enable == 1 && m_bNoticeOnPublish  && bNoticeClientArriveFlag == false)
	{  
		auto pClient = GetNetRevcBaseClient(nClient);
		if (pClient != NULL)
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_arrive;
 			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_arrive\",\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"mediaSourceCount\":%d,\"networkType\":%d,\"status\":%s,\"enable_hls\":%s,\"transcodingStatus\":%s,\"sourceURL\":\"%s\",\"networkType\":%d,\"readerCount\":%d,\"noneReaderDuration\":%d,\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"videoBitrate\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"audioBitrate\":%d,\"url\":{\"rtsp\":\"%s://%s:%d/%s/%s\",\"rtmp\":\"%s://%s:%d/%s/%s\",\"http-flv\":\"%s://%s:%d/%s/%s.flv\",\"ws-flv\":\"%s://%s:%d/%s/%s.flv\",\"http-mp4\":\"%s://%s:%d/%s/%s.mp4\",\"http-hls\":\"%s://%s:%d/%s/%s.m3u8\",\"webrtc\":\"%s://%s:%d/rtc/v1/whep/?app=%s&stream=%s\"}}",nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, ABL_MediaServerPort.mediaServerID, mediaSourceCount, netBaseNetType, enable_mp4 == true ? "true" : "false", enable_hls == true ? "true" : "false", H265ConvertH264_enable == true ? "true" : "false", sourceURL, pClient->netBaseNetType, mediaSendMap.size(), (int)0,
				m_mediaCodecInfo.szVideoName, m_mediaCodecInfo.nVideoFrameRate, m_mediaCodecInfo.nWidth, m_mediaCodecInfo.nHeight, m_mediaCodecInfo.nVideoBitrate, m_mediaCodecInfo.szAudioName, m_mediaCodecInfo.nChannels, m_mediaCodecInfo.nSampleRate, m_mediaCodecInfo.nAudioBitrate,
				ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nHlsPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream,
				ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream);

			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			bNoticeClientArriveFlag = true;
		}
 	}

	if (!(m_mediaCodecInfo.nWidth == 0 && m_mediaCodecInfo.nHeight == 0) || pVideoData == NULL || nDataLength <= 0 || !(strcmp(szVideoCodeName, "H264") == 0 || strcmp(szVideoCodeName, "H265") == 0) )
		return false;

	//��ֹ��Щ��Ƶ�����������ߣ�һֱ�ڷ��ļ�����Ƶ���ߣ�30���ڣ����������������Ͳ��ٷ����������� 
	if ((GetTickCount64() - tsCreateTime) > 1000 * 45)
		return false;

	int  nWidth = 0, nHeight = 0;
	bool bFind = false;
	int  nPos = -1;
	nPos = FindSpsPosition(szVideoCodeName,pVideoData, nDataLength, bFind);
	if (!bFind)
		return false;
 
	//ffmepg ��ȡ��� 
 	FFMPEGGetWidthHeight(pVideoData, nDataLength , szVideoCodeName, &nWidth,&nHeight);

	if (nWidth <= 0 || nHeight <= 0)
		return false;

	m_mediaCodecInfo.nWidth = nSrcWidth = nWidth;
	m_mediaCodecInfo.nHeight = nSrcHeight = nHeight;
 
	WriteLog(Log_Debug, "������ý��Դ /%s/%s �� = %d ,�� = %d ", app,stream,m_mediaCodecInfo.nWidth,m_mediaCodecInfo.nHeight);

	return true;
}

//ת��G711A G711U ΪAAC
bool   CMediaStreamSource::ConvertG711ToAAC(int nCodec, unsigned char* pG711, int nBytes,unsigned char* szOutAAC,int& nAACLength)
{
	if ( pG711 == NULL || nBytes <= 0)
		return false;

	if (aacEnc.hEncoder == NULL)
	{//��ʼ��AAC�����
		aacEnc.InitAACEncodec(64000, 8000, 1, &nAACEncodeLength);
		strcpy(m_mediaCodecInfo.szAudioName, "AAC");
		m_mediaCodecInfo.nChannels = 1;
		m_mediaCodecInfo.nSampleRate = 8000;
	}

	bool bRet = false;
	if (nBytes < 320)
	{//��Ҫƴ��
	   memcpy(g711CacheBuffer + nG711CacheLength, pG711, nBytes);
	   nG711CacheLength += nBytes;
 	   if (nG711CacheLength < 320)
		 return false ;
 	}

	if (nCodec == FLV_AUDIO_G711A)
	{
		if(nBytes < 320)
		  alaw_to_pcm16(320, (const char*)g711CacheBuffer, g711toPCM);
		else
		  alaw_to_pcm16(320, (const char*)pG711, g711toPCM);
	}
	else if (nCodec == FLV_AUDIO_G711U)
	{
		if (nBytes < 320)
		  ulaw_to_pcm16(320, (const char*)g711CacheBuffer, g711toPCM);
		else
		  ulaw_to_pcm16(320, (const char*)pG711, g711toPCM);
	}
	else
		return false;

	//��Ҫƴ��
	if (nBytes < 320)
 	   nG711CacheLength -= 320;
 
	if (1024 * 16 - nG711ToPCMCacheLength > 640)
	{
		memcpy(g711ToPCMCache + nG711ToPCMCacheLength, g711toPCM, 640);
		nG711ToPCMCacheLength += 640;
	}

	if (nG711ToPCMCacheLength >= nAACEncodeLength && aacEnc.pbPCMBuffer != NULL )
	{
		memcpy(aacEnc.pbPCMBuffer, g711ToPCMCache, nAACEncodeLength);
		aacEnc.EncodecAAC(&nRetunEncodeLength);

		if (nRetunEncodeLength > 0)
		{
			memcpy(szOutAAC,(unsigned char*)aacEnc.pbAACBuffer, nRetunEncodeLength);
			nAACLength = nRetunEncodeLength;
			bRet = true;
		}

		memmove(g711ToPCMCache, g711ToPCMCache + nAACEncodeLength, nG711ToPCMCacheLength - nAACEncodeLength);
		nG711ToPCMCacheLength = nG711ToPCMCacheLength - nAACEncodeLength;

		return bRet;
	}
	else
		return false; //����AAC��Ƶ����
}

//�޸�ˮӡ�ַ�
bool  CMediaStreamSource::ChangeVideoFilter(char *filterText, int fontSize, char *fontColor, float fontAlpha, int fontLeft, int fontTop)
{
	std::lock_guard<std::mutex> lock(mediaSendMapLock);
	
	if (pFFVideoFilter == NULL) 
      return false ;

    SAFE_DELETE(pFFVideoFilter);
    pFFVideoFilter = new CFFVideoFilter();
	if(pFFVideoFilter)
	{
	  pFFVideoFilter->waterMarkText = filterText ;
	  pFFVideoFilter->StartFilter(AV_PIX_FMT_YUV420P, m_h265ConvertH264Struct.convertOutWidth, m_h265ConvertH264Struct.convertOutHeight, 25, fontSize, fontColor, fontAlpha, fontLeft,fontTop);
	  return true ;
	}
	else
		return false ;
}

//������ͣ������ 
bool CMediaStreamSource::SetPause(bool bFlag)
{
	m_bPauseFlag = bFlag;
	WriteLog(Log_Debug, "CMediaStreamSource = %X, app = %s ,stream = %s SetPause() m_bPauseFlag = %d ", this, app,stream , m_bPauseFlag);
	return true;
}

//����һ��gop��Ƶ֡ 
bool CMediaStreamSource::CopyVideoGopFrameBufer()
{
	if (pVideoGopFrameBuffer.GetSize() <= 0)
		return false;

	pCopyVideoGopFrameBuffer.nFifoStart = pVideoGopFrameBuffer.nFifoStart;
	pCopyVideoGopFrameBuffer.nFifoEnd = pVideoGopFrameBuffer.nFifoEnd;
	pCopyVideoGopFrameBuffer.nMediaBufferLength = pVideoGopFrameBuffer.nMediaBufferLength;

	memcpy(pCopyVideoGopFrameBuffer.pMediaBuffer, pVideoGopFrameBuffer.pMediaBuffer, pVideoGopFrameBuffer.nMediaBufferLength);

	MediaFifoLengthList::iterator it;
	for (it = pVideoGopFrameBuffer.LengthList.begin();it != pVideoGopFrameBuffer.LengthList.end();++it)
		pCopyVideoGopFrameBuffer.LengthList.push_back(*it);
 
	return true;
}

//������Ƶ����
bool CMediaStreamSource::CopyAudioFrameBufer()
{
	if (pCacheAudioFifo.GetSize() <= 0)
		return false;

	pCopyCacheAudioFifo.nFifoStart = pCacheAudioFifo.nFifoStart;
	pCopyCacheAudioFifo.nFifoEnd = pCacheAudioFifo.nFifoEnd;
	pCopyCacheAudioFifo.nMediaBufferLength = pCacheAudioFifo.nMediaBufferLength;

	memcpy(pCopyCacheAudioFifo.pMediaBuffer, pCacheAudioFifo.pMediaBuffer, pCacheAudioFifo.nMediaBufferLength);

	MediaFifoLengthList::iterator it;
	for (it = pCacheAudioFifo.LengthList.begin(); it != pCacheAudioFifo.LengthList.end(); ++it)
		pCopyCacheAudioFifo.LengthList.push_back(*it);

	return true;
}

//����g711a g711u �Ƿ�ת�� 
void   CMediaStreamSource::SetG711ConvertAAC(int nFlag)
{
	nG711ConvertAAC = nFlag ;
	WriteLog(Log_Debug, "CMediaStreamSource = %X, app = %s ,stream = %s , SetG711ConvertAAC() ������Ƶ�Ƿ�ת����� nG711ConvertAAC = %d ", this, app, stream, nG711ConvertAAC);
}
