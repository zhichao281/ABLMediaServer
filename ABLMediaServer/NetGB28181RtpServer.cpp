/*
���ܣ�
    �������GB28181Rtp����������UDP��TCPģʽ 
    ���� ���귢��  ����������յ�ͬʱҲ֧�ֹ��귢�ͣ�    2023-05-18
	����֧��1078������ 2016\2019�汾����                 2023-12-20
����    2021-08-08
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181RtpServer.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern CMediaFifo                            pMessageNoticeFifo;          //��Ϣ֪ͨFIFO

#endif
extern CMediaFifo                            pDisconnectMediaSource;      //�������ý��Դ 
static void* NetGB28181RtpServer_ps_alloc(void* param, size_t bytes)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)param;

	return pThis->s_buffer;
}

static void NetGB28181RtpServer_ps_free(void* param, void* /*packet*/)
{
	return;
}

//PS ������{
static int NetGB28181RtpServer_ps_write(void* param, int stream, void* packet, size_t bytes)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)param;
	if (!pThis->bRunFlag.load())
		return -1;

	 pThis->GB28181PsToRtPacket((unsigned char*)packet, bytes);

#ifdef WriteSendPsFileFlag
	if (pThis->fWriteSendPsFile)
	{
		fwrite((char*)packet,1,bytes,pThis->fWriteSendPsFile);
		fflush(pThis->fWriteSendPsFile);
	}
#endif 

	return 0 ;
}

void RTP_DEPACKET_CALL_METHOD GB28181_rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)cb->userdata;
	if (!pThis->bRunFlag.load())
		return;

	if(pThis != NULL)
	{
		if (pThis->pMediaSource == NULL)
		{//���ȴ���ý��Դ,��Ϊ rtp + PS \ rtp + ES \ rtp + XHB ��Ҫʹ�� 

		   //�����10000���˿ڽ��룬���ܴ�����ͬ��ssrc ,����ý��Դ�ͻ��ظ�������ɾ�������µ����Ӷ�����Ϊ���˿ڽ����ǳ����ģ�û�а취Ԥ�ȼ��ssrc 
			auto tmpSource = GetMediaStreamSource(pThis->m_szShareMediaURL);
			if (tmpSource != NULL && atoi(pThis->m_openRtpServerStruct.RtpPayloadDataType) != 2)
			{
				WriteLog(Log_Debug, "�½�������Ҫ������ý��Դ %s �Ѿ����ڣ���ֹ��������ɾ�������½���������", pThis->m_szShareMediaURL);
				DeleteNetRevcBaseClient(pThis->hParent);
				return;
			}

			pThis->pMediaSource = CreateMediaStreamSource(pThis->m_szShareMediaURL, pThis->hParent, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pMediaSource != NULL)
			{
				pThis->pMediaSource->netBaseNetType = pThis->netBaseNetType;
				pThis->pMediaSource->nG711ConvertAAC = atoi(pThis->m_addStreamProxyStruct.G711ConvertAAC);
				sprintf(pThis->pMediaSource->sourceURL, "rtp://%s:%d/%s/%s", pThis->szClientIP, pThis->nClientPort, pThis->m_openRtpServerStruct.app, pThis->m_openRtpServerStruct.stream_id);
			}
		}

		if (pThis->nSSRC == 0)
		   pThis->nSSRC = cb->ssrc; //Ĭ�ϵ�һ��ssrc 
		if (pThis->pMediaSource && pThis->nSSRC == cb->ssrc && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] >= 0x31 && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] <= 0x33 )
		{
 			if (!pThis->bUpdateVideoFrameSpeedFlag && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] >= 0x32 && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] <= 0x33)
			{//������ƵԴ��֡�ٶ�
				int nVideoSpeed = 25;
				if (nVideoSpeed > 0)
				{
					pThis->bUpdateVideoFrameSpeedFlag = true;
  					//����UDP��TCP������Ϊ�����Ѿ����� 
					auto  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
					if (pGB28181Proxy != NULL)
						pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;

					if (pThis->pMediaSource)
					{
						pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
						pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
						pThis->pMediaSource->fileKeepMaxTime = atoi(pThis->m_addStreamProxyStruct.fileKeepMaxTime);
						pThis->pMediaSource->videoFileFormat = atoi(pThis->m_addStreamProxyStruct.videoFileFormat);
						pThis->pMediaSource->nG711ConvertAAC = atoi(pThis->m_addStreamProxyStruct.G711ConvertAAC);
					}

					WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
					pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
				}
			}

			if (pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x31)
			{//rtp + PS 
				if (ABL_MediaServerPort.gb28181LibraryUse == 1)
				{//����
				  ps_demux_input(pThis->psDeMuxHandle, cb->data, cb->datasize);
				}
				else
				{//�����ϳ�
				   if(pThis->psBeiJingLaoChen)
					 ps_demuxer_input(pThis->psBeiJingLaoChen, cb->data, cb->datasize);
				}
			}
			else if (pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] >= 0x32 && pThis->m_addStreamProxyStruct.RtpPayloadDataType[0] <= 0x33)
			{// rtp + ES \ rtp + XHB 
				 if(cb->payload == 98 )
				    pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
				 else if(cb->payload == 99)
				    pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");
				 else if(cb->payload == 0)//g711u 
					pThis->pMediaSource->PushAudio(cb->data, cb->datasize, "G711_U",1,8000);
				 else if (cb->payload == 8)//g711a
					 pThis->pMediaSource->PushAudio(cb->data, cb->datasize, "G711_A", 1, 8000);
				 else if (cb->payload == 97)//aac
				 {
#ifdef WriteRecvAACDataFlag
					 if (pThis->fWriteRecvAACFile)
					 {
						 fwrite(cb->data, 1, cb->datasize, pThis->fWriteRecvAACFile);
						 fflush(pThis->fWriteRecvAACFile);
					 }
#endif
					 //��ȡAAC��ʽ
 					 if (pThis->nRecvChannels == 0 && pThis->nRecvSampleRate == 0)
						 pThis->GetAACAudioInfo2(cb->data, cb->datasize, &pThis->nRecvSampleRate, &pThis->nRecvChannels);

 					 if(cb->datasize > 0 &&  cb->datasize < 2048 )
					    pThis->pMediaSource->PushAudio((unsigned char*)cb->data, cb->datasize,"AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
				 }
			}
  		}

	   if (ABL_MediaServerPort.nSaveGB28181Rtp == 1 && pThis->fWritePsFile != NULL && (GetTickCount64() - pThis->nCreateDateTime) < 1000 * 180 )
 	   {
		   fwrite(cb->data,1,cb->datasize,pThis->fWritePsFile);
		   fflush(pThis->fWritePsFile);
	   }
 	}
}

//��1078���ݽ����и� 
void  CNetGB28181RtpServer::SplitterJt1078CacheBuffer()
{
	Jt1078VideoRtpPacket_T*      p1078VideoHead;
	Jt1078AudioRtpPacket_T*      p1078AudioHead;
	Jt1078OtherRtpPacket_T*      pOtherHead;
	n1078Pos = 0;
	n1078CurrentProcCountLength = n1078CacheBufferLength;//��ǰ������ܳ��� 
	if (pMediaSource == NULL && m_openRtpServerStruct.RtpPayloadDataType[0] == 0x34)
	{
		if (strlen(m_openRtpServerStruct.app) > 0 && strlen(m_openRtpServerStruct.stream_id) > 0)
		{
			sprintf(m_szShareMediaURL, "/%s/%s", m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);
		    pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hParent, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
			if (pMediaSource != NULL)
			{
				pMediaSource->netBaseNetType = netBaseNetType;
				pMediaSource->enable_mp4 = strcmp(m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
				pMediaSource->enable_hls = strcmp(m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
				pMediaSource->fileKeepMaxTime = atoi(m_addStreamProxyStruct.fileKeepMaxTime);
				pMediaSource->videoFileFormat = atoi(m_addStreamProxyStruct.videoFileFormat);
				pMediaSource->nG711ConvertAAC = atoi(m_addStreamProxyStruct.G711ConvertAAC);
				sprintf(pMediaSource->sourceURL, "rtp://%s:%d/%s/%s", szClientIP, nClientPort, m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);
			}
		}
		else
		{
			if (n1078CacheBufferLength < sizeof(Jt1078VideoRtpPacket_T))
				return;

			p1078VideoHead = (Jt1078VideoRtpPacket_T*)(netDataCache + n1078Pos);
 			sprintf(sim, "%02X%02X%02X%02X%02X%02X", p1078VideoHead->sim[0], p1078VideoHead->sim[1], p1078VideoHead->sim[2], p1078VideoHead->sim[3], p1078VideoHead->sim[4], p1078VideoHead->sim[5]);
			UpdateSim(sim);
			jtt1078_KeepOpenPortType = atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType);
		
			if(jtt1078_KeepOpenPortType == 0 || jtt1078_KeepOpenPortType == 1)
			  sprintf(m_szShareMediaURL, "/%s/%s_%d", "1078", sim, p1078VideoHead->ch);//ʵ������
			else if (jtt1078_KeepOpenPortType == 2)
				sprintf(m_szShareMediaURL, "/%s/%s_%d_playback", "1078", sim, p1078VideoHead->ch); //¼��ط�
			else if (jtt1078_KeepOpenPortType == 3)
				sprintf(m_szShareMediaURL, "/%s/%s_%d_talk", "1078", sim, p1078VideoHead->ch); //�����Խ�
			else if (jtt1078_KeepOpenPortType == 4)
				sprintf(m_szShareMediaURL, "/%s/%s_%d_sub", "1078", sim, p1078VideoHead->ch); //����������
			else
				sprintf(m_szShareMediaURL, "/%s/%s_%d", "1078", sim, p1078VideoHead->ch);

			auto tmpSoure = GetMediaStreamSource(m_szShareMediaURL);
			if (tmpSoure != NULL)//�Ѿ�����������
			{
				if (jtt1078_KeepOpenPortType == 0 || jtt1078_KeepOpenPortType == 1)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_%llu", "1078", sim, p1078VideoHead->ch, nClient);//ʵ������
				else if (jtt1078_KeepOpenPortType == 2)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_playback_%llu", "1078", sim, p1078VideoHead->ch, nClient); //¼��ط�
				else if (jtt1078_KeepOpenPortType == 3)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_talk_%llu", "1078", sim, p1078VideoHead->ch, nClient); //�����Խ�
				else if (jtt1078_KeepOpenPortType == 4)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_sub_%llu", "1078", sim, p1078VideoHead->ch, nClient); //����������
				else
					sprintf(m_szShareMediaURL, "/%s/%s_%d_%llu", "1078", sim, p1078VideoHead->ch, nClient);
  			}

			pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
			if (pMediaSource)
			{
				pMediaSource->netBaseNetType = netBaseNetType;
				sprintf(pMediaSource->sourceURL, "rtp://%s:%d%s", szClientIP, nClientPort, m_szShareMediaURL);
			}
		}
 	}
  
	nStartProcessJtt1078Time = GetTickCount64();
	while (n1078CacheBufferLength >= 4096 && pMediaSource != NULL && (GetTickCount64() - nStartProcessJtt1078Time) <= 1000 * 3)
	{
		p1078VideoHead = (Jt1078VideoRtpPacket_T*)(netDataCache + n1078Pos);

		//�жϱ�־ͷ
		if (!(netDataCache[n1078Pos] == 0x30 && netDataCache[n1078Pos + 1] == 0x31 && netDataCache[n1078Pos + 2] == 0x63 && netDataCache[n1078Pos + 3] == 0x64))
		{
			nFind1078FlagPos = Find1078HeadFromCacheBuffer(netDataCache + n1078Pos, n1078CurrentProcCountLength - n1078Pos);
			if (nFind1078FlagPos < 0)
			{//�����д��󣬶����������� 
				n1078CacheBufferLength = 0;
				return;
			}
			else
			{//�ҵ���־λ�ã����¶�λ
				n1078NewPosition = n1078Pos + nFind1078FlagPos ;// n1078Pos Ϊ�Ѿ����ѵ�����λ�á�nFind1078FlagPos ��Ϊ�������һЩ�������� �Ӷ����������� ����λ�� 
				memmove(netDataCache, netDataCache + n1078NewPosition, n1078CurrentProcCountLength - n1078NewPosition);
				return; //�ȴ���һ�ζ�1078���ݽ��н�� 
			}
		}

		if (p1078VideoHead->frame_type == 0 || p1078VideoHead->frame_type == 1 || p1078VideoHead->frame_type == 2)
		{//��Ƶ
			nPayloadSize = ntohs(p1078VideoHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos ) > nPayloadSize)
			{
#ifdef WriteJt1078VideoFlag
				fwrite((unsigned char*)(netDataCache + (sizeof(Jt1078VideoRtpPacket_T) + n1078Pos)),1, nPayloadSize, fWrite1078File);
				fflush(fWrite1078File);
#endif
 				if (Ma1078CacheBufferLength - n1078VideoFrameBufferLength >= nPayloadSize)
				{
					memcpy(p1078VideoFrameBuffer + n1078VideoFrameBufferLength, netDataCache + (sizeof(Jt1078VideoRtpPacket_T) + n1078Pos), nPayloadSize);
					n1078VideoFrameBufferLength += nPayloadSize;

					if (nVideoPT == -1)
						nVideoPT = p1078VideoHead->pt;

					if (p1078VideoHead->packet_type == 0 && m_openRtpServerStruct.disableVideo[0] == 0x30 )
					{//����һ�������ɷָ�
						if (nVideoPT == 98)
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0;
					}
					else if (p1078VideoHead->packet_type == 2 && m_openRtpServerStruct.disableVideo[0] == 0x30)
					{//���1��
 						if(nVideoPT == 98 )
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0 ;
					}

					if (!bUpdateVideoFrameSpeedFlag)
					{//������ƵԴ��֡�ٶ�
 						if (ntohs(p1078VideoHead->frame_interval) > 0 && ntohs(p1078VideoHead->frame_interval) <= 1000)
						{
							strcpy(pMediaSource->sim, sim);
							bUpdateVideoFrameSpeedFlag = true;
							pMediaSource->UpdateVideoFrameSpeed(1000 / ntohs(p1078VideoHead->frame_interval), netBaseNetType);
							auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
						if (GetTickCount64() - nCreateDateTime >= 5000)
						{//�������ʧ�ܣ���������һ��Ĭ�ϵ�֡�ٶ� 
							bUpdateVideoFrameSpeedFlag = true;
							pMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
							auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
 					}
				}
				else
					n1078VideoFrameBufferLength = 0;
 
				n1078CacheBufferLength -= sizeof(Jt1078VideoRtpPacket_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078VideoRtpPacket_T) + nPayloadSize;
			}
 		}
		else if (p1078VideoHead->frame_type == 3)
		{//��Ƶ 
			Jt1078AudioRtpPacket_T* p1078AudioHead = (Jt1078AudioRtpPacket_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(p1078AudioHead->payload_size);

 			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
				if (nAudioPT == -1)
					nAudioPT = p1078AudioHead->pt;

				if (nAudioPT == 6 && m_openRtpServerStruct.disableAudio[0] == 0x30)
				{
					if(nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead,4) == 0)
					  pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos+4)), nPayloadSize-4, "G711_A", 1, 8000);
					else 
					  pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, "G711_A", 1, 8000);
				}
				else if (nAudioPT == 7 && m_openRtpServerStruct.disableAudio[0] == 0x30)
				{
					if (nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead, 4) == 0)
					  pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos+4)), nPayloadSize-4, "G711_U", 1, 8000);
					else 
					  pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, "G711_U", 1, 8000);
				}
				else if (nAudioPT == 19 && m_openRtpServerStruct.disableAudio[0] == 0x30)
				{//׼ȷ����aac��Ƶ��ʽ 
					
					//��ȡAAC��ʽ
					if (nRecvChannels == 0 && nRecvSampleRate == 0)
						GetAACAudioInfo2((netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, &nRecvSampleRate, &nRecvChannels);
 
					pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T) + n1078Pos)), nPayloadSize, "AAC", nRecvChannels, nRecvSampleRate);
				}

				n1078CacheBufferLength -= sizeof(Jt1078AudioRtpPacket_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078AudioRtpPacket_T) + nPayloadSize;
			}

			if (bUpdateVideoFrameSpeedFlag == false && (GetTickCount64() - nCreateDateTime >= 5000))
			{//���ֻ����Ƶ����Ҫ���¸�������Ѿ��յ�������  
				bUpdateVideoFrameSpeedFlag = true;
				strcpy(pMediaSource->sim, sim);
				pMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
				auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
			}
		}
		else if (p1078VideoHead->frame_type == 4)
		{//͸������
			Jt1078OtherRtpPacket_T* pOtherHead = (Jt1078OtherRtpPacket_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(pOtherHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
 
				n1078CacheBufferLength -= sizeof(Jt1078OtherRtpPacket_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078OtherRtpPacket_T) + nPayloadSize;
			}
		}
 	}

	//��ʣ��������ƶ�����
	if (n1078CacheBufferLength > 0 && n1078Pos > 0 )
		memmove(netDataCache, netDataCache + n1078Pos, n1078CacheBufferLength);
}

//��1078���ݽ����и� 
void  CNetGB28181RtpServer::SplitterJt1078CacheBuffer2019()
{
	Jt1078VideoRtpPacket2019_T*      p1078VideoHead;
	Jt1078AudioRtpPacket2019_T*      p1078AudioHead;
	Jt1078OtherRtpPacket2019_T*      pOtherHead;

	n1078Pos = 0;
	n1078CurrentProcCountLength = n1078CacheBufferLength;//��ǰ������ܳ��� 
	if (pMediaSource == NULL && m_openRtpServerStruct.RtpPayloadDataType[0] == 0x34)
	{
		if (strlen(m_openRtpServerStruct.app) > 0 && strlen(m_openRtpServerStruct.stream_id) > 0)
		{
			sprintf(m_szShareMediaURL, "/%s/%s", m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);
			pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, hParent, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
			if (pMediaSource != NULL)
			{
				pMediaSource->netBaseNetType = netBaseNetType;
				pMediaSource->enable_mp4 = strcmp(m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
				pMediaSource->enable_hls = strcmp(m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
				pMediaSource->fileKeepMaxTime = atoi(m_addStreamProxyStruct.fileKeepMaxTime);
				pMediaSource->videoFileFormat = atoi(m_addStreamProxyStruct.videoFileFormat);
				pMediaSource->nG711ConvertAAC = atoi(m_addStreamProxyStruct.G711ConvertAAC);
				sprintf(pMediaSource->sourceURL, "rtp://%s:%d/%s/%s", szClientIP, nClientPort, m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);
			}
		}
		else
		{
			if (n1078CacheBufferLength < sizeof(Jt1078VideoRtpPacket_T))
				return;

			p1078VideoHead = (Jt1078VideoRtpPacket2019_T*)(netDataCache + n1078Pos);
			sprintf(sim, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", p1078VideoHead->sim[0], p1078VideoHead->sim[1], p1078VideoHead->sim[2], p1078VideoHead->sim[3], p1078VideoHead->sim[4], p1078VideoHead->sim[5], p1078VideoHead->sim[6], p1078VideoHead->sim[7], p1078VideoHead->sim[8], p1078VideoHead->sim[9]);
			UpdateSim(sim);
			jtt1078_KeepOpenPortType = atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType);
		
			if(jtt1078_KeepOpenPortType == 0 || jtt1078_KeepOpenPortType == 1)
			  sprintf(m_szShareMediaURL, "/%s/%s_%d", "1078", sim, p1078VideoHead->ch);//ʵ������
			else if (jtt1078_KeepOpenPortType == 2)
				sprintf(m_szShareMediaURL, "/%s/%s_%d_playback", "1078", sim, p1078VideoHead->ch); //¼��ط�
			else if (jtt1078_KeepOpenPortType == 3)
				sprintf(m_szShareMediaURL, "/%s/%s_%d_talk", "1078", sim, p1078VideoHead->ch); //�����Խ�
			else if (jtt1078_KeepOpenPortType == 4)
				sprintf(m_szShareMediaURL, "/%s/%s_%d_sub", "1078", sim, p1078VideoHead->ch); //����������
			else
				sprintf(m_szShareMediaURL, "/%s/%s_%d", "1078", sim, p1078VideoHead->ch);

			auto tmpSoure = GetMediaStreamSource(m_szShareMediaURL);
			if (tmpSoure != NULL)//�Ѿ�����������
			{
				if (jtt1078_KeepOpenPortType == 0 || jtt1078_KeepOpenPortType == 1)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_%llu", "1078", sim, p1078VideoHead->ch, nClient);//ʵ������
				else if (jtt1078_KeepOpenPortType == 2)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_playback_%llu", "1078", sim, p1078VideoHead->ch, nClient); //¼��ط�
				else if (jtt1078_KeepOpenPortType == 3)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_talk_%llu", "1078", sim, p1078VideoHead->ch, nClient); //�����Խ�
				else if (jtt1078_KeepOpenPortType == 4)
					sprintf(m_szShareMediaURL, "/%s/%s_%d_sub_%llu", "1078", sim, p1078VideoHead->ch, nClient); //����������
				else
					sprintf(m_szShareMediaURL, "/%s/%s_%d_%llu", "1078", sim, p1078VideoHead->ch, nClient);
  			}
			pMediaSource = CreateMediaStreamSource(m_szShareMediaURL, nClient, MediaSourceType_LiveMedia, 0, m_h265ConvertH264Struct);
			if (pMediaSource)
			{
				pMediaSource->netBaseNetType = netBaseNetType;
				sprintf(pMediaSource->sourceURL, "rtp://%s:%d%s", szClientIP, nClientPort, m_szShareMediaURL);
			}
		}
	}

	nStartProcessJtt1078Time = GetTickCount64();
	while (n1078CacheBufferLength >= 4096 && pMediaSource != NULL && (GetTickCount64() - nStartProcessJtt1078Time) <= 1000 * 3 )
	{
		p1078VideoHead = (Jt1078VideoRtpPacket2019_T*)(netDataCache + n1078Pos);

		//�жϱ�־ͷ
		if (!(netDataCache[n1078Pos] == 0x30 && netDataCache[n1078Pos + 1] == 0x31 && netDataCache[n1078Pos + 2] == 0x63 && netDataCache[n1078Pos + 3] == 0x64))
		{
			nFind1078FlagPos = Find1078HeadFromCacheBuffer(netDataCache + n1078Pos, n1078CurrentProcCountLength - n1078Pos);
			if (nFind1078FlagPos < 0)
			{//�����д��󣬶����������� 
				n1078CacheBufferLength = 0;
				return;
			}
			else
			{//�ҵ���־λ�ã����¶�λ
				n1078NewPosition = n1078Pos + nFind1078FlagPos;// n1078Pos Ϊ�Ѿ����ѵ�����λ�á�nFind1078FlagPos ��Ϊ�������һЩ�������� �Ӷ����������� ����λ�� 
				memmove(netDataCache, netDataCache + n1078NewPosition, n1078CurrentProcCountLength - n1078NewPosition);
				return; //�ȴ���һ�ζ�1078���ݽ��н�� 
			}
		}

		if (p1078VideoHead->frame_type == 0 || p1078VideoHead->frame_type == 1 || p1078VideoHead->frame_type == 2)
		{//��Ƶ
			nPayloadSize = ntohs(p1078VideoHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
#ifdef WriteJt1078VideoFlag
				fwrite((unsigned char*)(netDataCache + (sizeof(Jt1078VideoRtpPacket2019_T) + n1078Pos)), 1, nPayloadSize, fWrite1078File);
				fflush(fWrite1078File);
#endif
				if (Ma1078CacheBufferLength - n1078VideoFrameBufferLength >= nPayloadSize)
				{
					memcpy(p1078VideoFrameBuffer + n1078VideoFrameBufferLength, netDataCache + (sizeof(Jt1078VideoRtpPacket2019_T) + n1078Pos), nPayloadSize);
					n1078VideoFrameBufferLength += nPayloadSize;

					if (nVideoPT == -1)
						nVideoPT = p1078VideoHead->pt;

					if (p1078VideoHead->packet_type == 0 && m_openRtpServerStruct.disableVideo[0] == 0x30)
					{//����һ�������ɷָ�
						if (nVideoPT == 98)
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0;
					}
					else if (p1078VideoHead->packet_type == 2 && m_openRtpServerStruct.disableVideo[0] == 0x30)
					{//���1��
						if (nVideoPT == 98)
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H264");
						else if (nVideoPT == 99)
							pMediaSource->PushVideo(p1078VideoFrameBuffer, n1078VideoFrameBufferLength, "H265");

						n1078VideoFrameBufferLength = 0;
					}

					if (!bUpdateVideoFrameSpeedFlag)
					{//������ƵԴ��֡�ٶ�
						if (ntohs(p1078VideoHead->frame_interval) > 0 && ntohs(p1078VideoHead->frame_interval) <= 1000)
						{
							strcpy(pMediaSource->sim, sim);
							bUpdateVideoFrameSpeedFlag = true;
							pMediaSource->UpdateVideoFrameSpeed(1000 / ntohs(p1078VideoHead->frame_interval), netBaseNetType);
							auto pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
						if (GetTickCount64() - nCreateDateTime >= 5000)
						{//�������ʧ�ܣ���������һ��Ĭ�ϵ�֡�ٶ� 
							bUpdateVideoFrameSpeedFlag = true;
							pMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
							auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
							if (pGB28181Proxy != NULL)
								pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
						}
					}
				}
				else
					n1078VideoFrameBufferLength = 0;

				n1078CacheBufferLength -= sizeof(Jt1078VideoRtpPacket2019_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078VideoRtpPacket2019_T) + nPayloadSize;
			}
		}
		else if (p1078VideoHead->frame_type == 3)
		{//��Ƶ 
			Jt1078AudioRtpPacket2019_T* p1078AudioHead = (Jt1078AudioRtpPacket2019_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(p1078AudioHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{
				if (nAudioPT == -1)
					nAudioPT = p1078AudioHead->pt;

				if (nAudioPT == 6 && m_openRtpServerStruct.disableAudio[0] == 0x30)
				{
					if(nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead, 4) == 0)
  					  pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos + 4)), nPayloadSize - 4, "G711_A", 1, 8000);
					else 
					 pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, "G711_A", 1, 8000);
				}
				else if (nAudioPT == 7 && m_openRtpServerStruct.disableAudio[0] == 0x30)
				{
					if (nPayloadSize == 324 || nPayloadSize == 164 || memcmp((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket_T)) + n1078Pos), sz1078AudioFrameHead, 4) == 0)
					  pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos+4)), nPayloadSize-4, "G711_U", 1, 8000);
					else 
					  pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, "G711_U", 1, 8000);
				}
				else if (nAudioPT == 19 && m_openRtpServerStruct.disableAudio[0] == 0x30)
				{//׼ȷ����aac��Ƶ��ʽ 
					//��ȡAAC��ʽ
					if (nRecvChannels == 0 && nRecvSampleRate == 0)
						GetAACAudioInfo2((netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, &nRecvSampleRate, &nRecvChannels);
 
					pMediaSource->PushAudio((unsigned char*)(netDataCache + (sizeof(Jt1078AudioRtpPacket2019_T) + n1078Pos)), nPayloadSize, "AAC", nRecvChannels, nRecvSampleRate);
				}
 
				n1078CacheBufferLength -= sizeof(Jt1078AudioRtpPacket2019_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078AudioRtpPacket2019_T) + nPayloadSize;
			}

			if (bUpdateVideoFrameSpeedFlag == false && (GetTickCount64() - nCreateDateTime >= 5000) )
			{//���ֻ����Ƶ����Ҫ���¸�������Ѿ��յ�������  
				bUpdateVideoFrameSpeedFlag = true;
				strcpy(pMediaSource->sim, sim);
				pMediaSource->UpdateVideoFrameSpeed(25, netBaseNetType);
				auto  pGB28181Proxy = GetNetRevcBaseClient(hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;
			}
		}
		else if (p1078VideoHead->frame_type == 4)
		{//͸������
			Jt1078OtherRtpPacket2019_T* pOtherHead = (Jt1078OtherRtpPacket2019_T*)(netDataCache + n1078Pos);
			nPayloadSize = ntohs(pOtherHead->payload_size);

			if (nPayloadSize > 0 && (n1078CurrentProcCountLength - n1078Pos) > nPayloadSize)
			{

				n1078CacheBufferLength -= sizeof(Jt1078OtherRtpPacket2019_T) + nPayloadSize;
				n1078Pos += sizeof(Jt1078OtherRtpPacket2019_T) + nPayloadSize;
			}
		}
	}

	//��ʣ��������ƶ�����
	if (n1078CacheBufferLength > 0 && n1078Pos > 0)
		memmove(netDataCache, netDataCache + n1078Pos, n1078CacheBufferLength);
}

//�ڻ����в���1078��־ 0x30 ,0x31 ,0x63, 0x64 
int   CNetGB28181RtpServer::Find1078HeadFromCacheBuffer(unsigned char* pData, int nLength)
{
	if (nLength <= 4 || pData == NULL )
		return -1;

	for (int i = 0; i < nLength - 4 ; i++)
	{
		if (pData[i] == 0x30 && pData[i+1] == 0x31 && pData[i + 2] == 0x63 && pData[i + 3] == 0x64)
		{
			return i;
		}
	}
	return -2;
}

static int on_gb28181_unpacket(void* param, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)param;
	if (!pThis->bRunFlag.load())
		return -1;

	if (pThis->pMediaSource == NULL)
	{
		pThis->bRunFlag.exchange(false);
		pDisconnectBaseNetFifo.push((unsigned char*)&pThis->nClient, sizeof(pThis->nClient));
 		return -1;
	}

 	if ((PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_G711A == avtype || PSI_STREAM_AUDIO_G711U == avtype) && pThis->m_addStreamProxyStruct.disableAudio[0] == 0x30)
	{
		if (PSI_STREAM_AAC == avtype)
		{//aac
		    //��ȡAACý����Ϣ
			if (pThis->nRecvChannels == 0 && pThis->nRecvSampleRate == 0)
				pThis->GetAACAudioInfo2((unsigned char*)data, bytes, &pThis->nRecvSampleRate, &pThis->nRecvChannels);
 
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, "AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
		}
		else if (PSI_STREAM_AUDIO_G711A == avtype)
		{// G711A  
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_A", 1, 8000);
		}
		else if (PSI_STREAM_AUDIO_G711U == avtype)
		{// G711U  
			pThis->pMediaSource->PushAudio((unsigned char*)data, bytes, "G711_U", 1, 8000);
		}
	}
	else if (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype || PSI_STREAM_VIDEO_SVAC == avtype)
	{
		if (pThis->m_addStreamProxyStruct.disableVideo[0] == 0x30)
		{//�������û�й��˵���Ƶ
			if (PSI_STREAM_H264 == avtype)
				pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
			else if (PSI_STREAM_H265 == avtype)
				pThis->pMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
		}
	}

	if (!pThis->bUpdateVideoFrameSpeedFlag)
	{//������ƵԴ��֡�ٶ�
		int nVideoSpeed = 25 ;
		if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
		{
			pThis->bUpdateVideoFrameSpeedFlag = true;

			//����UDP��TCP������Ϊ�����Ѿ����� 
			auto  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
			if (pGB28181Proxy != NULL)
				pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;

			if (pThis->pMediaSource)
			{
				pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
				pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
				pThis->pMediaSource->fileKeepMaxTime = atoi(pThis->m_addStreamProxyStruct.fileKeepMaxTime) ;
				pThis->pMediaSource->nG711ConvertAAC = atoi(pThis->m_addStreamProxyStruct.G711ConvertAAC);
			}

			WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
			pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
		}
	}

	return 0;
}
static void mpeg_ps_dec_testonstream(void* param, int stream, int codecid, const void* extra, int bytes, int finish)
{
	printf("stream %d, codecid: %d, finish: %s\n", stream, codecid, finish ? "true" : "false");
}

void PS_DEMUX_CALL_METHOD GB28181_RtpRecv_demux_callback(_ps_demux_cb* cb)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)cb->userdata;
	if (!pThis->bRunFlag.load())
		return;
 
	if (pThis && cb->streamtype == e_rtpdepkt_st_h264 || cb->streamtype == e_rtpdepkt_st_h265 ||
		cb->streamtype == e_rtpdepkt_st_mpeg4 || cb->streamtype == e_rtpdepkt_st_mjpeg)
	{
		if (pThis->pMediaSource == NULL)
		{
			pThis->pMediaSource = CreateMediaStreamSource(pThis->m_szShareMediaURL, pThis->hParent, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pMediaSource != NULL)
			{
				pThis->pMediaSource->netBaseNetType = pThis->netBaseNetType;
				sprintf(pThis->pMediaSource->sourceURL, "rtp://%s:%d/%s/%s", pThis->szClientIP, pThis->nClientPort, pThis->m_openRtpServerStruct.app, pThis->m_openRtpServerStruct.stream_id);
			}
		}
		
		if(pThis->pMediaSource == NULL)
		{
			pThis->bRunFlag.exchange(false);
			pDisconnectBaseNetFifo.push((unsigned char*)&pThis->nClient, sizeof(pThis->nClient));
 			return ;
		}

		if (pThis->m_addStreamProxyStruct.disableVideo[0] == 0x30)
		{//�������û�й��˵���Ƶ
 			if (cb->streamtype == e_rtpdepkt_st_h264)
				pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H264");
			else if (cb->streamtype == e_rtpdepkt_st_h265)
				pThis->pMediaSource->PushVideo(cb->data, cb->datasize, "H265");
		}

		if (!pThis->bUpdateVideoFrameSpeedFlag)
		{//������ƵԴ��֡�ٶ�
			int nVideoSpeed = pThis->CalcFlvVideoFrameSpeed(cb->pts, 90000);
			if (nVideoSpeed > 0 && pThis->pMediaSource != NULL)
			{
				pThis->bUpdateVideoFrameSpeedFlag = true;

				//����UDP��TCP������Ϊ�����Ѿ����� 
				auto  pGB28181Proxy = GetNetRevcBaseClient(pThis->hParent);
				if (pGB28181Proxy != NULL)
					pGB28181Proxy->bUpdateVideoFrameSpeedFlag = true;

				if (pThis->pMediaSource)
				{
					pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
					pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
				    pThis->pMediaSource->fileKeepMaxTime = atoi(pThis->m_addStreamProxyStruct.fileKeepMaxTime);
					pThis->pMediaSource->videoFileFormat = atoi(pThis->m_addStreamProxyStruct.videoFileFormat);
					pThis->pMediaSource->nG711ConvertAAC = atoi(pThis->m_addStreamProxyStruct.G711ConvertAAC);
				}

				WriteLog(Log_Debug, "nClient = %llu , ������ƵԴ %s ��֡�ٶȳɹ�����ʼ�ٶ�Ϊ%d ,���º���ٶ�Ϊ%d, ", pThis->nClient, pThis->pMediaSource->m_szURL, pThis->pMediaSource->m_mediaCodecInfo.nVideoFrameRate, nVideoSpeed);
				pThis->pMediaSource->UpdateVideoFrameSpeed(nVideoSpeed, pThis->netBaseNetType);
			}
		}
	}
	else 
	{
		if (pThis->pMediaSource == NULL)
		{
			pThis->pMediaSource = CreateMediaStreamSource(pThis->m_szShareMediaURL, pThis->hParent, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pMediaSource)
			{
				pThis->pMediaSource->netBaseNetType = pThis->netBaseNetType;
				pThis->pMediaSource->enable_mp4 = strcmp(pThis->m_addStreamProxyStruct.enable_mp4, "1") == 0 ? true : false;//��¼�Ƿ�¼��
				pThis->pMediaSource->enable_hls = strcmp(pThis->m_addStreamProxyStruct.enable_hls, "1") == 0 ? true : false;//��¼�Ƿ���hls
				pThis->pMediaSource->fileKeepMaxTime = atoi(pThis->m_addStreamProxyStruct.fileKeepMaxTime);
				pThis->pMediaSource->videoFileFormat = atoi(pThis->m_addStreamProxyStruct.videoFileFormat);
				pThis->pMediaSource->nG711ConvertAAC = atoi(pThis->m_addStreamProxyStruct.G711ConvertAAC);
				sprintf(pThis->pMediaSource->sourceURL, "rtp://%s:%d/%s/%s", pThis->szClientIP, pThis->nClientPort, pThis->m_openRtpServerStruct.app, pThis->m_openRtpServerStruct.stream_id);
			}
		}

		if (pThis->pMediaSource == NULL)
		{
			pThis->bRunFlag.exchange(false);
			pDisconnectBaseNetFifo.push((unsigned char*)&pThis->nClient, sizeof(pThis->nClient));
 			return ;
		}

		if (pThis->m_addStreamProxyStruct.disableAudio[0] == 0x30)
		{//��Ƶû�й���
			if (cb->streamtype == e_rtpdepkt_st_aac)
			{//aac
			     //��ȡAAC��ʽ��Ϣ 
				if(pThis->nRecvChannels == 0 && pThis->nRecvSampleRate == 0)
				  pThis->GetAACAudioInfo2(cb->data, cb->datasize, &pThis->nRecvSampleRate,&pThis->nRecvChannels);

				if (pThis->nRecvChannels > 0 && pThis->nRecvSampleRate > 0)
				  pThis->pMediaSource->PushAudio(cb->data, cb->datasize, "AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
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
}

CNetGB28181RtpServer::CNetGB28181RtpServer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	memset((char*)&jt1078VideoHead, 0x00, sizeof(jt1078VideoHead));
	memset((char*)&jt1078AudioHead, 0x00, sizeof(jt1078AudioHead));
	memset((char*)&jt1078OtherHead, 0x00, sizeof(jt1078OtherHead));
	memset((char*)&jt1078VideoHead2019, 0x00, sizeof(jt1078VideoHead2019));
	memset((char*)&jt1078AudioHead2019, 0x00, sizeof(jt1078AudioHead2019));
	memset((char*)&jt1078OtherHead2019, 0x00, sizeof(jt1078OtherHead2019));
	jt1078VideoHead.head[0] = 0x30;
	jt1078VideoHead.head[1] = 0x31;
	jt1078VideoHead.head[2] = 0x63;
	jt1078VideoHead.head[3] = 0x64;
	memcpy(jt1078AudioHead.head, jt1078VideoHead.head, 4);
	memcpy(jt1078OtherHead.head, jt1078VideoHead.head, 4);

	memcpy(jt1078VideoHead2019.head, jt1078VideoHead.head, 4);
	memcpy(jt1078AudioHead2019.head, jt1078VideoHead.head, 4);
	memcpy(jt1078OtherHead2019.head, jt1078VideoHead.head, 4);

	jt1078VideoHead.v = jt1078AudioHead.v = jt1078OtherHead.v = jt1078VideoHead2019.v = jt1078AudioHead2019.v = jt1078OtherHead2019.v = 2;
	jt1078VideoHead.cc = jt1078AudioHead.cc = jt1078OtherHead.cc = jt1078VideoHead2019.cc = jt1078AudioHead2019.cc = jt1078OtherHead2019.cc = 1;
	jt1078VideoHead.m = jt1078AudioHead.m = jt1078OtherHead.m = jt1078VideoHead2019.m = jt1078AudioHead2019.m = jt1078OtherHead2019.m = 1;
	jt1078VideoHead.ch = jt1078AudioHead.ch = jt1078OtherHead.ch = jt1078VideoHead2019.ch = jt1078AudioHead2019.ch = jt1078OtherHead2019.ch = 1;

 #ifdef WriteRecvAACDataFlag
	fWriteRecvAACFile = fopen("e:\\recvES.aac", "wb");
#endif
#ifdef WriteSendPsFileFlag
	char szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.ps", ABL_MediaSeverRunPath, this);
	fWriteSendPsFile = fopen(szFileName, "wb");
#endif 
#ifdef  WriteRtpFileFlag
	char szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.rtp", ABL_MediaSeverRunPath, this);
	fWriteRtpFile = fopen(szFileName, "wb");
#endif
#ifdef  WriteRtpTimestamp
	nRtpTimestampType = -1;
	nRptDataArrayOrder = 0;
	bCheckRtpTimestamp = false;
	nStartTimestap = 3600;
#endif 
#ifdef WriteJt1078VideoFlag
	char szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.264", ABL_MediaSeverRunPath, this);
	fWrite1078File = fopen(szFileName, "wb");
#endif
	nAddSend_app_streamDatetime = GetTickCount64();
	nRecvChannels =  nRecvSampleRate = 0;
	nRecvRtpPacketCount = 0;
	nMaxRtpLength = 0;

	nVideoPT = nAudioPT = -1;
	p1078VideoFrameBuffer = NULL ;
	n1078CacheBufferLength = n1078VideoFrameBufferLength = 0;;

 	aacDataLength = 0;
	nMaxRtpSendVideoMediaBufferLength = 640;//Ĭ���ۼ�640
	nSendRtpVideoMediaBufferLength = 0; //�Ѿ����۵ĳ���  ��Ƶ
	nStartVideoTimestamp = GB28181VideoStartTimestampFlag; //��һ֡��Ƶ��ʼʱ��� ��
	nCurrentVideoTimestamp = 0;// ��ǰ֡ʱ���

	addThreadPoolFlag = false;
	videoPTS = audioPTS = 0;
	nVideoStreamID = nAudioStreamID = -1;
	handler.alloc = NetGB28181RtpServer_ps_alloc;
	handler.write = NetGB28181RtpServer_ps_write;
	handler.free = NetGB28181RtpServer_ps_free;
	videoPTS = audioPTS = 0;
	s_buffer = NULL;
	psBeiJingLaoChenMuxer = NULL;
	s_buffer = NULL;
	hRtpPS = 0;
	nClientPort = nPort;
	nClientRtcp = hParent = 0;
	fWritePsFile = NULL;
	nRtpRtcpPacketType = 0;
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	nServer = hServer;
	m_gbPayload = 96;
	hRtpHandle = psDeMuxHandle  = 0;
	psBeiJingLaoChen = NULL;
	memset((char*)&mediaCodecInfo, 0x00, sizeof(mediaCodecInfo));
	bInitFifoFlag = false;
	pMediaSource = NULL; 
	netDataCache = NULL ; //�������ݻ���
	netDataCacheLength = 0;//�������ݻ����С
	nNetStart = nNetEnd = 0; //����������ʼλ��\����λ��
	MaxNetDataCacheCount = MaxNetDataCacheBufferLength;
	nSendRtcpTime = 0;
	pRtpAddress = pSrcAddress = NULL;
	bRunFlag.exchange(true);
	fWritePsFile  = NULL ;
	pWriteRtpFile = NULL ;
	strcpy(szClientIP, szIP);
	
	if (ABL_MediaServerPort.nSaveGB28181Rtp == 1)
	{
		char szVFile[string_length_1024];
		sprintf(szVFile, "%s%llu_%X.ps", ABL_MediaServerPort.debugPath, nClient, this);
 	    fWritePsFile = fopen(szVFile,"wb") ;
		sprintf(szVFile, "%s%llu_%X.rtp", ABL_MediaServerPort.debugPath, nClient, this);
		pWriteRtpFile = fopen(szVFile, "wb");
	}

 	WriteLog(Log_Debug, "CNetGB28181RtpServer ���� = %X  nClient = %llu ", this, nClient);
}

CNetGB28181RtpServer::~CNetGB28181RtpServer()
{
	bRunFlag.exchange(false);
	std::lock_guard<std::mutex> lock(netDataLock);

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{
		XHNetSDK_DestoryUdp(nClient);
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)
	{
	}

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//����
		if (psDeMuxHandle > 0)
		{
			ps_demux_stop(psDeMuxHandle);
			psDeMuxHandle = 0;
		}
	}
	else
	{//�����ϳ�
		if (psBeiJingLaoChen != NULL)
		{
			ps_demuxer_destroy(psBeiJingLaoChen);
			psBeiJingLaoChen = NULL;
		}
	}
 
	if (hRtpHandle > 0)
	{
		rtp_depacket_stop(hRtpHandle);
		hRtpHandle = 0;
 	}

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{
	  if(bInitFifoFlag)
	    NetDataFifo.FreeFifo();
	}
 
    SAFE_ARRAY_DELETE(netDataCache);

	if (fWritePsFile)
	{
	    fclose(fWritePsFile);
		fWritePsFile = NULL;
	 }
	 if (pWriteRtpFile)
	 {
		 fclose(pWriteRtpFile);
		 pWriteRtpFile = NULL;
	 }
 
	 if(hRtpPS > 0)
	   rtp_packet_stop(hRtpPS);
	 if (psBeiJingLaoChenMuxer != NULL)
		 ps_muxer_destroy(psBeiJingLaoChenMuxer);

	 SAFE_DELETE(pRtpAddress);
	 SAFE_DELETE(pSrcAddress);
	 SAFE_ARRAY_DELETE(s_buffer);
	 SAFE_ARRAY_DELETE(p1078VideoFrameBuffer);
	 
 	 malloc_trim(0);

	 if (hParent > 0 && netBaseNetType != NetBaseNetType_NetGB28181RtpServerTCP_Active && atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) == 0 )
	 {
		 WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ������������������ hParent = %llu, nClient = %llu ,nMediaClient = %llu", this, hParent, nClient, nMediaClient);
		 pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));
	 }

	 //����rtcp 
	 if (nClientRtcp > 0)
		 XHNetSDK_DestoryUdp(nClientRtcp); 

	 //����ɾ��ý��Դ
	 if (strlen(m_szShareMediaURL) > 0 && pMediaSource != NULL )
		 pDisconnectMediaSource.push((unsigned char*)m_szShareMediaURL, strlen(m_szShareMediaURL));

#ifdef WriteSendPsFileFlag
	 if(fWriteSendPsFile)
 	    fclose(fWriteSendPsFile);
#endif 
#ifdef  WriteRtpFileFlag
	 if(fWriteRtpFile)
 	   fclose(fWriteRtpFile);
#endif
#ifdef WriteRecvAACDataFlag
	 fclose(fWriteRecvAACFile);
#endif
#ifdef WriteJt1078VideoFlag
	 fclose(fWrite1078File);
#endif
	 //����û�дﵽ֪ͨ
	 if (ABL_MediaServerPort.hook_enable == 1 && bUpdateVideoFrameSpeedFlag == false)
	 {
		 MessageNoticeStruct msgNotice;
		 msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_not_arrive;
		 sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_not_arrive\",\"mediaServerId\":\"%s\",\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"key\":%llu}", ABL_MediaServerPort.mediaServerID,m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream,  netBaseNetType, nClient);
		 pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	 }

	 WriteLog(Log_Debug, "CNetGB28181RtpServer ���� = %X  nClient = %llu , hParent= %llu , app = %s ,stream = %s ,bUpdateVideoFrameSpeedFlag = %d ", this, nClient, hParent, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream, bUpdateVideoFrameSpeedFlag);
}

int CNetGB28181RtpServer::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	std::lock_guard<std::mutex> lock(netDataLock);

	if (!bRunFlag.load() || m_openRtpServerStruct.send_disableVideo[0] == 0x31)
		return -1;

	if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x31)
	{//PS 
		if (nVideoStreamID != -1 && psBeiJingLaoChenMuxer != NULL && strlen(mediaCodecInfo.szVideoName) > 0)
		{
			if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				nflags = CheckVideoIsIFrame("H264", pVideoData, nDataLength);
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				nflags = CheckVideoIsIFrame("H265", pVideoData, nDataLength);

			ps_muxer_input((ps_muxer_t*)psBeiJingLaoChenMuxer, nVideoStreamID, nflags, videoPTS, videoPTS, pVideoData, nDataLength);
			videoPTS += (90000 / mediaCodecInfo.nVideoFrameRate);
		}
	}
	else if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x32)
	{//ES 
		if (hRtpPS > 0 )
		{
			inputPS.data = pVideoData;
			inputPS.datasize = nDataLength;
			rtp_packet_input(&inputPS);
		}
	}else if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x34)
	{//jtt1078 
		if (strcmp(m_openRtpServerStruct.jtt1078_version, "2013") == 0 || strcmp(m_openRtpServerStruct.jtt1078_version, "2016") == 0)
			SendJtt1078VideoPacket(pVideoData,nDataLength);
		else if (strcmp(m_openRtpServerStruct.jtt1078_version, "2019") == 0)
			SendJtt1078VideoPacket2019(pVideoData, nDataLength);
	}
	return 0;
}

int CNetGB28181RtpServer::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	std::lock_guard<std::mutex> lock(netDataLock);

	if (!bRunFlag.load() || m_openRtpServerStruct.send_disableAudio[0] == 0x31)
		return -1;

	if (strlen(mediaCodecInfo.szAudioName) > 0)
	{
		if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x31 && nAudioStreamID != -1 &&  psBeiJingLaoChenMuxer != NULL )
		{//PS 
			ps_muxer_input((ps_muxer_t*)psBeiJingLaoChenMuxer, nAudioStreamID, 0, audioPTS, audioPTS, pAudioData, nDataLength);

			if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
				audioPTS += mediaCodecInfo.nBaseAddAudioTimeStamp;
			else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
				audioPTS += nDataLength / 8;
		}
		else if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x32)
		{//ES 
			inputPS.data = pAudioData;
			inputPS.datasize = nDataLength;
			rtp_packet_input(&inputPS);
		}
		else if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x34)
		{//jtt1078 
			if (strcmp(m_openRtpServerStruct.jtt1078_version, "2013") == 0 || strcmp(m_openRtpServerStruct.jtt1078_version, "2016") == 0)
				SendJtt1078AduioPacket(pAudioData,nDataLength);
			else if (strcmp(m_openRtpServerStruct.jtt1078_version, "2019") == 0)
				SendJtt1078AduioPacket2019(pAudioData, nDataLength);
		}
	}
	return 0;
}

int CNetGB28181RtpServer::SendVideo()
{

	return 0;
}

int CNetGB28181RtpServer::SendAudio()
{

	return 0;
}

int CNetGB28181RtpServer::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (!bRunFlag.load() || nDataLength <= 0 )
		return -1;
	std::lock_guard<std::mutex> lock(netDataLock);
 
#ifdef  WriteRtpFileFlag
	if (fWriteRtpFile)
	{
		fwrite((unsigned char*)&nDataLength, 1, sizeof(nDataLength), fWriteRtpFile);
		fwrite(pData, 1, nDataLength, fWriteRtpFile);
		fflush(fWriteRtpFile);
		return 0;
     }
#endif

	//���ӱ���ԭʼ��rtp���ݣ����еײ����
	if (ABL_MediaServerPort.nSaveGB28181Rtp == 1 && pWriteRtpFile != NULL && (GetTickCount64() - nCreateDateTime) < 1000 * 300)
	{
		fwrite(pData, 1, nDataLength, pWriteRtpFile);
		fflush(pWriteRtpFile);
	}

	//������߼��
	nRecvDataTimerBySecond = 0;

	if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x34)
	{//jt 1078 ���������Ǳ�׼�� rtp �� 
		if (p1078VideoFrameBuffer == NULL)
  			p1078VideoFrameBuffer = new unsigned char[Ma1078CacheBufferLength];
		if(netDataCache == NULL )
			netDataCache = new unsigned char[Ma1078CacheBufferLength];

		if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
		{//UDP
			if (pRtpAddress == NULL && address != NULL )
			{
				pRtpAddress = new sockaddr_in;
				memcpy((char*)pRtpAddress, (char*)address, sizeof(sockaddr_in));
				strcpy(szClientIP, inet_ntoa(pRtpAddress->sin_addr));
				nClientPort = ntohs(pRtpAddress->sin_port);
			}
		}

 		if (Ma1078CacheBufferLength - n1078CacheBufferLength >= nDataLength)
		{
			memcpy(netDataCache + n1078CacheBufferLength, pData, nDataLength);
			n1078CacheBufferLength += nDataLength;
		}
		else
			n1078CacheBufferLength = 0;
		
		return 0 ;
	}

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{//UDP
		if (pRtpAddress == NULL && pSrcAddress == NULL &&  address != NULL )
		{
			pRtpAddress = new sockaddr_in;
			pSrcAddress = new sockaddr_in;
			memcpy((char*)pRtpAddress, (char*)address, sizeof(sockaddr_in));
			memcpy((char*)pSrcAddress, (char*)address, sizeof(sockaddr_in));
			unsigned short nPort = ntohs(pRtpAddress->sin_port);
			pRtpAddress->sin_port = htons(nPort + 1);
 			sprintf(m_addStreamProxyStruct.url, "rtp://%s:%d/%s/%s", inet_ntoa(pSrcAddress->sin_addr), ntohs(pSrcAddress->sin_port), m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
			strcpy(szClientIP, inet_ntoa(pRtpAddress->sin_addr));
			nClientPort = ntohs(pRtpAddress->sin_port);
		}

		//��ȡ��һ��ssrc 
		if (nSSRC == 0 && pData != NULL &&  nDataLength > sizeof(rtpHeader) )
		{
			rtpHeaderPtr = (_rtp_header*)pData;
			nSSRC = rtpHeaderPtr->ssrc;
		}else 
			rtpHeaderPtr = (_rtp_header*)pData;

		if (!bInitFifoFlag)
		{
	        NetDataFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
			bInitFifoFlag = true;
	    }

		if (address && pRtpAddress)
		{//��ֻ֤�ϵ�һ��IP���˿ڽ�����PS���� 
			if (strcmp(inet_ntoa(pRtpAddress->sin_addr), inet_ntoa(((sockaddr_in*)address)->sin_addr)) == 0 &&  //IP ��ͬ
				ntohs(pRtpAddress->sin_port) - 1 == ntohs(((sockaddr_in*)address)->sin_port) &&  //�˿���ͬ
				nSSRC == rtpHeaderPtr->ssrc  // ssrc ��ͬ 
				)
			{
 	         NetDataFifo.push((unsigned char*)pData, nDataLength);
			 if (nClientPort == 0)
			 {
				 strcpy(szClientIP, inet_ntoa(pRtpAddress->sin_addr));
				 nClientPort = ntohs(((sockaddr_in*)address)->sin_port);
			 }
			}
 		}
	}
	else if (netDataCache != NULL && (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream))
	{//TCP 
 		if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
		{//ʣ��ռ��㹻
			memcpy(netDataCache + nNetEnd, pData, nDataLength);
			netDataCacheLength += nDataLength;
			nNetEnd += nDataLength;
		}
		else
		{//ʣ��ռ䲻������Ҫ��ʣ���buffer��ǰ�ƶ�
			if (netDataCacheLength > 0)
			{//���������ʣ��
				memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
				nNetStart = 0;
				nNetEnd = netDataCacheLength;

				if (MaxNetDataCacheCount - nNetEnd < nDataLength)
				{
					nNetStart = nNetEnd = netDataCacheLength = 0;
					WriteLog(Log_Debug, "CNetGB28181RtpServer = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return 0;
				}
			}
			else
			{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
				nNetStart = nNetEnd = netDataCacheLength = 0;
 			}
			memcpy(netDataCache + nNetEnd, pData, nDataLength);
			netDataCacheLength += nDataLength;
			nNetEnd += nDataLength;
		}
 	}

	 return 0;
}

int CNetGB28181RtpServer::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(netDataLock);
	if(!bRunFlag.load())
		return -1 ;

	if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x34)
	{//jt1078
		if (addThreadPoolFlag == false && strlen(m_openRtpServerStruct.send_app) > 0 && strlen(m_openRtpServerStruct.send_stream_id) > 0 )
		{
			char szMediaSource[string_length_1024] = { 0 };
			sprintf(szMediaSource, "/%s/%s", m_openRtpServerStruct.send_app, m_openRtpServerStruct.send_stream_id);
			auto pSendMediaSource = GetMediaStreamSource(szMediaSource);
			if (pSendMediaSource)
			{
				addThreadPoolFlag = true;
				pSendMediaSource->AddClientToMap(nClient);
			}
		}

		if(strcmp(m_openRtpServerStruct.jtt1078_version,"2013") == 0 || strcmp(m_openRtpServerStruct.jtt1078_version, "2016") == 0)
		  SplitterJt1078CacheBuffer();
		else if(strcmp(m_openRtpServerStruct.jtt1078_version, "2019") == 0)
		  SplitterJt1078CacheBuffer2019();

		return 0;
	}

	//���ݵ���Ŵ����ظ���rtp���� 
	CreateSendRtpByPS();

	unsigned char* pData = NULL;
	int            nLength;

	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{//UDP
		pData = NetDataFifo.pop(&nLength);
		if (pData != NULL)
		{
			if (nLength > 0)
			{
				//����rtpͷ�����payload���н��,��Ч��ֹ�û���д��
				if (hRtpHandle == 0)
				{
					rtpHeadPtr = (_rtp_header*)pData ;
					m_gbPayload = rtpHeadPtr->payload;
				}

				RtpDepacket(pData, nLength);
			}

			NetDataFifo.pop_front();
		}

		//��������rtcp ��
		if (GetTickCount64() - nSendRtcpTime >= 5 * 1000 && psBeiJingLaoChenMuxer == NULL ) // psBeiJingLaoChenMuxer == NULL ʱ ��û�лظ� /send_app/send_stream 
		{
			nSendRtcpTime = GetTickCount64();

			memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
			rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
			rtcpRR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

			XHNetSDK_Sendto(nClientRtcp, szRtcpSRBuffer, rtcpSRBufferLength, pRtpAddress);
 		}
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
	{//TCP ��ʽ��rtp����ȡ 
		while (netDataCacheLength > 2048)
		{//���ܻ���̫��buffer,������ɽ��չ�������ֻ����Ƶ��ʱ���������ʱ�ܴ� 2048 �ȽϺ��� 
			memcpy(rtpHeadOfTCP, netDataCache + nNetStart, 2);
			if ((rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x00) || (rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x01) || (rtpHeadOfTCP[0] == 0x24 ))
			{
			    nNetStart += 2;
				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2; 
				netDataCacheLength -= 4;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 2;//4���ֽڷ�ʽ
			}
			else
			{
 				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2;
			    netDataCacheLength -= 2;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 1;//2���ֽڷ�ʽ��
			}

			//��ȡrtp���İ�ͷ
			rtpHeadPtr = (_rtp_header*)(netDataCache + nNetStart);

			nRtpLength = ntohs(nRtpLength);
			if (nRtpLength > 65535 )
			{
				WriteLog(Log_Debug, "CNetGB28181RtpServer = %X rtp��ͷ��������  nClient = %llu ,nRtpLength = %llu", this, nClient, nRtpLength);
 				pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
				return -1;
			}

			//���ȺϷ� ������rtp�� ��rtpHeadPtr->v == 2) ,��ֹrtcp����ִ��rtp���
			if (nRtpLength > 0 && rtpHeadPtr->v == 2)
			{
				//����rtpͷ�����payload���н��,��Ч��ֹ�û���д��
				if (hRtpHandle == 0)
					m_gbPayload = rtpHeadPtr->payload;

				//��10000���굥�˿ڽ��붨��url���� 
				if (netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream && strlen(m_szShareMediaURL) == 0)
				{
					strcpy(m_addStreamProxyStruct.app, "rtp");
					sprintf(m_addStreamProxyStruct.stream, "%X", ntohl(rtpHeadPtr->ssrc));
					sprintf(m_szShareMediaURL, "/rtp/%X", ntohl(rtpHeadPtr->ssrc));
				}

				if (nRecvRtpPacketCount < 1000 )
				{
					nRecvRtpPacketCount ++ ;
					if (nRtpLength > nMaxRtpLength)
					{//��¼rtp ����󳤶� 
						nMaxRtpLength = nRtpLength;
						WriteLog(Log_Debug, "CNetGB28181RtpServer = %X  nClient = %llu , ��ȡ��rtp��󳤶� , nMaxRtpLength = %d ", this, nClient, nMaxRtpLength);
					}
				}
 
				//rtp �����������Ž��н��
				if(nRtpLength <= nMaxRtpLength || nRtpLength < 1500 )
				  RtpDepacket(netDataCache + nNetStart, nRtpLength);
				else
				{//rtp �������쳣
					WriteLog(Log_Debug, "CNetGB28181RtpServer = %X rtp��ͷ��������  nClient = %llu ,nRtpLength = %llu , nMaxRtpLength = %d ", this, nClient, nRtpLength, nMaxRtpLength);
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
					return -1;
				}
 			}

			nNetStart += nRtpLength;
			netDataCacheLength -= nRtpLength;
		}

		//��������rtcp ��
		if (GetTickCount64() - nSendRtcpTime >= 5 * 1000 && psBeiJingLaoChenMuxer == NULL && netBaseNetType != NetBaseNetType_GB28181TcpPSInputStream)  // psBeiJingLaoChenMuxer == NULL ʱ ��û�лظ� /send_app/send_stream 
		{
			nSendRtcpTime = GetTickCount64();

			memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
			rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
			rtcpRR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

			ProcessRtcpData(szRtcpSRBuffer, rtcpSRBufferLength, 1);
		}
 	}
 	return 0;
}

//rtp ���
struct ps_demuxer_notify_t notify_NetGB28181RtpServer = { mpeg_ps_dec_testonstream,};

bool  CNetGB28181RtpServer::RtpDepacket(unsigned char* pData, int nDataLength) 
{
	if (pData == NULL || nDataLength >= 65536 || !bRunFlag.load() || nDataLength < 12 )
		return false;

	//����rtp���
	if (hRtpHandle == 0)
	{
		rtp_depacket_start(GB28181_rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle);

		if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x31)
		{//rtp + PS
		  rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_gbps);
 		}
		else if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x32)
		{//rtp + ES
			if(m_gbPayload == 98)
			  rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_h264);
			else if(m_gbPayload == 99)
			  rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_h265);
		}
		else if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x33)
		{//rtp + xhb
			rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_xhb);
		}
 
 		WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ,����rtp��� hRtpHandle = %d ,psDeMuxHandle = %d", this, hRtpHandle, psDeMuxHandle);
	}

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//����
		if(psDeMuxHandle == 0)
		  ps_demux_start(GB28181_RtpRecv_demux_callback, (void*)this, e_ps_dux_timestamp, &psDeMuxHandle);
	}
	else
	{//�����ϳ�
		if (psBeiJingLaoChen == NULL)
		{
			psBeiJingLaoChen = ps_demuxer_create(on_gb28181_unpacket, this);
			if(psBeiJingLaoChen != NULL )
		      ps_demuxer_set_notify(psBeiJingLaoChen, &notify_NetGB28181RtpServer, this);
		}
	}

	if (hRtpHandle > 0 )
	{
		if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x31 || m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x32)
		{//rtp + PS �� rtp + ES 
#ifdef  WriteRtpTimestamp
			if (!bCheckRtpTimestamp)
			{
				if (nDataLength > 0 && nDataLength < (16 * 1024))
				{
					memcpy(szRtpDataArray[nRptDataArrayOrder], pData, nDataLength);
					nRtpDataArrayLength[nRptDataArrayOrder] = nDataLength;
				}
				nRptDataArrayOrder ++;
				if (nRptDataArrayOrder >= 3)
				{//��3֡
					bCheckRtpTimestamp = true;
					unsigned char szZeroTimestamp[4] = { 0 };
					if (memcmp(szRtpDataArray[0] + 4, szZeroTimestamp, 4) == 0 && memcmp(szRtpDataArray[1] + 4, szZeroTimestamp, 4) == 0 && memcmp(szRtpDataArray[2] + 4, szZeroTimestamp, 4) == 0)
					{//ʱ���Ϊ0 
						nRtpTimestampType = 1;

						for (int i = 0; i < 3; i++)
						{//��ԭ�������rtp�����н��
 							writeRtpHead = (_rtp_header*)szRtpDataArray[i];
							nWriteTimeStamp = nStartTimestap;
							nWriteTimeStamp = htonl(nWriteTimeStamp);
							memcpy(szRtpDataArray[i] + 4, (char*)&nWriteTimeStamp, sizeof(nWriteTimeStamp));
							if (writeRtpHead->mark == 1)
								nStartTimestap += 3600;
							rtp_depacket_input(hRtpHandle, szRtpDataArray[i], nRtpDataArrayLength[i]);
						}
					}
					else
					{//������ʱ���
						nRtpTimestampType = 2;
 						for (int i = 0; i < 3; i++)//��ԭ�������rtp�����н��
 							rtp_depacket_input(hRtpHandle, szRtpDataArray[i], nRtpDataArrayLength[i]);
  					}
 					return true ; //�Ѿ��ѻ����3֡�������
  				}
			}
 
			if (nRtpTimestampType == 1)
			{
			   writeRtpHead = (_rtp_header*)pData;
			   nWriteTimeStamp = nStartTimestap;
			   nWriteTimeStamp = htonl(nWriteTimeStamp);
			   memcpy(pData + 4, (char*)&nWriteTimeStamp, sizeof(nWriteTimeStamp));
 			   if(writeRtpHead->mark == 1)
			      nStartTimestap += 3600;
			   if(nStartTimestap >= 0xFFFFFFFF - (3600 * 25))
				   nStartTimestap = 3600 ; //ʱ�����ת
  			}
			if (nRtpTimestampType >= 1)
				rtp_depacket_input(hRtpHandle, pData, nDataLength);
#else
			rtp_depacket_input(hRtpHandle, pData, nDataLength);
#endif
		}
		else if (m_addStreamProxyStruct.RtpPayloadDataType[0] == 0x33)
		{// rtp + XHB  ��rtp ��ͷ����չ 
 		   rtpHeaderXHB = (_rtp_header*)pData;
		   if (rtpHeaderXHB->x == 1 && nDataLength > 16 )
		   {
			   memcpy((unsigned char*)&rtpExDataLength, pData + 12 + 2, sizeof(rtpExDataLength));
			   rtpExDataLength = ntohs(rtpExDataLength) * 4 ;

			   if (nDataLength - (12 + 4 + rtpExDataLength ) > 0  )
			   {
			     pData[0] = 0x80;
			     memmove(pData + 12, pData + (12 + 4 + rtpExDataLength), nDataLength - (12 + 4 + rtpExDataLength));
			     rtp_depacket_input(hRtpHandle, pData, nDataLength - ( 4 + rtpExDataLength) );
			   }
		   }else
			   rtp_depacket_input(hRtpHandle, pData, nDataLength);
		}
  	}

	return true;
}

//TCP��ʽ����rtcp��
void  CNetGB28181RtpServer::ProcessRtcpData(unsigned char* szRtcpData, int nDataLength, int nChan)
{
	if ( !(netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active) || !bRunFlag.load())
		return;

	if (nRtpRtcpPacketType == 1)
	{//2�ֽڷ�ʽ
		unsigned short nRtpLen = htons(nDataLength);
		memcpy(szRtcpDataOverTCP, (unsigned char*)&nRtpLen, sizeof(nRtpLen));

		memcpy(szRtcpDataOverTCP + 2, szRtcpData, nDataLength);
		XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 2 , ABL_MediaServerPort.nSyncWritePacket);
	}
	else if (nRtpRtcpPacketType == 2)
	{///4�ֽڷ�ʽ
	  szRtcpDataOverTCP[0] = '$';
	  szRtcpDataOverTCP[1] = nChan;
	  unsigned short nRtpLen = htons(nDataLength);
	  memcpy(szRtcpDataOverTCP + 2, (unsigned char*)&nRtpLen, sizeof(nRtpLen));

	  memcpy(szRtcpDataOverTCP + 4, szRtcpData, nDataLength);
	  XHNetSDK_Write(nClient, szRtcpDataOverTCP, nDataLength + 4, ABL_MediaServerPort.nSyncWritePacket);
	}
}

//rtp����ص���Ƶ
void GB28181RtpServer_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetGB28181RtpServer* pThis = (CNetGB28181RtpServer*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag.load())
		return;

	if (pThis->netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP)
	{//udp ֱ�ӷ��� 
	    pThis->SendGBRtpPacketUDP(cb->data, cb->datasize);
	}
	else if (pThis->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || pThis->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || pThis->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
	{//TCP ��Ҫƴ�� ��ͷ
		pThis->GB28181SentRtpVideoData(cb->data, cb->datasize);
	}
}

//�����ظ�rtp\ps 
void  CNetGB28181RtpServer::CreateSendRtpByPS()
{
	if (strlen(m_openRtpServerStruct.send_app) > 0 && strlen(m_openRtpServerStruct.send_stream_id) > 0 && addThreadPoolFlag == false  )
	{//��֤�����̳߳�ֻ��һ��
		if (m_openRtpServerStruct.detectSendAppStream[0] == 0x30)
		{//openRtpServer �����
			if (GetTickCount64() - nAddSend_app_streamDatetime > 1000 )
			{
				nAddSend_app_streamDatetime = GetTickCount64(); 

				char szMediaSource[string_length_1024] = { 0 };
				sprintf(szMediaSource, "/%s/%s", m_openRtpServerStruct.send_app, m_openRtpServerStruct.send_stream_id);
				auto pMediaSource = GetMediaStreamSource(szMediaSource);

				if (pMediaSource != NULL)
				{//�ѿͻ��� ����Դ��ý�忽������
					if ((GetTickCount64() - pMediaSource->tsCreateTime > 3000) || (strlen(pMediaSource->m_mediaCodecInfo.szVideoName) > 0 && strlen(pMediaSource->m_mediaCodecInfo.szAudioName) > 0) )
					{
					   addThreadPoolFlag = true;
					   pMediaSource->AddClientToMap(nClient);
					   WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ���� nClient = %llu, �ظ����� %s �ɹ� ", this, nClient, szMediaSource);
 					}
				}else 
					WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ��nClient = %llu, ���� %s ��δ���룬�뾡�������ܻظ��ɹ� ", this, nClient, szMediaSource);
			} 
		}
		else
		{//����ʱ�Ѿ����
			char szMediaSource[string_length_1024] = { 0 };
			sprintf(szMediaSource, "/%s/%s", m_openRtpServerStruct.send_app, m_openRtpServerStruct.send_stream_id);
			auto pMediaSource = GetMediaStreamSource(szMediaSource);

			if (pMediaSource != NULL)
			{//�ѿͻ��� ����Դ��ý�忽������
				if (GetTickCount64() - pMediaSource->nCreateDateTime > 500 && (strlen(pMediaSource->m_mediaCodecInfo.szVideoName) > 0 || strlen(pMediaSource->m_mediaCodecInfo.szAudioName) > 0))
				{
					addThreadPoolFlag = true;
					pMediaSource->AddClientToMap(nClient);
					WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ���� nClient = %llu, �ظ����� %s �ɹ� ", this, nClient, szMediaSource);
				}
			}
			else
				WriteLog(Log_Debug, "CNetGB28181RtpServer = %X ��nClient = %llu, ���� %s ��δ���룬�뾡�������ܻظ��ɹ� ", this, nClient, szMediaSource);
		}
	}
 
	if (strlen(m_openRtpServerStruct.send_app) > 0 && strlen(m_openRtpServerStruct.send_stream_id) > 0 &&  mediaCodecInfo.nVideoFrameRate > 0 && hRtpPS == 0 )
	{
		if (strlen(mediaCodecInfo.szVideoName) == 0 )
			nMaxRtpSendVideoMediaBufferLength = 640;
		else
			nMaxRtpSendVideoMediaBufferLength = MaxRtpSendVideoMediaBufferLength;
 
		//����rtp 
		int nRet = rtp_packet_start(GB28181RtpServer_rtp_packet_callback_func_send, (void*)this, &hRtpPS);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ��������Ƶrtp���ʧ��,nClient = %llu,  nRet = %d", this, nClient, nRet);
			return;
		}
		optionPS.handle = hRtpPS;

		if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x31)
		{
			optionPS.mediatype = e_rtppkt_mt_video;
			optionPS.payload = 96;
			optionPS.streamtype = e_rtppkt_st_gb28181;
			optionPS.ssrc = rand();
			optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);

			rtp_packet_setsessionopt(&optionPS);
			inputPS.handle = hRtpPS;
			inputPS.ssrc = optionPS.ssrc;

			//����PS
			s_buffer = new  char[IDRFrameMaxBufferLength];
			psBeiJingLaoChenMuxer = ps_muxer_create(&handler, this);

			if (nVideoStreamID == -1 && psBeiJingLaoChenMuxer != NULL && m_openRtpServerStruct.send_disableVideo[0] == 0x30)
			{//������Ƶ 
				if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
					nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChenMuxer, PSI_STREAM_H264, NULL, 0);
				else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
					nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChenMuxer, PSI_STREAM_H265, NULL, 0);
			}

			if (nAudioStreamID == -1 && psBeiJingLaoChenMuxer != NULL && strlen(mediaCodecInfo.szAudioName) > 0 && m_openRtpServerStruct.send_disableAudio[0] == 0x30)
			{//������Ƶ
				if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
					nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChenMuxer, PSI_STREAM_AAC, NULL, 0);
				else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
					nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChenMuxer, PSI_STREAM_AUDIO_G711A, NULL, 0);
				else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
					nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChenMuxer, PSI_STREAM_AUDIO_G711U, NULL, 0);
			}
		}//if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x31)
		else if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x32)
		{
			if (atoi(m_openRtpServerStruct.send_disableAudio) == 1)
			{//ֻ����Ƶ
				optionPS.mediatype = e_rtppkt_mt_video;
				if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				{
					strcpy(m_openRtpServerStruct.payload, "98");
					optionPS.payload = 98;
					optionPS.streamtype = e_rtppkt_st_h264;
				}
				else  if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				{
					strcpy(m_openRtpServerStruct.payload, "99");
					optionPS.payload = 99;
					optionPS.streamtype = e_rtppkt_st_h265;
				}
				optionPS.ssrc = rand();
				optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
			}
			else if (atoi(m_openRtpServerStruct.send_disableVideo) == 1)
			{//ֻ����Ƶ 
				optionPS.mediatype = e_rtppkt_mt_audio;
				optionPS.ssrc = rand();
				if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
				{
					optionPS.ttincre = 320;
					optionPS.streamtype = e_rtppkt_st_g711a;
					optionPS.payload = 8;
				}
				else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
				{
					optionPS.ttincre = 320;
					optionPS.streamtype = e_rtppkt_st_g711u;
					optionPS.payload = 0;
				}
				else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
				{
					optionPS.ttincre = 1024;
					optionPS.streamtype = e_rtppkt_st_aac_have_adts;
					optionPS.payload = 97;
				}
			}

			rtp_packet_setsessionopt(&optionPS);
			inputPS.handle = hRtpPS;
			inputPS.ssrc = optionPS.ssrc;
		}//else if (m_openRtpServerStruct.RtpPayloadDataType[0] == 0x32)
 	}
}

//PS ���ݴ����rtp 
void  CNetGB28181RtpServer::GB28181PsToRtPacket(unsigned char* pPsData, int nLength)
{
	if (hRtpPS > 0 && bRunFlag.load())
	{
		inputPS.data = pPsData;
		inputPS.datasize = nLength;
		rtp_packet_input(&inputPS);
	}
}

//udp��ʽ����rtp��
void  CNetGB28181RtpServer::SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength)
{
	if(pSrcAddress != NULL && bRunFlag.load() && pRtpData != NULL && nLength > 0 )
	  XHNetSDK_Sendto(nClient, pRtpData, nLength, pSrcAddress);
}

//����28181PS����TCP��ʽ���� 
void  CNetGB28181RtpServer::GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength)
{
	if (bRunFlag.load() == false  )
		return;

	if ((nMaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0)
	{//ʣ��ռ䲻���洢 ,��ֹ���� 
		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, ABL_MediaServerPort.nSyncWritePacket);
		if (nSendRet != 0)
		{
			bRunFlag.exchange(false);
 			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, ���͹���RTP�������� ��Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nStartVideoTimestamp != GB28181VideoStartTimestampFlag &&  nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//����һ֡�µ���Ƶ 
		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, ABL_MediaServerPort.nSyncWritePacket);
		if (nSendRet != 0)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, ���͹���RTP�������� ��Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			bRunFlag.exchange(false);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	if (ABL_MediaServerPort.nGBRtpTCPHeadType == 1)
	{//���� TCP���� 4���ֽڷ�ʽ
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 0] = '$';
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 1] = 0;
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 4), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
		nSendRtpVideoMediaBufferLength += nDataLength + 4;
	}
	else if (ABL_MediaServerPort.nGBRtpTCPHeadType == 2)
	{//���� TCP���� 2 ���ֽڷ�ʽ
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + nSendRtpVideoMediaBufferLength, (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
		nSendRtpVideoMediaBufferLength += nDataLength + 2;
	}
	else
	{
		bRunFlag.exchange(false);
		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, �Ƿ��Ĺ���TCP��ͷ���ͷ�ʽ(����Ϊ 1��2 )nGBRtpTCPHeadType = %d ", this, ABL_MediaServerPort.nGBRtpTCPHeadType);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
	}
}

//׷��adts��Ϣͷ
void  CNetGB28181RtpServer::AddADTSHeadToAAC(unsigned char* szData, int nAACLength)
{
	aacDataLength  = nAACLength + 7;
	uint8_t profile = 2;
	uint8_t channel_configuration = mediaCodecInfo.nChannels;
	aacData[0] = 0xFF; /* 12-syncword */
	aacData[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
	aacData[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
	aacData[3] = ((channel_configuration & 0x03) << 6) | ((aacDataLength >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
	aacData[4] = (uint8_t)(aacDataLength >> 3);
	aacData[5] = ((aacDataLength & 0x07) << 5) | 0x1F;
	aacData[6] = 0xFC | ((aacDataLength / 1024) & 0x03);

	memcpy(aacData + 7, szData, nAACLength);
}

//���͵�һ������
int CNetGB28181RtpServer::SendFirstRequst()
{
	if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active)
	{//�ظ�http�������ӳɹ���
		sprintf(szResponseBody, "{\"code\":0,\"port\":%d,\"memo\":\"success\",\"key\":%llu}", nReturnPort, nClient);
		ResponseHttp(nClient_http, szResponseBody, false);
	}
	return 0;
}

//����m3u8�ļ�
bool  CNetGB28181RtpServer::RequestM3u8File()
{
	return true;
}

//����1078��Ƶ���� 
void  CNetGB28181RtpServer::SendJtt1078VideoPacket(unsigned char* pData,int nLength)
{
	if (pData != NULL && nLength > 0)
	{
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			jt1078VideoHead.pt = 98;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			jt1078VideoHead.pt = 99;
		else
		{
 			return;
		}

		//ȷ��֡���� 
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, pData, nLength))
		{
			jt1078VideoHead.frame_type = 0x00;
			nPFrameCount = 1;
		}
		else
		{
			jt1078VideoHead.frame_type = 0x01;
			nPFrameCount++;
		}

		//�����ܰ����� 
		pPacketCount = nLength / JTT1078_MaxPacketLength;
		if (nLength % JTT1078_MaxPacketLength != 0)
			pPacketCount += 1;

		jt1078SendPacketLenth = 0;
		nPacketOrder = 0;
		nSrcVideoPos = 0;
		while (nLength > 0)
		{
			nPacketOrder++;

			//�ܳ���С�ڵ���950 
			if (pPacketCount == 1)
				jt1078VideoHead.packet_type = 0;
			else
			{//��� 
				if (nPacketOrder == 1)
					jt1078VideoHead.packet_type = 1;//��һ��
				else if (nPacketOrder > 1 && nPacketOrder < pPacketCount)
					jt1078VideoHead.packet_type = 3;//�м��
				else
					jt1078VideoHead.packet_type = 2;//����
			}

			//����ʱ���� 
			jt1078VideoHead.frame_interval = htons(1000 / mediaCodecInfo.nVideoFrameRate);
			jt1078VideoHead.i_frame_interval = htons(nPFrameCount * (1000 / mediaCodecInfo.nVideoFrameRate));
 
			if (nLength >= JTT1078_MaxPacketLength)
			{
				jt1078VideoHead.payload_size = htons(JTT1078_MaxPacketLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead, sizeof(jt1078VideoHead));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, JTT1078_MaxPacketLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, JTT1078_MaxPacketLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += JTT1078_MaxPacketLength;
				nSrcVideoPos += JTT1078_MaxPacketLength;

				nLength -= JTT1078_MaxPacketLength;
			}
			else
			{
				jt1078VideoHead.payload_size = htons(nLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead, sizeof(jt1078VideoHead));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, nLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, nLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += nLength;
				nSrcVideoPos += nLength;

				nLength = 0;
			}

			if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP && pRtpAddress != NULL)
			{//udp 
				XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, (void*)pRtpAddress);
				jt1078SendPacketLenth = 0;
			}
			else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
			{//tcp 
				if ((jt1078SendPacketLenth > (MaxGB28181RtpSendVideoMediaBufferLength - 4096)) || nLength == 0)
				{
					XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, ABL_MediaServerPort.nSyncWritePacket);

					jt1078SendPacketLenth = 0;
				}
			}

			jt1078VideoHead.seq++;
			if (jt1078VideoHead.seq >= 0xFFFF)
				jt1078VideoHead.seq = 0;
		}
	}
}
//����1078��Ƶ���� 
void  CNetGB28181RtpServer::SendJtt1078VideoPacket2019(unsigned char* pData, int nLength)
{
	if ( pData != NULL && nLength > 0 )
	{
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			jt1078VideoHead2019.pt = 98;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			jt1078VideoHead2019.pt = 99;
		else
		{
 			return;
		}

		//ȷ��֡���� 
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, pData, nLength))
		{
			jt1078VideoHead2019.frame_type = 0x00;
			nPFrameCount = 1;
		}
		else
		{
			jt1078VideoHead2019.frame_type = 0x01;
			nPFrameCount++;
		}

		//�����ܰ����� 
		pPacketCount = nLength / JTT1078_MaxPacketLength;
		if (nLength % JTT1078_MaxPacketLength != 0)
			pPacketCount += 1;

		jt1078SendPacketLenth = 0;
		nPacketOrder = 0;
		nSrcVideoPos = 0;
		while (nLength > 0)
		{
			nPacketOrder++;

			//�ܳ���С�ڵ���950 
			if (pPacketCount == 1)
				jt1078VideoHead2019.packet_type = 0;
			else
			{//��� 
				if (nPacketOrder == 1)
					jt1078VideoHead2019.packet_type = 1;//��һ��
				else if (nPacketOrder > 1 && nPacketOrder < pPacketCount)
					jt1078VideoHead2019.packet_type = 3;//�м��
				else
					jt1078VideoHead2019.packet_type = 2;//����
			}

			//����ʱ���� 
			jt1078VideoHead2019.frame_interval = htons(1000 / mediaCodecInfo.nVideoFrameRate);
			jt1078VideoHead2019.i_frame_interval = htons(nPFrameCount * (1000 / mediaCodecInfo.nVideoFrameRate));

			if (nLength >= JTT1078_MaxPacketLength)
			{
				jt1078VideoHead2019.payload_size = htons(JTT1078_MaxPacketLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead2019, sizeof(jt1078VideoHead2019));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead2019);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, JTT1078_MaxPacketLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, JTT1078_MaxPacketLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += JTT1078_MaxPacketLength;
				nSrcVideoPos += JTT1078_MaxPacketLength;

				nLength -= JTT1078_MaxPacketLength;
			}
			else
			{
				jt1078VideoHead2019.payload_size = htons(nLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead2019, sizeof(jt1078VideoHead2019));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead2019);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, nLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, nLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += nLength;
				nSrcVideoPos += nLength;

				nLength = 0;
			}

			if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP &&  pRtpAddress != NULL )
			{//udp 
				XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, (void*)pRtpAddress);
				jt1078SendPacketLenth = 0;
			}
			else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
			{//tcp 
				if ((jt1078SendPacketLenth > (MaxGB28181RtpSendVideoMediaBufferLength - 4096)) || nLength == 0)
				{
					XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, ABL_MediaServerPort.nSyncWritePacket);

					jt1078SendPacketLenth = 0;
				}
			}

			jt1078VideoHead2019.seq++;
			if (jt1078VideoHead2019.seq >= 0xFFFF)
				jt1078VideoHead2019.seq = 0;
		}
	}
}

//����1078��Ƶ���� 
void  CNetGB28181RtpServer::SendJtt1078AduioPacket(unsigned char* pData, int nLength)
{


	if ( pData != NULL && nLength > 0 )
	{
		if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			jt1078AudioHead.pt = 6;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			jt1078AudioHead.pt = 7;
		else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			jt1078AudioHead.pt = 19;
		else
		{
 			return;
		}

		jt1078AudioHead.frame_type = 0x03;
		jt1078AudioHead.packet_type = 0x00;

		jt1078AudioHead.payload_size = htons(nLength);

		memcpy(szSendRtpVideoMediaBuffer, (char*)&jt1078AudioHead, sizeof(jt1078AudioHead));
		memcpy(szSendRtpVideoMediaBuffer + sizeof(jt1078AudioHead), pData, nLength);

		if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP && pRtpAddress != NULL )
		{//udp 
			XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead), (void*)pRtpAddress);
		}
		else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
		{//tcp 
			XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead), ABL_MediaServerPort.nSyncWritePacket);
		}

		jt1078AudioHead.seq++;
		if (jt1078AudioHead.seq >= 0xFFFF)
			jt1078AudioHead.seq = 0;
	}
}
//����1078��Ƶ���� 
void  CNetGB28181RtpServer::SendJtt1078AduioPacket2019(unsigned char* pData, int nLength)
{
	if ( pData != NULL && nLength > 0 )
	{
		if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			jt1078AudioHead2019.pt = 6;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			jt1078AudioHead2019.pt = 7;
		else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			jt1078AudioHead2019.pt = 19;
		else
		{
 			return;
		}

		jt1078AudioHead2019.frame_type = 0x03;
		jt1078AudioHead2019.packet_type = 0x00;

		jt1078AudioHead2019.payload_size = htons(nLength);

		memcpy(szSendRtpVideoMediaBuffer, (char*)&jt1078AudioHead2019, sizeof(jt1078AudioHead2019));
		memcpy(szSendRtpVideoMediaBuffer + sizeof(jt1078AudioHead2019), pData, nLength);

		if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP && pRtpAddress != NULL )
		{//udp 
			XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead2019), (void*)pRtpAddress);
		}
		else if (netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
		{//tcp 
			XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead2019), ABL_MediaServerPort.nSyncWritePacket);
		}

		jt1078AudioHead2019.seq++;
		if (jt1078AudioHead2019.seq >= 0xFFFF)
			jt1078AudioHead2019.seq = 0;

	}
}
