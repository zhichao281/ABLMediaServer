/*
功能：
    接收TS流，形成媒体源，用于接收推送ts流文件
 	 
日期    2022-02-16
作者    罗家兄弟
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


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern void LIBNET_CALLMETHOD                onread(NETHANDLE srvhandle, NETHANDLE clihandle, uint8_t* data, uint32_t datasize, void* address);

#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                       ABL_MediaServerPort;


extern CMediaFifo                            pDisconnectBaseNetFifo; //清理断裂的链接 
extern char                                  ABL_MediaSeverRunPath[256]; //当前路径
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern void LIBNET_CALLMETHOD                onread(NETHANDLE srvhandle, NETHANDLE clihandle, uint8_t* data, uint32_t datasize, void* address);

#endif

CNetServerRecvRtpTS_PS::CNetServerRecvRtpTS_PS(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	netBaseNetType = NetBaseNetType_NetGB28181RecvRtpPS_TS;
 	int nRet = XHNetSDK_BuildUdp(NULL,ABL_MediaServerPort.ps_tsRecvPort, NULL, &nClient, onread, 1);
	nClientPort = ABL_MediaServerPort.ps_tsRecvPort ;
	WriteLog(Log_Debug, (nRet == 0) ? "绑定端口 [udp] %d 成功(success) " : "绑定端口 %d(udp) 失败(fail) ", ABL_MediaServerPort.ps_tsRecvPort);
	WriteLog(Log_Debug, "CNetServerRecvRtpTS_PS 构造 = %X  nClient = %llu ,nRet = %d ", this, nClient,nRet);
}

CNetServerRecvRtpTS_PS::~CNetServerRecvRtpTS_PS()
{
 	XHNetSDK_DestoryUdp(nClient);
	WriteLog(Log_Debug, "CNetServerRecvRtpTS_PS 析构 = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
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
		return -1;//非法数据

	rtpHeadPtr = (_rtp_header*)pData;
	rtpClient = BaseRecvRtpSSRCNumber + rtpHeadPtr->ssrc;
	if (rtpHeadPtr->v != 2)
		return -1;//不是rtp包

 	auto  rtpClientPtr = GetNetRevcBaseClient(rtpClient);
	if (rtpClientPtr == NULL && address != NULL)
	{
		char szTemp[256] = { 0 };
		char szRtpSource[256] = { 0 };
		sprintf(szTemp, "/rtp/%X", rtpHeadPtr->ssrc);
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
			rtpClientPtr->nSSRC = rtpHeadPtr->ssrc; //默认第一个ssrc 
		if (rtpClientPtr->nSSRC == rtpHeadPtr->ssrc)
 			rtpClientPtr->InputNetData(nServerHandle, nClientHandle, pData, nDataLength,address);
	}

    return 0;
}

int CNetServerRecvRtpTS_PS::ProcessNetData()
{
 	return 0;
}

//发送第一个请求
int CNetServerRecvRtpTS_PS::SendFirstRequst()
{
  	 return 0;
}

//请求m3u8文件
bool  CNetServerRecvRtpTS_PS::RequestM3u8File()
{
	return true;
}