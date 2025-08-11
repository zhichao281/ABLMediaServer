/*
���ܣ�
    ����TS�����γ�ý��Դ�����ڽ�������ts���ļ�
 	 
����    2022-02-16
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerRecvRtpTS_PS.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern void LIBNET_CALLMETHOD                onread(NETHANDLE srvhandle, NETHANDLE clihandle, uint8_t* data, uint32_t datasize, void* address);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern void LIBNET_CALLMETHOD                onread(NETHANDLE srvhandle, NETHANDLE clihandle, uint8_t* data, uint32_t datasize, void* address);

#endif

CNetServerRecvRtpTS_PS::CNetServerRecvRtpTS_PS(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	netBaseNetType = NetBaseNetType_NetGB28181RecvRtpPS_TS;
 	int nRet = XHNetSDK_BuildUdp(NULL,ABL_MediaServerPort.ps_tsRecvPort, NULL, &nClient, onread, 1);
	nClientPort = ABL_MediaServerPort.ps_tsRecvPort ;
	WriteLog(Log_Debug, (nRet == 0) ? "�󶨶˿� [ udp ] %d �ɹ�(success) " : "�󶨶˿� %d(udp) ʧ��(fail) ", ABL_MediaServerPort.ps_tsRecvPort);
	WriteLog(Log_Debug, "CNetServerRecvRtpTS_PS ���� = %X  nClient = %llu ,nRet = %d ", this, nClient,nRet);
}

CNetServerRecvRtpTS_PS::~CNetServerRecvRtpTS_PS()
{
 	XHNetSDK_DestoryUdp(nClient);
	WriteLog(Log_Debug, "CNetServerRecvRtpTS_PS ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetServerRecvRtpTS_PS::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	return 0;
}

int CNetServerRecvRtpTS_PS::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}

int CNetServerRecvRtpTS_PS::SendVideo()
{
	return 0;
}

int CNetServerRecvRtpTS_PS::SendAudio()
{

	return 0;
}

int CNetServerRecvRtpTS_PS::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (nDataLength <= 12 || pData == NULL  )
		return -1;//�Ƿ�����

	rtpHeadPtr = (_rtp_header*)pData;
	rtpClient = BaseRecvRtpSSRCNumber + rtpHeadPtr->ssrc;
	if (rtpHeadPtr->v != 2)
		return -1;//����rtp��

 	auto  rtpClientPtr = GetNetRevcBaseClient(rtpClient);
	if (rtpClientPtr == NULL && address != NULL )
	{
		char szTemp[256] = { 0 };
		char szRtpSource[256] = { 0 };
		sprintf(szTemp, "/rtp/%X",ntohl(rtpHeadPtr->ssrc));
		sprintf(szRtpSource, "rtp://%s:%d", inet_ntoa(((sockaddr_in*)address)->sin_addr), ntohs(((sockaddr_in*)address)->sin_port));
		strcat(szRtpSource, szTemp);

		if(memcmp(pData + sizeof(_rtp_header), psHeadFlag, 4) == 0)
		{//PS 
		   rtpClientPtr = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181UDPPSStreamInput, 0, rtpClient, inet_ntoa(((sockaddr_in*)address)->sin_addr), ntohs(((sockaddr_in*)address)->sin_port), szTemp);
		}
		else
		{//TS
			rtpClientPtr = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181UDPTSStreamInput, 0, rtpClient, inet_ntoa(((sockaddr_in*)address)->sin_addr), ntohs(((sockaddr_in*)address)->sin_port), szTemp);
		}
	}

	if (rtpClientPtr != NULL)
	{
		if (rtpClientPtr->nSSRC == 0)
			rtpClientPtr->nSSRC = rtpHeadPtr->ssrc; //Ĭ�ϵ�һ��ssrc 
		if (rtpClientPtr->nSSRC == rtpHeadPtr->ssrc)
 			rtpClientPtr->InputNetData(nServerHandle, nClientHandle, pData, nDataLength,address);
	}

    return 0;
}

int CNetServerRecvRtpTS_PS::ProcessNetData()
{
 	return 0;
}

//���͵�һ������
int CNetServerRecvRtpTS_PS::SendFirstRequst()
{
  	 return 0;
}

//����m3u8�ļ�
bool  CNetServerRecvRtpTS_PS::RequestM3u8File()
{
	return true;
}