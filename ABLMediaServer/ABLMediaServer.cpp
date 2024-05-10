/*
功能：
		高性能流媒体服务器(ABLMediaServer)主框架

		媒体输入方式             支持音视频格式
----------------------------------------------------------------------------------------
				1、rtsp          视频H264、H265，音频AAC、G711A、G711U)
				2、rtmp          视频H264，      音频AAC、G711A、G711U)
				3、国标GB28181   视频H264、H265，音频AAC、G711A、G711U)
				4、WebRTC        视频H264、H265，音频AAC、G711A、G711U)
				5、
		媒体输输出方式
----------------------------------------------------------------------------------------
				1、rtsp          视频H264、H265，音频AAC、G711A、G711U)
				2、rtmp          视频H264、H265，音频AAC、G711A、G711U)
				3、http-flv      视频H264、H265，音频AAC、G711A、G711U)
				4、m3u8          视频H264、H265，音频AAC、G711A、G711U)
				5、fmp4          视频H264、H265，音频AAC、G711A、G711U)
				6、国标GB28181   视频H264、H265，音频AAC、G711A、G711U)
				7、WebRTC        视频H264、H265，音频AAC、G711A、G711U)
				8、ws-flv        视频H264、H265，音频AAC、G711A、G711U)

日期    2021-04-02
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "../webrtc-streamer/inc/rtc_obj_sdk.h"

NETHANDLE srvhandle_8080, srvhandle_554, srvhandle_1935, srvhandle_6088, srvhandle_8088, srvhandle_8089, srvhandle_9088, srvhandle_9298, srvhandle_10000;
#ifdef USE_BOOST
typedef boost::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef boost::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;
CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;
std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
CNetBaseThreadPool* NetBaseThreadPool;
CNetBaseThreadPool* RecordReplayThreadPool;//录像回放线程池
CNetBaseThreadPool* MessageSendThreadPool;//消息发送线程池

/* 媒体数据存储 -------------------------------------------------------------------------------------*/
typedef boost::shared_ptr<CMediaStreamSource>                    CMediaStreamSource_ptr;
typedef boost::unordered_map<string, CMediaStreamSource_ptr>     CMediaStreamSource_ptrMap;
CMediaStreamSource_ptrMap                                        xh_ABLMediaStreamSourceMap;
std::mutex                                                       ABL_CMediaStreamSourceMapLock;

/* 录像文件存储 -------------------------------------------------------------------------------------*/
typedef boost::shared_ptr<CRecordFileSource>                     CRecordFileSource_ptr;
typedef boost::unordered_map<string, CRecordFileSource_ptr>      CRecordFileSource_ptrMap;
CRecordFileSource_ptrMap                                         xh_ABLRecordFileSourceMap;
std::mutex                                                       ABL_CRecordFileSourceMapLock;

/* 图片文件存储 -------------------------------------------------------------------------------------*/
typedef boost::shared_ptr<CPictureFileSource>                    CPictureFileSource_ptr;
typedef boost::unordered_map<string, CPictureFileSource_ptr>     CPictureFileSource_ptrMap;
CPictureFileSource_ptrMap                                        xh_ABLPictureFileSourceMap;
std::mutex                                                       ABL_CPictureFileSourceMapLock;

uint64_t                                                         ArrayAddMutePacketList[8192];//增加静音包的列表
uint64_t                                                         nMaxAddMuteListNumber = 0; //最后一个静音对象所在的序号

volatile bool                                                    ABL_bMediaServerRunFlag = true;
volatile bool                                                    ABL_bExitMediaServerRunFlag = false; //退出处理线程标志 
CMediaFifo                                                       pDisconnectBaseNetFifo;             //清理断裂的链接 
CMediaFifo                                                       pReConnectStreamProxyFifo;          //需要重新连接代理ID 
CMediaFifo                                                       pMessageNoticeFifo;          //消息通知FIFO
CMediaFifo                                                       pNetBaseObjectFifo;          //存储基类对象ID
char                                                             ABL_MediaSeverRunPath[256] = { 0 }; //当前路径
char                                                             ABL_wwwMediaPath[256] = { 0 }; //www 子路径
uint64_t                                                         ABL_nBaseCookieNumber = 100; //Cookie 序号 
char                                                             ABL_szLocalIP[128] = { 0 };
uint64_t                                                         ABL_nPrintCheckNetRevcBaseClientDisconnect = 0;
unsigned int                                                     ABL_nCurrentSystemCpuCount = 4;//当前系统cpu物理核心数 
CNetRevcBase_ptr                                                 GetNetRevcBaseClient(NETHANDLE CltHandle);
bool 	                                                         ABL_bCudaFlag = false;
int                                                              ABL_nCudaCount = 0;
volatile bool                                                    ABL_bRestartServerFlag = false;
volatile bool                                                    ABL_bInitXHNetSDKFlag = false;
volatile bool                                                    ABL_bInitCudaSDKFlag = false;
char                                                             szConfigFileName[512] = { 0 };
CNetRevcBase_ptr                                                 CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
CMediaStreamSource_ptr                                           GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound);

#else

typedef std::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef std::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;
CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;
std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
CNetBaseThreadPool* NetBaseThreadPool;
CNetBaseThreadPool* RecordReplayThreadPool;//录像回放线程池
CNetBaseThreadPool* MessageSendThreadPool;//消息发送线程池

/* 媒体数据存储 -------------------------------------------------------------------------------------*/
typedef std::shared_ptr<CMediaStreamSource>                    CMediaStreamSource_ptr;
typedef std::unordered_map<string, CMediaStreamSource_ptr>     CMediaStreamSource_ptrMap;
CMediaStreamSource_ptrMap                                        xh_ABLMediaStreamSourceMap;
std::mutex                                                       ABL_CMediaStreamSourceMapLock;

/* 录像文件存储 -------------------------------------------------------------------------------------*/
typedef std::shared_ptr<CRecordFileSource>                     CRecordFileSource_ptr;
typedef std::unordered_map<string, CRecordFileSource_ptr>      CRecordFileSource_ptrMap;
CRecordFileSource_ptrMap                                         xh_ABLRecordFileSourceMap;
std::mutex                                                       ABL_CRecordFileSourceMapLock;

/* 图片文件存储 -------------------------------------------------------------------------------------*/
typedef std::shared_ptr<CPictureFileSource>                    CPictureFileSource_ptr;
typedef std::unordered_map<string, CPictureFileSource_ptr>     CPictureFileSource_ptrMap;
CPictureFileSource_ptrMap                                        xh_ABLPictureFileSourceMap;
std::mutex                                                       ABL_CPictureFileSourceMapLock;

uint64_t                                                         ArrayAddMutePacketList[8192];//增加静音包的列表
uint64_t                                                         nMaxAddMuteListNumber = 0; //最后一个静音对象所在的序号

volatile bool                                                    ABL_bMediaServerRunFlag = true;
volatile bool                                                    ABL_bExitMediaServerRunFlag = false; //退出处理线程标志 
CMediaFifo                                                       pDisconnectBaseNetFifo;             //清理断裂的链接 
CMediaFifo                                                       pReConnectStreamProxyFifo;          //需要重新连接代理ID 
CMediaFifo                                                       pMessageNoticeFifo;          //消息通知FIFO
CMediaFifo                                                       pNetBaseObjectFifo;          //存储基类对象ID
char                                                             ABL_MediaSeverRunPath[256] = { 0 }; //当前路径
char                                                             ABL_wwwMediaPath[256] = { 0 }; //www 子路径
uint64_t                                                         ABL_nBaseCookieNumber = 100; //Cookie 序号 
char                                                             ABL_szLocalIP[128] = { 0 };
uint64_t                                                         ABL_nPrintCheckNetRevcBaseClientDisconnect = 0;
unsigned int                                                     ABL_nCurrentSystemCpuCount = 4;//当前系统cpu物理核心数 
CNetRevcBase_ptr                                                 GetNetRevcBaseClient(NETHANDLE CltHandle);
bool 	                                                         ABL_bCudaFlag = false;
int                                                              ABL_nCudaCount = 0;
volatile bool                                                    ABL_bRestartServerFlag = false;
volatile bool                                                    ABL_bInitXHNetSDKFlag = false;
volatile bool                                                    ABL_bInitCudaSDKFlag = false;
char                                                             szConfigFileName[512] = { 0 };
CNetRevcBase_ptr                                                 CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
CMediaStreamSource_ptr                                           GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound);

#endif  //USE_BOOST

#ifdef OS_System_Windows
//cuda 解码 
HINSTANCE            hCudaDecodeInstance;
ABL_cudaDecode_Init cudaCodec_Init = NULL;
ABL_cudaDecode_GetDeviceGetCount  cudaCodec_GetDeviceGetCount = NULL;
ABL_cudaDecode_GetDeviceName cudaCodec_GetDeviceName = NULL;
ABL_cudaDecode_GetDeviceUse cudaCodec_GetDeviceUse = NULL;
ABL_CreateVideoDecode cudaCodec_CreateVideoDecode = NULL;
ABL_CudaVideoDecode cudaCodec_CudaVideoDecode = NULL;
ABL_DeleteVideoDecode cudaCodec_DeleteVideoDecode = NULL;
ABL_GetCudaDecodeCount cudaCodec_GetCudaDecodeCount = NULL;
ABL_VideoDecodeUnInit cudaCodec_UnInit = NULL;

#else
void*              pCudaDecodeHandle = NULL ;
ABL_cudaDecode_Init cudaCodec_Init = NULL ;
ABL_cudaDecode_GetDeviceGetCount  cudaCodec_GetDeviceGetCount  = NULL ;
ABL_cudaDecode_GetDeviceName cudaCodec_GetDeviceName = NULL ;
ABL_cudaDecode_GetDeviceUse cudaCodec_GetDeviceUse = NULL ;
ABL_CreateVideoDecode cudaCodec_CreateVideoDecode = NULL ;
ABL_CudaVideoDecode cudaCodec_CudaVideoDecode  = NULL ;
ABL_DeleteVideoDecode cudaCodec_DeleteVideoDecode = NULL ;
ABL_GetCudaDecodeCount cudaCodec_GetCudaDecodeCount = NULL ;
ABL_VideoDecodeUnInit cudaCodec_UnInit = NULL ;

void*              pCudaEncodeHandle = NULL ;
ABL_cudaEncode_Init cudaEncode_Init = NULL ;
ABL_cudaEncode_GetDeviceGetCount cudaEncode_GetDeviceGetCount  = NULL;
ABL_cudaEncode_GetDeviceName cudaEncode_GetDeviceName  = NULL;
ABL_cudaEncode_CreateVideoEncode cudaEncode_CreateVideoEncode  = NULL;
ABL_cudaEncode_DeleteVideoEncode cudaEncode_DeleteVideoEncode  = NULL;
ABL_cudaEncode_CudaVideoEncode cudaEncode_CudaVideoEncode  = NULL;
ABL_cudaEncode_UnInit cudaEncode_UnInit  = NULL;

#endif

//把需要增加
bool   AddClientToMapAddMutePacketList(uint64_t nClient)
{
	bool bRet = false;

	for (int i = 0; i < 8192; i++)
	{
		if (bRet == false && ArrayAddMutePacketList[i] == 0)
		{
			ArrayAddMutePacketList[i] = nClient;
			bRet = true;
		}

		//查找最大的序号
		if (ArrayAddMutePacketList[i] > 0 && (i + 1) > nMaxAddMuteListNumber)
			nMaxAddMuteListNumber = i + 1;
	}

	return bRet;
}

bool   DelClientToMapFromMutePacketList(uint64_t nClient)
{
	bool bRet = false;
	int  nMaxNumber = 0;
	for (int i = 0; i < nMaxAddMuteListNumber; i++)
	{
		if (bRet == false && ArrayAddMutePacketList[i] == nClient)
		{
			ArrayAddMutePacketList[i] = 0;
			bRet = true;
		}

		//查找最大的序号
		if (ArrayAddMutePacketList[i] > 0 && (i + 1) > nMaxNumber)
			nMaxNumber = i + 1;
	}
	nMaxAddMuteListNumber = nMaxNumber;

	return bRet;
}

uint64_t GetCurrentSecondByTime(char* szDateTime)
{
	if (szDateTime == NULL || strlen(szDateTime) < 14)
		return 0;

	time_t clock;
	struct tm tm;

	sscanf(szDateTime, "%04d%02d%02d%02d%02d%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	tm.tm_year = tm.tm_year - 1900;
	tm.tm_mon = tm.tm_mon - 1;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	return clock;
}

#ifdef OS_System_Windows
CSimpleIniA        ABL_ConfigFile;
uint64_t GetCurrentSecond()
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	return clock;
}

BOOL GBK2UTF8(char* szGbk, char* szUtf8, int Len)
{
	// 先将多字节GBK（CP_ACP或ANSI）转换成宽字符UTF-16  
	// 得到转换后，所需要的内存字符数  
	int n = MultiByteToWideChar(CP_ACP, 0, szGbk, -1, NULL, 0);
	// 字符数乘以 sizeof(WCHAR) 得到字节数  
	WCHAR* str1 = new WCHAR[sizeof(WCHAR) * n];
	// 转换  
	MultiByteToWideChar(CP_ACP,  // MultiByte的代码页Code Page  
		0,            //附加标志，与音标有关  
		szGbk,        // 输入的GBK字符串  
		-1,           // 输入字符串长度，-1表示由函数内部计算  
		str1,         // 输出  
		n             // 输出所需分配的内存  
	);

	// 再将宽字符（UTF-16）转换多字节（UTF-8）  
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	if (n > Len)
	{
		delete[] str1;
		return FALSE;
	}
	int nUTF_8 = WideCharToMultiByte(CP_UTF8, 0, str1, -1, szUtf8, n, NULL, NULL);
	delete[] str1;
	str1 = NULL;

	return TRUE;
}

#else
CSimpleIniA                                                      ABL_ConfigFile;
#endif

MediaServerPort                                                  ABL_MediaServerPort;
int64_t                                                          nTestRtmpPushID;
unsigned short                                                   ABL_nGB28181Port = 10002;

#ifndef OS_System_Windows

int GB2312ToUTF8(char* szSrc, size_t iSrcLen, char* szDst, size_t iDstLen)
{
	iconv_t cd = iconv_open("utf-8//IGNORE", "gb2312//IGNORE");
	if (0 == cd)
		return -2;
	memset(szDst, 0, iDstLen);
	char** src = &szSrc;
	char** dst = &szDst;
	if (-1 == (int)iconv(cd, src, &iSrcLen, dst, &iDstLen))
	{
		iconv_close(cd);
		return -1;
	}
	iconv_close(cd);
	return 0;
}

unsigned long GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

int64_t GetTickCount64()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

uint64_t GetCurrentSecond()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec);
}

//延时
void  Sleep(int mMicroSecond)
{
	if (mMicroSecond > 0)
		usleep(mMicroSecond * 1000);
	else
		usleep(5 * 1000);
}

bool GetLocalAdaptersInfo(string& strIPList)
{
	struct ifaddrs* ifaddr, * ifa;

	int family, s;

	char szAllIPAddress[4096] = { 0 };
	char host[NI_MAXHOST] = { 0 };

	if (getifaddrs(&ifaddr) == -1)
	{ //通过getifaddrs函数得到所有网卡信息
		return false;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{ //做链表做循环

		if (ifa->ifa_addr == NULL) //判断地址是否为空
			continue;

		family = ifa->ifa_addr->sa_family; //得到IP地址的协议族

		if (family == AF_INET)
		{ //判断协议族是AF_INET还是AF_INET6
			memset(host, 0x00, NI_MAXHOST);

			//通过getnameinfo函数得到对应的IP地址。NI_MAXHOST为宏定义，值为1025. NI_NUMERICHOST宏定义，和NI_NUMERICSERV对应，都试一下就知道了。
			s = getnameinfo(ifa->ifa_addr,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
				sizeof(struct sockaddr_in6),
				host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (!(strcmp(host, "127.0.0.1") == 0 || strcmp(host, "0.0.0.0") == 0))
			{
				strcat(szAllIPAddress, host);
				strcat(szAllIPAddress, ",");
			}

			if (s != 0)
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
			}
		}
	}
	WriteLog(Log_Debug, "szAllIPAddress = %s ", szAllIPAddress);
	strIPList = szAllIPAddress;

	return true;
}

#endif

//无锁查找，在外层已经有锁
CNetRevcBase_ptr GetNetRevcBaseClientNoLock(NETHANDLE CltHandle)
{
	
	CNetRevcBase_ptr   pClient = NULL;

	auto iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		pClient = (*iterator1).second;
		return pClient;
	}
	else
	{
		return NULL;
	}
}

CMediaStreamSource_ptr CreateMediaStreamSource(char* szURL, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct)
{
	std::lock_guard<std::mutex> lock(ABL_CMediaStreamSourceMapLock);

	
	CMediaStreamSource_ptr pXHClient = NULL;
	string                 strURL = szURL;
 
	//先查找是否存在，如果存在则返回原来存在的，保证不会空指针造成崩溃
	auto iterator1 = xh_ABLMediaStreamSourceMap.find(szURL);
	if (iterator1 != xh_ABLMediaStreamSourceMap.end())
	{
		pXHClient = (*iterator1).second;
		return pXHClient;
	}

	try
	{
		do
		{
#ifdef USE_BOOST
			pXHClient = boost::make_shared<CMediaStreamSource>(szURL, nClient, nSourceType, nDuration, h265ConvertH264Struct);
#else
			pXHClient = std::make_shared<CMediaStreamSource>(szURL, nClient, nSourceType, nDuration, h265ConvertH264Struct);
#endif

 		} while (pXHClient == NULL);
	}
	catch (const std::exception &e)
	{
		return NULL;
	}

	auto ret =
		xh_ABLMediaStreamSourceMap.insert(std::make_pair(strURL, pXHClient));
	if (!ret.second)
	{
		return NULL;
	}

	return pXHClient;
}

CMediaStreamSource_ptr GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false)
{
	std::lock_guard<std::mutex> lock(ABL_CMediaStreamSourceMapLock);

	
	CMediaStreamSource_ptr   pClient = NULL;
 
	auto iterator1 = xh_ABLMediaStreamSourceMap.find(szURL);
	if (iterator1 != xh_ABLMediaStreamSourceMap.end())
	{
		pClient = (*iterator1).second;
		return pClient;
	}
	else
	{
		//码流找不到
		if (ABL_MediaServerPort.hook_enable == 1 && bNoticeStreamNoFound && strstr(szURL, RecordFileReplaySplitter) == NULL)
		{
			int      nPos2 = 0;
			char     szApp[512] = { 0 };
			char     szStream[512] = { 0 };
			string   strMediaSource = szURL;

			nPos2 = strMediaSource.find("/", 1);
			if (nPos2 > 0 && nPos2 != string::npos && strlen(szURL) < 512)
			{
				memcpy(szApp, szURL + 1, nPos2 - 1);
				memcpy(szStream, szURL + nPos2 + 1, strlen(szURL) - nPos2 - 1);
			}

			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_Not_found;
			sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\"}", szApp, szStream, ABL_MediaServerPort.mediaServerID);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		}

		return NULL;
	}
}

CMediaStreamSource_ptr GetMediaStreamSourceNoLock(char* szURL)
{

	CMediaStreamSource_ptr   pClient = NULL;

	auto iterator1 = xh_ABLMediaStreamSourceMap.find(szURL);
	if (iterator1 != xh_ABLMediaStreamSourceMap.end())
	{
		pClient = (*iterator1).second;
		return pClient;
	}
	else
	{
		return NULL;
	}
}

bool  DeleteMediaStreamSource(char* szURL)
{
	std::lock_guard<std::mutex> lock(ABL_CMediaStreamSourceMapLock);

	
	auto iterator1 = xh_ABLMediaStreamSourceMap.find(szURL);
	if (iterator1 != xh_ABLMediaStreamSourceMap.end())
	{
		//媒体断线时通知
		if (ABL_MediaServerPort.hook_enable == 1 && (strlen((*iterator1).second->m_mediaCodecInfo.szVideoName) > 0 || strlen((*iterator1).second->m_mediaCodecInfo.szAudioName) > 0))
		{
			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_disconnect;
			sprintf(msgNotice.szMsg, "{\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\",\"networkType\":%d,\"key\":%llu}", (*iterator1).second->app, (*iterator1).second->stream, ABL_MediaServerPort.mediaServerID, (*iterator1).second->netBaseNetType, (*iterator1).second->nClient);
			pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
		}

		xh_ABLMediaStreamSourceMap.erase(iterator1);
		return true;
	}
	else
	{
		return false;
	}
}

//把客户端ID，从媒体资源移除，不再拷贝 
bool DeleteClientMediaStreamSource(uint64_t nClient)
{
	std::lock_guard<std::mutex> lock(ABL_CMediaStreamSourceMapLock);
	CMediaStreamSource_ptrMap::iterator iterator1;
	CMediaStreamSource_ptr   pClient = NULL;
	bool bDeleteFlag = false;

	for (iterator1 = xh_ABLMediaStreamSourceMap.begin(); iterator1 != xh_ABLMediaStreamSourceMap.end(); ++iterator1)
	{
		pClient = (*iterator1).second;
		if (pClient->DeleteClientFromMap(nClient))
		{
			bDeleteFlag = true;
			break;
		}
	}
	return bDeleteFlag;
}

//删除媒体源
int  CloseMediaStreamSource(closeStreamsStruct closeStruct)
{
	std::lock_guard<std::mutex> lock(ABL_CMediaStreamSourceMapLock);
	CMediaStreamSource_ptrMap::iterator iterator1;
	CMediaStreamSource_ptr   pClient = NULL;
	int  nDeleteCount = 0;

	for (iterator1 = xh_ABLMediaStreamSourceMap.begin(); iterator1 != xh_ABLMediaStreamSourceMap.end(); ++iterator1)
	{
		pClient = (*iterator1).second;

		if (closeStruct.force == 1 && strlen(closeStruct.app) > 0 && strlen(closeStruct.stream) > 0)
		{//强制关闭
			if (strcmp(pClient->app, closeStruct.app) == 0 && strcmp(pClient->stream, closeStruct.stream) == 0)
			{
				nDeleteCount++;
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
		}
		else if (closeStruct.force == 1 && strlen(closeStruct.app) > 0 && strlen(closeStruct.stream) == 0)
		{//强制关闭
			if (strcmp(pClient->app, closeStruct.app) == 0)
			{
				nDeleteCount++;
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
		}
		else if (closeStruct.force == 1 && strlen(closeStruct.app) == 0 && strlen(closeStruct.stream) > 0)
		{//强制关闭
			if (strcmp(pClient->stream, closeStruct.stream) == 0)
			{
				nDeleteCount++;
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
		}
		else if (closeStruct.force == 1 && strlen(closeStruct.app) == 0 && strlen(closeStruct.stream) == 0)
		{//强制关闭
			nDeleteCount++;
			pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
		}
		else if (closeStruct.force == 0 && strlen(closeStruct.app) > 0 && strlen(closeStruct.stream) > 0)
		{//不强制关闭
			if (pClient->mediaSendMap.size() == 0 && strcmp(pClient->app, closeStruct.app) == 0 && strcmp(pClient->stream, closeStruct.stream) == 0)
			{
				nDeleteCount++;
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
		}
		else if (closeStruct.force == 0 && strlen(closeStruct.app) > 0 && strlen(closeStruct.stream) == 0)
		{//不强制关闭
			if (pClient->mediaSendMap.size() == 0 && strcmp(pClient->app, closeStruct.app) == 0)
			{
				nDeleteCount++;
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
		}
		else if (closeStruct.force == 0 && strlen(closeStruct.app) == 0 && strlen(closeStruct.stream) > 0)
		{//不强制关闭
			if (pClient->mediaSendMap.size() == 0 && strcmp(pClient->stream, closeStruct.stream) == 0)
			{
				nDeleteCount++;
				pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
			}
		}
	}
	return nDeleteCount;
}

//先把基类的视频接入对象ID全部装入FIFO，这样减少一个lock，杜绝外部混乱调用http api 函数造成死锁 
int   GetAllNetBaseObjectToFifo()
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);
	CNetRevcBase_ptrMap::iterator iterator1;
	pNetBaseObjectFifo.Reset();

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		CNetRevcBase_ptr pClient = (*iterator1).second;
		if (pClient->netBaseNetType == NetBaseNetType_WebSocektRecvAudio || pClient->netBaseNetType == NetBaseNetType_addStreamProxyControl || pClient->netBaseNetType == NetBaseNetType_RtspServerRecvPush || pClient->netBaseNetType == NetBaseNetType_RtmpServerRecvPush || pClient->netBaseNetType == NetBaseNetType_NetServerReadMultRecordFile ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || pClient->netBaseNetType == ReadRecordFileInput_ReadFMP4File || pClient->netBaseNetType == NetBaseNetType_NetGB28181UDPTSStreamInput || pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181UDPPSStreamInput || (pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP && strlen(pClient->m_startSendRtpStruct.recv_app) > 0 && strlen(pClient->m_startSendRtpStruct.recv_stream) > 0) || (pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect && strlen(pClient->m_startSendRtpStruct.recv_app) > 0 && strlen(pClient->m_startSendRtpStruct.recv_stream) > 0) ||
			(pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive && strlen(pClient->m_startSendRtpStruct.recv_app) > 0 && strlen(pClient->m_startSendRtpStruct.recv_stream) > 0) || pClient->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
		{//代理拉流（rtsp,rtmp,flv,hls ）,rtsp推流，rtmp推流，gb28181，webrtc 
			pNetBaseObjectFifo.push((unsigned char*)&(*iterator1).second->nClient, sizeof((*iterator1).second->nClient));
		}
	}

	return pNetBaseObjectFifo.GetSize();
}

//获取媒体源
int GetAllMediaStreamSource(char* szMediaSourceInfo, getMediaListStruct mediaListStruct)
{
	int              nMediaCount = 0;
	char             szTemp2[1024 * 48] = { 0 };
	char             szShareMediaURL[string_length_2048];
	bool             bAddFlag = false;
	uint64_t         nNoneReadDuration = 0;
	unsigned short   nClientPort;
	char             szApp[string_length_256] = { 0 };
	char             szStream[string_length_512] = { 0 };
	unsigned char* pData;
	uint64_t         nClient;
	int              nLength;

	//获取基类的视频接入对象ID
	GetAllNetBaseObjectToFifo();

	if (xh_ABLMediaStreamSourceMap.size() > 0)
		strcpy(szMediaSourceInfo, "{\"code\":0,\"memo\":\"success\",\"mediaList\":[");

	while ((pData = pNetBaseObjectFifo.pop(&nLength)) != NULL)
	{
		memcpy((char*)&nClient, pData, sizeof(uint64_t));
		CNetRevcBase_ptr pClient = GetNetRevcBaseClient(nClient);

		if (pClient != NULL)
		{
			if (pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
			{
				sprintf(szShareMediaURL, "/%s/%s", pClient->m_startSendRtpStruct.recv_app, pClient->m_startSendRtpStruct.recv_stream); //国标全双工
				strcpy(szApp, pClient->m_startSendRtpStruct.recv_app);
				strcpy(szStream, pClient->m_startSendRtpStruct.recv_stream);
			}
			else
			{
				sprintf(szShareMediaURL, "/%s/%s", pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream);
				strcpy(szApp, pClient->m_addStreamProxyStruct.app);
				strcpy(szStream, pClient->m_addStreamProxyStruct.stream);
			}
			auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
			bAddFlag = false;

			if (strlen(mediaListStruct.app) == 0 && strlen(mediaListStruct.stream) == 0 && tmpMediaSource != NULL)
			{
				bAddFlag = true;
			}
			else if (strlen(mediaListStruct.app) > 0 && strlen(mediaListStruct.stream) > 0 && tmpMediaSource != NULL)
			{
				if (strcmp(mediaListStruct.app, tmpMediaSource->app) == 0 && strcmp(mediaListStruct.stream, tmpMediaSource->stream) == 0)
					bAddFlag = true;
			}
			else if (strlen(mediaListStruct.app) > 0 && strlen(mediaListStruct.stream) == 0 && tmpMediaSource != NULL)
			{
				if (strcmp(mediaListStruct.app, tmpMediaSource->app) == 0)
					bAddFlag = true;
			}
			else if (strlen(mediaListStruct.app) == 0 && strlen(mediaListStruct.stream) > 0 && tmpMediaSource != NULL)
			{
				if (strcmp(mediaListStruct.stream, tmpMediaSource->stream) == 0)
					bAddFlag = true;
			}
			else
				bAddFlag = false;

			if (bAddFlag == true && tmpMediaSource != NULL)
			{
				if (strlen(tmpMediaSource->m_mediaCodecInfo.szVideoName) > 0 || strlen(tmpMediaSource->m_mediaCodecInfo.szAudioName) > 0)
				{
					memset(szTemp2, 0x00, sizeof(szTemp2));
					if (tmpMediaSource->mediaSendMap.size() > 0)
						nNoneReadDuration = 0;
					else
						nNoneReadDuration = GetCurrentSecond() - tmpMediaSource->nLastWatchTimeDisconect;

					nClientPort = pClient->nClientPort;
					if (pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)//国标TCP接入，需要替换为绑定的端口 
					{
						CNetRevcBase_ptr pParent = GetNetRevcBaseClient(pClient->hParent);
						if (pParent != NULL)
						{
							nClientPort = pParent->nClientPort;
							sprintf(pClient->m_addStreamProxyStruct.url, "rtp://%s:%d%s", pClient->szClientIP, nClientPort, szShareMediaURL);
						}
					}

					if (tmpMediaSource->nMediaSourceType == MediaSourceType_LiveMedia)
					{//实况播放
						sprintf(szTemp2, "{\"key\":%llu,\"port\":%d,\"app\":\"%s\",\"stream\":\"%s\",\"sim\":\"%s\",\"status\":%s,\"enable_hls\":%s,\"transcodingStatus\":%s,\"sourceURL\":\"%s\",\"networkType\":%d,\"readerCount\":%d,\"noneReaderDuration\":%llu,\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"videoBitrate\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"audioBitrate\":%d,\"url\":{\"rtsp\":\"rtsp://%s:%d/%s/%s\",\"rtmp\":\"rtmp://%s:%d/%s/%s\",\"http-flv\":\"http://%s:%d/%s/%s.flv\",\"ws-flv\":\"ws://%s:%d/%s/%s.flv\",\"http-mp4\":\"http://%s:%d/%s/%s.mp4\",\"http-hls\":\"http://%s:%d/%s/%s.m3u8\",\"webrtc\":\"http://%s:%d/webrtc-streamer.html?video=/%s/%s\"}},", tmpMediaSource->nClient, nClientPort, szApp, szStream, tmpMediaSource->sim, tmpMediaSource->enable_mp4 == true ? "true" : "false", tmpMediaSource->enable_hls == true ? "true" : "false", tmpMediaSource->H265ConvertH264_enable == true ? "true" : "false", pClient->m_addStreamProxyStruct.url, pClient->netBaseNetType, tmpMediaSource->mediaSendMap.size(), nNoneReadDuration,
							tmpMediaSource->m_mediaCodecInfo.szVideoName, tmpMediaSource->m_mediaCodecInfo.nVideoFrameRate, tmpMediaSource->m_mediaCodecInfo.nWidth, tmpMediaSource->m_mediaCodecInfo.nHeight, tmpMediaSource->m_mediaCodecInfo.nVideoBitrate, tmpMediaSource->m_mediaCodecInfo.szAudioName, tmpMediaSource->m_mediaCodecInfo.nChannels, tmpMediaSource->m_mediaCodecInfo.nSampleRate, tmpMediaSource->m_mediaCodecInfo.nAudioBitrate,
							ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, szApp, szStream);
					}
					else
					{//录像点播
						sprintf(szTemp2, "{\"key\":%llu,\"port\":%d,\"app\":\"%s\",\"stream\":\"%s\",\"sim\":\"%s\",\"status\":%s,\"enable_hls\":%s,\"transcodingStatus\":%s,\"sourceURL\":\"%s\",\"networkType\":%d,\"readerCount\":%d,\"noneReaderDuration\":%llu,\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"videoBitrate\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"audioBitrate\":%d,\"url\":{\"rtsp\":\"rtsp://%s:%d/%s/%s\",\"rtmp\":\"rtmp://%s:%d/%s/%s\",\"http-flv\":\"http://%s:%d/%s/%s.flv\",\"ws-flv\":\"ws://%s:%d/%s/%s.flv\",\"http-mp4\":\"http://%s:%d/%s/%s.mp4\"}},", tmpMediaSource->nClient, nClientPort, szApp, szStream, tmpMediaSource->sim, tmpMediaSource->enable_mp4 == true ? "true" : "false", "false", tmpMediaSource->enable_mp4 == true ? "true" : "false", pClient->m_addStreamProxyStruct.url, pClient->netBaseNetType, tmpMediaSource->mediaSendMap.size(), nNoneReadDuration,
							tmpMediaSource->m_mediaCodecInfo.szVideoName, tmpMediaSource->m_mediaCodecInfo.nVideoFrameRate, tmpMediaSource->m_mediaCodecInfo.nWidth, tmpMediaSource->m_mediaCodecInfo.nHeight, tmpMediaSource->m_mediaCodecInfo.nVideoBitrate, tmpMediaSource->m_mediaCodecInfo.szAudioName, tmpMediaSource->m_mediaCodecInfo.nChannels, tmpMediaSource->m_mediaCodecInfo.nSampleRate, tmpMediaSource->m_mediaCodecInfo.nAudioBitrate,
							ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, szApp, szStream,
							ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, szApp, szStream);
					}

					strcat(szMediaSourceInfo, szTemp2);

					nMediaCount++;
				}
			}
		}//if (pClient != NULL)

		pNetBaseObjectFifo.pop_front();
	}

	if (nMediaCount > 0)
	{
		szMediaSourceInfo[strlen(szMediaSourceInfo) - 1] = 0x00;
		strcat(szMediaSourceInfo, "]}");
	}

	if (nMediaCount == 0)
		sprintf(szMediaSourceInfo, "{\"code\":%d,\"memo\":\"MediaSource [app: %s , stream: %s] Not Found .\"}", IndexApiCode_RequestFileNotFound, mediaListStruct.app, mediaListStruct.stream);

	return nMediaCount;
}

//获取服务器占用端口
int GetALLListServerPort(char* szMediaSourceInfo, ListServerPortStruct  listServerPortStruct)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int   nMediaCount = 0;
	char  szTemp2[string_length_8192] = { 0 };
	uint64_t nClient = 0;

	if (xh_ABLNetRevcBaseMap.size() > 0)
	{
		strcpy(szMediaSourceInfo, "{\"code\":0,\"memo\":\"success\",\"data\":[");
	}

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		CNetRevcBase_ptr    pClient = (*iterator1).second;

		//WriteLog(Log_Debug,  "GetALLListServerPort netBaseNetType = %d, nClient = %llu , app = %s ,string = %s ,nClientPort = %d",pClient->netBaseNetType, pClient->nClient, pClient->m_addStreamProxyStruct.app,pClient->m_addStreamProxyStruct.stream,pClient->nClientPort);

		if (strlen(pClient->m_addStreamProxyStruct.app) > 0 && strlen(pClient->m_addStreamProxyStruct.stream) > 0 && pClient->nClientPort > 0)
		{
			if (pClient->nClientPort >= 0 && !(pClient->netBaseNetType == NetBaseNetType_addStreamProxyControl || pClient->netBaseNetType == NetBaseNetType_addPushProxyControl || pClient->netBaseNetType == NetBaseNetType_NetServerHTTP ||
				pClient->netBaseNetType == NetBaseNetType_RecordFile_FMP4 || pClient->netBaseNetType == NetBaseNetType_RecordFile_MP4 || pClient->netBaseNetType == RtspPlayerType_RecordReplay || pClient->netBaseNetType == NetBaseNetType_SnapPicture_JPEG ||
				pClient->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream
				))
			{
				memset(szTemp2, 0x00, sizeof(szTemp2));

				if (pClient->netBaseNetType >= NetBaseNetType_RtspClientRecv && pClient->netBaseNetType <= NetBaseNetType_GB28181ClientPushUDP)
					nClient = pClient->hParent;//有代理类的都需要 返回父类ID 
				else
					nClient = pClient->nClient;

				if (strlen(listServerPortStruct.app) == 0 && strlen(listServerPortStruct.stream) == 0)
				{
					sprintf(szTemp2, "{\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"port\":%d},", nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, pClient->netBaseNetType, pClient->nClientPort);
				}
				else if (strlen(listServerPortStruct.app) > 0 && strlen(listServerPortStruct.stream) > 0 && strcmp(pClient->m_addStreamProxyStruct.app, listServerPortStruct.app) == 0 && strcmp(pClient->m_addStreamProxyStruct.stream, listServerPortStruct.stream) == 0)
				{
					sprintf(szTemp2, "{\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"port\":%d},", nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, pClient->netBaseNetType, pClient->nClientPort);
				}
				else if (strlen(listServerPortStruct.app) > 0 && strlen(listServerPortStruct.stream) == 0 && strcmp(pClient->m_addStreamProxyStruct.app, listServerPortStruct.app) == 0)
				{
					sprintf(szTemp2, "{\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"port\":%d},", nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, pClient->netBaseNetType, pClient->nClientPort);
				}
				else if (strlen(listServerPortStruct.app) == 0 && strlen(listServerPortStruct.stream) > 0 && strcmp(pClient->m_addStreamProxyStruct.stream, listServerPortStruct.stream) == 0)
				{
					sprintf(szTemp2, "{\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"port\":%d},", nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, pClient->netBaseNetType, pClient->nClientPort);
				}

				if (strlen(szTemp2) > 0)
				{
					strcat(szMediaSourceInfo, szTemp2);
					nMediaCount++;
				}
			}
		}

	}

	if (nMediaCount > 0)
	{
		szMediaSourceInfo[strlen(szMediaSourceInfo) - 1] = 0x00;
		strcat(szMediaSourceInfo, "]}");
	}

	if (nMediaCount == 0)
	{
		sprintf(szMediaSourceInfo, "{\"code\":%d,\"memo\":\"listServerPort [app: %s , stream: %s] Not Found .\"}", IndexApiCode_RequestFileNotFound, listServerPortStruct.app, listServerPortStruct.stream);
	}

	return nMediaCount;
}

//获取所有往外发送的列
int GetAllOutList(char* szMediaSourceInfo, char* szOutType)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;
	int   nMediaCount = 0;
	char  szTemp2[string_length_8192] = { 0 };
	uint64_t nClient = 0;

	if (xh_ABLMediaStreamSourceMap.size() > 0)
	{
		strcpy(szMediaSourceInfo, "{\"code\":0,\"memo\":\"success\",\"outList\":[");
	}

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		pClient = (*iterator1).second;

		if (pClient->netBaseNetType == NetBaseNetType_RtmpServerSendPush || pClient->netBaseNetType == NetBaseNetType_RtspServerSendPush || pClient->netBaseNetType == NetBaseNetType_HttpFLVServerSendPush ||
			pClient->netBaseNetType == NetBaseNetType_HttpHLSServerSendPush || pClient->netBaseNetType == NetBaseNetType_WsFLVServerSendPush || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || pClient->netBaseNetType == NetBaseNetType_RtspClientPush || pClient->netBaseNetType == NetBaseNetType_RtmpClientPush ||
			pClient->netBaseNetType == NetBaseNetType_HttpMP4ServerSendPush || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive || pClient->netBaseNetType == NetBaseNetType_NetClientWebrtcPlayer)
		{
			if (pClient->netBaseNetType >= NetBaseNetType_RtspClientRecv && pClient->netBaseNetType <= NetBaseNetType_GB28181ClientPushUDP)
				nClient = pClient->hParent;//有代理类的都需要 返回父类ID 
			else
				nClient = pClient->nClient;

			if (strlen(pClient->mediaCodecInfo.szVideoName) > 0 || strlen(pClient->mediaCodecInfo.szAudioName) > 0)
			{
				sprintf(szTemp2, "{\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"sourceURL\":\"%s\",\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"networkType\":%d,\"dst_url\":\"%s\",\"dst_port\":%d},", nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, pClient->m_addStreamProxyStruct.url,
					pClient->mediaCodecInfo.szVideoName, pClient->mediaCodecInfo.nVideoFrameRate, pClient->mediaCodecInfo.nWidth, pClient->mediaCodecInfo.nHeight, pClient->mediaCodecInfo.szAudioName, pClient->mediaCodecInfo.nChannels, pClient->mediaCodecInfo.nSampleRate, pClient->netBaseNetType, pClient->szClientIP, pClient->nClientPort);

				strcat(szMediaSourceInfo, szTemp2);

				nMediaCount++;
			}
		}
	}

	if (nMediaCount > 0)
	{
		szMediaSourceInfo[strlen(szMediaSourceInfo) - 1] = 0x00;
		strcat(szMediaSourceInfo, "]}");
	}

	if (nMediaCount == 0)
	{
		sprintf(szMediaSourceInfo, "{\"code\":%d,\"memo\":\"success\",\"count\":%d}", IndexApiCode_OK, nMediaCount);
	}

	return nMediaCount;
}

//检测app,stream 是否被占用 
bool CheckAppStreamExisting(char* szAppStreamURL)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;
	bool   bAppStreamExisting = false;
	char   szTemp2[string_length_8192] = { 0 };

	if (xh_ABLNetRevcBaseMap.size() <= 0)
	{
		return false;
	}

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		pClient = (*iterator1).second;
		if (pClient != NULL && pClient->netBaseNetType != NetBaseNetType_NetServerHTTP)
		{
			sprintf(szTemp2, "/%s/%s", pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream);
			if (strcmp(szTemp2, szAppStreamURL) == 0)
			{
				bAppStreamExisting = true;
				WriteLog(Log_Debug, "CheckAppStreamExisting(), url = %s  已经存在 ,正在接入 !", szAppStreamURL);
				break;
			}
		}
	}
	if (!bAppStreamExisting)
		WriteLog(Log_Debug, "CheckAppStreamExisting(), url = %s  尚未使用 !", szAppStreamURL);

	return bAppStreamExisting;
}

/* 媒体数据存储 -------------------------------------------------------------------------------------*/

/* 录像文件存储 -------------------------------------------------------------------------------------*/
//删除过期的m3u8文件
int  DeleteExpireM3u8File()
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptrMap::iterator it;
	CRecordFileSource_ptr   pRecord = NULL;
	int                     nDeleteCount = 0;

	for (it = xh_ABLRecordFileSourceMap.begin(); it != xh_ABLRecordFileSourceMap.end(); ++it)
	{
		pRecord = (*it).second;
		if (pRecord)
			nDeleteCount += pRecord->DeleteM3u8ExpireFile();
	}
	return nDeleteCount;
}

CRecordFileSource_ptr GetRecordFileSource(char* szShareURL)
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptrMap::iterator iterator1;
	CRecordFileSource_ptr   pRecord = NULL;

	iterator1 = xh_ABLRecordFileSourceMap.find(szShareURL);
	if (iterator1 != xh_ABLRecordFileSourceMap.end())
	{
		pRecord = (*iterator1).second;
		return pRecord;
	}
	else
	{
		return NULL;
	}
}

CRecordFileSource_ptr CreateRecordFileSource(char* app, char* stream)
{
	char szShareURL[string_length_1024] = { 0 };
	sprintf(szShareURL, "/%s/%s", app, stream);
	CRecordFileSource_ptr pReordFile = GetRecordFileSource(szShareURL);
	if (pReordFile != NULL)
	{
		WriteLog(Log_Debug, "CreateRecordFileSource 失败 , app = %s ,stream = %s 已经存在 ", app, stream);
		return NULL;
	}

	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptr pRecord = NULL;

	try
	{
		do
		{
#ifdef USE_BOOST
			pRecord = boost::make_shared<CRecordFileSource>(app, stream);
#else
			pRecord = std::make_shared<CRecordFileSource>(app, stream);
#endif
	
		} while (pRecord == NULL);
	}
	catch (const std::exception& e)
	{
		return NULL;
	}

	auto ret =
		xh_ABLRecordFileSourceMap.insert(std::make_pair(pRecord->m_szShareURL, pRecord));
	if (!ret.second)
	{
		return NULL;
	}

	return pRecord;
}

bool  DeleteRecordFileSource(char* szURL)
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptrMap::iterator iterator1;

	iterator1 = xh_ABLRecordFileSourceMap.find(szURL);
	if (iterator1 != xh_ABLRecordFileSourceMap.end())
	{
		xh_ABLRecordFileSourceMap.erase(iterator1);
		return true;
	}
	else
	{
		return false;
	}
}

//增加一个录像文件到录像媒体源
bool AddRecordFileToRecordSource(char* szShareURL, char* szFileName)
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptrMap::iterator iterator1;

	iterator1 = xh_ABLRecordFileSourceMap.find(szShareURL);
	if (iterator1 != xh_ABLRecordFileSourceMap.end())
	{
		return (*iterator1).second->AddRecordFile(szFileName);
	}
	else
	{
		return false;
	}
}

extern  const struct mov_buffer_t* mov_file_buffer(void);
//查询录像
int queryRecordListByTime(char* szMediaSourceInfo, queryRecordListStruct queryStruct)
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptrMap::iterator iterator1;
	CRecordFileSource_ptr   pRecord = NULL;
	list<uint64_t>::iterator it2;

	int   nMediaCount = 0;
	char  szTemp2[1024 * 32] = { 0 };
	char  szTemp1[string_length_2048] = { 0 };
	char  szShareMediaURL[string_length_2048] = { 0 };
	bool  bAddFlag = false;
	FILE* fp = NULL;
	char           szFileName[string_length_1024] = { 0 };
	mov_reader_t* mov = NULL;
	uint64_t       duration = 0;//录像回放时读取到录像文件长度
	FILE* fileM3U8 = NULL;
	char           m3u8FileName[string_length_512] = { 0 };
	char           mapm3u8FileName[string_length_512] = { 0 };
	int            nFileOrder = 0;
	char           szRecordURL[string_length_48K] = { 0 };
	char           szRecordPlayURL[string_length_2048] = { 0 };
	CNetRevcBase_ptr  mutlRecordPlay = NULL;
	CMediaStreamSource_ptr pMediaStreamPtr = NULL;
	char           szFileNameTime[string_length_256] = { 0 };
	uint64_t       nTime1, nTime2;
	bool           bFlag1 = false;
	bool           bFlag2 = false;

	if (xh_ABLRecordFileSourceMap.size() > 0)
	{
		if (ABL_MediaServerPort.videoFileFormat == 3)
		{
#ifdef  OS_System_Windows
			sprintf(m3u8FileName, "%s%s\\%s\\%s_%s.m3u8", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
			sprintf(mapm3u8FileName, "\\%s\\%s\\%s_%s.m3u8", queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
#else
			sprintf(m3u8FileName, "%s%s/%s/%s_%s.m3u8", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
			sprintf(mapm3u8FileName, "/%s/%s/%s_%s.m3u8", queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
#endif 
			fileM3U8 = fopen(m3u8FileName, "wb");
			sprintf(szTemp1, "\"http-hls\":\"http://%s:%d/%s/%s%s%s_%s.m3u8\"", ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, queryStruct.starttime, queryStruct.endtime);
	}

		if (fileM3U8 != NULL)
		{//ts 切片
			sprintf(szRecordURL, "\"url\":{\"rtsp\": \"rtsp://%s:%d/%s/%s_%s-%s\",\"rtmp\": \"rtmp://%s:%d/%s/%s_%s-%s\",\"http-flv\": \"http://%s:%d/%s/%s_%s-%s.flv\",\"ws-flv\": \"ws://%s:%d/%s/%s_%s-%s.flv\",\"http-mp4\": \"http://%s:%d/%s/%s_%s-%s.mp4\",%s}",
				ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
				szTemp1
			);
		}
		else
		{//非TS 
			if (ABL_MediaServerPort.nHlsEnable == 1)
			{//hls切片
				sprintf(szRecordURL, "\"url\":{\"rtsp\": \"rtsp://%s:%d/%s/%s_%s-%s\",\"rtmp\": \"rtmp://%s:%d/%s/%s_%s-%s\",\"http-flv\": \"http://%s:%d/%s/%s_%s-%s.flv\",\"ws-flv\": \"ws://%s:%d/%s/%s_%s-%s.flv\",\"http-mp4\": \"http://%s:%d/%s/%s_%s-%s.mp4\",\"http-hls\": \"http://%s:%d/%s/%s_%s-%s.m3u8\"}",
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime
				);
			}
			else
			{
				sprintf(szRecordURL, "\"url\":{\"rtsp\": \"rtsp://%s:%d/%s/%s_%s-%s\",\"rtmp\": \"rtmp://%s:%d/%s/%s_%s-%s\",\"http-flv\": \"http://%s:%d/%s/%s_%s-%s.flv\",\"ws-flv\": \"ws://%s:%d/%s/%s_%s-%s.flv\",\"http-mp4\": \"http://%s:%d/%s/%s_%s-%s.mp4\"}",
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime
				);
			}
		}
}

	sprintf(szShareMediaURL, "/%s/%s", queryStruct.app, queryStruct.stream);
	iterator1 = xh_ABLRecordFileSourceMap.find(szShareMediaURL);
	if (iterator1 != xh_ABLRecordFileSourceMap.end())
	{
		pRecord = (*iterator1).second;

		if (ABL_MediaServerPort.videoFileFormat == 3 && strlen(mapm3u8FileName) > 0)
			pRecord->AddM3u8FileToMap(mapm3u8FileName);

		for (it2 = pRecord->fileList.begin(); it2 != pRecord->fileList.end(); it2++)
		{
			sprintf(szFileNameTime, "%llu", *it2);
			nTime1 = GetCurrentSecondByTime(szFileNameTime);
			nTime2 = GetCurrentSecondByTime(queryStruct.starttime);
			bFlag1 = false;
			bFlag2 = false;
			if (nFileOrder == 0)
			{
				if (nTime1 <= nTime2 && (nTime1 + ABL_MediaServerPort.fileSecond) > nTime2)
					bFlag1 = true;//第一个符合条件的文件 
			}
			if (nFileOrder >= 1 && *it2 <= atoll(queryStruct.endtime))
				bFlag2 = true;//后面符合条件的文件

			//符合条件的mp4文件 
			if (bFlag1 || bFlag2)
			{
				memset(szTemp2, 0x00, sizeof(szTemp2));

				nFileOrder++;

				if (fileM3U8 != NULL)
				{//生成m3u8文件
					if (nFileOrder == 1)
					{
						sprintf(szTemp2, "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%d\n#EXT-X-MEDIA-SEQUENCE:%llu\n#EXT-X-ALLOW-CACHE:NO\n", ABL_MediaServerPort.fileSecond, *it2);
						fwrite(szTemp2, 1, strlen(szTemp2), fileM3U8);
						memset(szTemp2, 0x00, sizeof(szTemp2));
					}
					sprintf(szTemp1, "#EXTINF:%d.000,\n/%s/%s%s%llu.mp4\n", ABL_MediaServerPort.fileSecond, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2);
					fwrite(szTemp1, 1, strlen(szTemp1), fileM3U8);
					fflush(fileM3U8);
				}

#ifdef  OS_System_Windows
				sprintf(szFileName, "%s%s\\%s\\%llu.mp4", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, *it2);
#else
				sprintf(szFileName, "%s%s/%s/%llu.mp4", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, *it2);
#endif 
				if (nFileOrder == 1)
				{
					sprintf(szRecordPlayURL, "/%s/%s_%s-%s", queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);

					//没有存在媒体源再创建
					if ((pMediaStreamPtr = GetMediaStreamSource(szRecordPlayURL, false)) == NULL)
						mutlRecordPlay = CreateNetRevcBaseClient(NetBaseNetType_NetServerReadMultRecordFile, *it2, 0, szFileName, 0, szRecordPlayURL);
					if (mutlRecordPlay != NULL)
					{
						memcpy((char*)&mutlRecordPlay->m_queryRecordListStruct, (char*)&queryStruct, sizeof(queryStruct));
						sprintf(szMediaSourceInfo, "{\"code\":0,\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s_%s-%s\",\"starttime\":\"%s\",\"endtime\":\"%s\",%s,\"recordFileList\":[", mutlRecordPlay->nClient, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime, queryStruct.starttime, queryStruct.endtime, szRecordURL);
					}
					else
					{
						if (pMediaStreamPtr != NULL)
							sprintf(szMediaSourceInfo, "{\"code\":0,\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s_%s-%s\",\"starttime\":\"%s\",\"endtime\":\"%s\",%s,\"recordFileList\":[", pMediaStreamPtr->nClient, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime, queryStruct.starttime, queryStruct.endtime, szRecordURL);
					}
				}
				if (mutlRecordPlay != NULL)
					mutlRecordPlay->mutliRecordPlayNameList.push_back(szFileName);

				if (ABL_MediaServerPort.enable_GetFileDuration != 0)
				{//需要获取录像文件真实时长 
					fp = fopen(szFileName, "rb");
					if (fp)
					{
						mov = mov_reader_create(mov_file_buffer(), fp);
						if (mov)
							duration = mov_reader_getduration(mov);
					}
				}
				else
					duration = ABL_MediaServerPort.fileSecond * 1000;

				sprintf(szTemp2, "{\"file\":\"%llu.mp4\",\"duration\":%llu,\"url\":{\"rtsp\":\"rtsp://%s:%d/%s/%s%s%llu\",\"rtmp\":\"rtmp://%s:%d/%s/%s%s%llu\",\"http-flv\":\"http://%s:%d/%s/%s%s%llu.flv\",\"ws-flv\":\"ws://%s:%d/%s/%s%s%llu.flv\",\"http-mp4\":\"http://%s:%d/%s/%s%s%llu.mp4\",\"download\":\"http://%s:%d/%s/%s%s%llu.mp4?download_speed=%d\"}},", *it2, duration / 1000,
					ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2, ABL_MediaServerPort.httpDownloadSpeed);

				strcat(szMediaSourceInfo, szTemp2);
				nMediaCount++;

				if (ABL_MediaServerPort.enable_GetFileDuration != 0)
				{
					if (mov)
						mov_reader_destroy(mov);
					if (fp)
						fclose(fp);
				}
			}
		}
	}
	else
	{
		if (fileM3U8)
			fclose(fileM3U8);
		return 0;
	}

	if (nMediaCount > 0)
	{
		//计算最后一个mp4的播放时长 
		if (mutlRecordPlay != NULL)
		{
			CNetServerReadMultRecordFile* pReadMp4File = (CNetServerReadMultRecordFile*)mutlRecordPlay.get();
			if (pReadMp4File)
				pReadMp4File->CalcLastMp4FileDuration();
		}

		if (fileM3U8)
		{
			sprintf(szTemp1, "#EXT-X-ENDLIST\n");
			fwrite(szTemp1, 1, strlen(szTemp1), fileM3U8);
			fflush(fileM3U8);
		}
		szMediaSourceInfo[strlen(szMediaSourceInfo) - 1] = 0x00;
		strcat(szMediaSourceInfo, "]}");
	}

	if (fileM3U8)
		fclose(fileM3U8);

	if (nMediaCount == 0)
	{
		ABLDeleteFile(m3u8FileName);
		sprintf(szMediaSourceInfo, "{\"code\":%d,\"memo\":\"RecordList [app: %s , stream: %s] Record File Not Found .\"}", IndexApiCode_RequestFileNotFound, queryStruct.app, queryStruct.stream);
	}

	return nMediaCount;
}

//查询一个录像文件是否存在
bool QureyRecordFileFromRecordSource(char* szShareURL, char* szFileName)
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptrMap::iterator iterator1;

	iterator1 = xh_ABLRecordFileSourceMap.find(szShareURL);
	if (iterator1 != xh_ABLRecordFileSourceMap.end())
	{
		return (*iterator1).second->queryRecordFile(szFileName);
	}
	else
	{
		return false;
	}
}

/* 录像文件存储 -------------------------------------------------------------------------------------*/

/* 图片文件存储 -------------------------------------------------------------------------------------*/
CPictureFileSource_ptr GetPictureFileSource(char* szShareURL, bool bLock)
{
	if (bLock)
		std::lock_guard<std::mutex> lock(ABL_CPictureFileSourceMapLock);

	CPictureFileSource_ptrMap::iterator iterator1;
	CPictureFileSource_ptr   pPicture = NULL;

	iterator1 = xh_ABLPictureFileSourceMap.find(szShareURL);
	if (iterator1 != xh_ABLPictureFileSourceMap.end())
	{
		pPicture = (*iterator1).second;
		return pPicture;
	}
	else
	{
		return NULL;
	}
}

CPictureFileSource_ptr CreatePictureFileSource(char* app, char* stream)
{
	char szShareURL[string_length_1024] = { 0 };
	sprintf(szShareURL, "/%s/%s", app, stream);
	CPictureFileSource_ptr pReordFile = GetPictureFileSource(szShareURL, true);
	if (pReordFile != NULL)
	{
		WriteLog(Log_Debug, "CreatePictureFileSource 失败 , app = %s ,stream = %s 已经存在 ", app, stream);
		return NULL;
	}

	std::lock_guard<std::mutex> lock(ABL_CPictureFileSourceMapLock);

	CPictureFileSource_ptr pPicture = NULL;

	try
	{
		do
		{
#ifdef USE_BOOST
			pPicture = boost::make_shared<CPictureFileSource>(app, stream);
#else
			pPicture = std::make_shared<CPictureFileSource>(app, stream);
#endif
	
		} while (pPicture == NULL);
	}
	catch (const std::exception& e)
	{
		return NULL;
	}

	auto ret =
		xh_ABLPictureFileSourceMap.insert(std::make_pair(pPicture->m_szShareURL, pPicture));
	if (!ret.second)
	{
		return NULL;
	}

	return pPicture;
}

bool  DeletePictureFileSource(char* szURL)
{
	std::lock_guard<std::mutex> lock(ABL_CPictureFileSourceMapLock);

	CPictureFileSource_ptrMap::iterator iterator1;

	iterator1 = xh_ABLPictureFileSourceMap.find(szURL);
	if (iterator1 != xh_ABLPictureFileSourceMap.end())
	{
		xh_ABLPictureFileSourceMap.erase(iterator1);
		return true;
	}
	else
	{
		return false;
	}
}

//增加一个录像文件到录像媒体源
bool AddPictureFileToPictureSource(char* szShareURL, char* szFileName)
{
	std::lock_guard<std::mutex> lock(ABL_CPictureFileSourceMapLock);

	CPictureFileSource_ptrMap::iterator iterator1;

	iterator1 = xh_ABLPictureFileSourceMap.find(szShareURL);
	if (iterator1 != xh_ABLPictureFileSourceMap.end())
	{
		return (*iterator1).second->AddPictureFile(szFileName);
	}
	else
	{
		return false;
	}
}

//查询录像
int queryPictureListByTime(char* szMediaSourceInfo, queryPictureListStruct queryStruct)
{
	std::lock_guard<std::mutex> lock(ABL_CPictureFileSourceMapLock);

	CPictureFileSource_ptrMap::iterator iterator1;
	CPictureFileSource_ptr   pPicture = NULL;
	list<uint64_t>::iterator it2;

	int   nMediaCount = 0;
	char  szTemp2[string_length_8192] = { 0 };
	char  szShareMediaURL[string_length_2048] = { 0 };
	bool  bAddFlag = false;

	if (xh_ABLPictureFileSourceMap.size() > 0)
	{
		sprintf(szMediaSourceInfo, "{\"code\":0,\"app\":\"%s\",\"stream\":\"%s\",\"starttime\":\"%s\",\"endtime\":\"%s\",\"PictureFileList\":[", queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
	}

	sprintf(szShareMediaURL, "/%s/%s", queryStruct.app, queryStruct.stream);
	iterator1 = xh_ABLPictureFileSourceMap.find(szShareMediaURL);
	if (iterator1 != xh_ABLPictureFileSourceMap.end())
	{
		pPicture = (*iterator1).second;

		for (it2 = pPicture->fileList.begin(); it2 != pPicture->fileList.end(); it2++)
		{
			if ((*it2 / 100) >= atoll(queryStruct.starttime) && (*it2 / 100) <= atoll(queryStruct.endtime))
			{
				memset(szTemp2, 0x00, sizeof(szTemp2));

				sprintf(szTemp2, "{\"file\":\"%llu.jpg\",\"url\":\"http://%s:%d/%s/%s/%llu.jpg\"},", *it2,
					ABL_szLocalIP, ABL_MediaServerPort.nHttpServerPort, queryStruct.app, queryStruct.stream, *it2);

				strcat(szMediaSourceInfo, szTemp2);
				nMediaCount++;
			}
		}
	}
	else
	{
		return 0;
	}

	if (nMediaCount > 0)
	{
		szMediaSourceInfo[strlen(szMediaSourceInfo) - 1] = 0x00;
		strcat(szMediaSourceInfo, "]}");
	}

	if (nMediaCount == 0)
	{
		sprintf(szMediaSourceInfo, "{\"code\":%d,\"memo\":\"PictureList [app: %s , stream: %s] Picture File Not Found .\"}", IndexApiCode_RequestFileNotFound, queryStruct.app, queryStruct.stream);
	}

	return nMediaCount;
}

//查询一个图片文件是否存在
bool QureyPictureFileFromPictureSource(char* szShareURL, char* szFileName)
{
	std::lock_guard<std::mutex> lock(ABL_CPictureFileSourceMapLock);

	CPictureFileSource_ptrMap::iterator iterator1;

	iterator1 = xh_ABLPictureFileSourceMap.find(szShareURL);
	if (iterator1 != xh_ABLPictureFileSourceMap.end())
	{
		return (*iterator1).second->queryPictureFile(szFileName);
	}
	else
	{
		return false;
	}
}
/* 图片文件存储 -------------------------------------------------------------------------------------*/

void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	void* address);

void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result, uint16_t nLocalPort);

void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);

#ifdef USE_BOOST
CNetRevcBase_ptr CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptr pXHClient = NULL;
	try
	{
		do
		{
			if (netClientType == NetRevcBaseClient_ServerAccept)
			{
				if (serverHandle == srvhandle_8080)
					pXHClient = boost::make_shared<CNetServerHTTP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_554)
					pXHClient = boost::make_shared<CNetRtspServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_1935)
					pXHClient = boost::make_shared<CNetRtmpServerRecv>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_8088)
					pXHClient = boost::make_shared<CNetServerHTTP_FLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_6088)
					pXHClient = boost::make_shared<CNetServerWS_FLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_9088)
					pXHClient = boost::make_shared<CNetServerHLS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_8089)
					pXHClient = boost::make_shared<CNetServerHTTP_MP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_9298)
					pXHClient = boost::make_shared<CNetServerRecvAudio>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_10000)
				{//国标单端口输入
					pXHClient = boost::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient != NULL)
					{
						CNetGB28181RtpServer* gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
						pXHClient->netBaseNetType = NetBaseNetType_GB28181TcpPSInputStream;
						gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength];
						gb28181TCP->m_addStreamProxyStruct.RtpPayloadDataType[0] = 0x31;//PS  
						gb28181TCP->m_addStreamProxyStruct.disableVideo[0] = 0x30;//没有屏蔽视频
						gb28181TCP->m_addStreamProxyStruct.disableAudio[0] = 0x30;//没有屏蔽音频
					}
				}
				else
				{
					CNetRevcBase_ptr gb28181Listen = GetNetRevcBaseClientNoLock(serverHandle);
					if (gb28181Listen && gb28181Listen->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen && gb28181Listen->nMediaClient == 0)
					{//国标TCP 被动方式接入 
						CNetGB28181RtpServer* gb28181TCP = NULL;
						pXHClient = boost::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, gb28181Listen->m_szShareMediaURL);
						if (pXHClient != NULL)
						{
							pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Server;//国标28181 tcp 方式接收码流 
							gb28181Listen->nMediaClient = CltHandle; //已经有人连接进来

							gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
							if (gb28181TCP)
							{
								strcpy(gb28181TCP->szClientIP, szIP);
								gb28181TCP->nClientPort = nPort;
								gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength]; //在使用前先准备好内存 
							}

							pXHClient->hParent = gb28181Listen->nClient;//记录国标代理句柄号
							pXHClient->m_gbPayload = atoi(gb28181Listen->m_openRtpServerStruct.payload);//更新paylad 
							memcpy((char*)&pXHClient->m_addStreamProxyStruct, (char*)&gb28181Listen->m_addStreamProxyStruct, sizeof(gb28181Listen->m_addStreamProxyStruct));
							memcpy((char*)&pXHClient->m_openRtpServerStruct, (char*)&gb28181Listen->m_openRtpServerStruct, sizeof(gb28181Listen->m_openRtpServerStruct));
							memcpy((char*)&pXHClient->m_h265ConvertH264Struct, (char*)&gb28181Listen->m_h265ConvertH264Struct, sizeof(gb28181Listen->m_h265ConvertH264Struct));//单独指定转码参数
						}
					}
					else if (gb28181Listen && gb28181Listen->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen && gb28181Listen->nMediaClient == 0)
					{//国标 tcp 被动方式 发送 
						pXHClient = boost::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, gb28181Listen->m_szShareMediaURL);
						if (pXHClient != NULL)
						{
							pXHClient->hParent = gb28181Listen->nClient;//记录listen的ID 
							pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpTCP_Passive;//国标28181 tcp 被动方式发送码流 
							gb28181Listen->nMediaClient = CltHandle; //已经有人连接进来，只允许一个连接进来 
							memcpy((char*)&pXHClient->m_startSendRtpStruct, (char*)&gb28181Listen->m_startSendRtpStruct, sizeof(pXHClient->m_startSendRtpStruct)); //把listen对象的 m_startSendRtpStruct 拷贝给CNetGB28181RtpClient对象的 m_startSendRtpStruct
							pXHClient->SendFirstRequst();
						}
					}
					else
						return NULL;
				}
			}
			else if (netClientType == NetRevcBaseClient_addStreamProxyControl || netClientType == NetRevcBaseClient_addFFmpegProxyControl)
			{//代理拉流控制包括自研、调用ffmepg 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CNetClientAddStreamProxy>(netClientType, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->nClient = CltHandle;
			}
			else if (netClientType == NetRevcBaseClient_addPushProxyControl)
			{//代理推流控制 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CNetClientAddPushProxy>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->nClient = CltHandle;
			}
			else if (netClientType == NetRevcBaseClient_addStreamProxy)
			{//代理拉流
				if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".m3u8") != NULL)
				{//hls 暂时不支持 hls 拉流 
					pXHClient = boost::make_shared<CNetClientRecvHttpHLS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".flv") != NULL)
				{//flv 
					pXHClient = boost::make_shared<CNetClientRecvFLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (memcmp(szIP, "rtsp://", 7) == 0)
				{//rtsp 
					pXHClient = boost::make_shared<CNetClientRecvRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
					if (CltHandle == 0)
					{//连接失败
						WriteLog(Log_Debug, "CreateNetRevcBaseClient()，连接 rtsp 服务器 失败 szURL = %s , szIP = %s ,port = %s ", szIP, pXHClient->m_rtspStruct.szIP, pXHClient->m_rtspStruct.szPort);
						pDisconnectBaseNetFifo.push((unsigned char*)&pXHClient->nClient, sizeof(pXHClient->nClient));
					}
				}
				else if (memcmp(szIP, "rtmp://", 7) == 0)
				{//rtmp
					pXHClient = boost::make_shared<CNetClientRecvRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (strstr(szIP, ".mp4") != NULL || strstr(szIP, ".mov") != NULL || strstr(szIP, ".mkv") != NULL || strstr(szIP, ".ts") != NULL || strstr(szIP, ".ps") != NULL || strstr(szIP, ".flv") != NULL || strstr(szIP, ".264") != NULL || strstr(szIP, ".265") != NULL)
				{//本地文件
					CltHandle = XHNetSDK_GenerateIdentifier();
					pXHClient = boost::make_shared<CNetClientReadLocalMediaFile>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient;
				}
				else
					return NULL;
			}
			else if (netClientType == NetRevcBaseClient_addFFmpegProxy)
			{//ffmpeg 代理拉流
				if (strstr(szIP, "rtsp://") != NULL || strstr(szIP, "rtmp://") != NULL || strstr(szIP, "http://") != NULL || strstr(szIP, "https://") != NULL || strstr(szIP, ".mp4") != NULL || strstr(szIP, ".mov") != NULL || strstr(szIP, ".mkv") != NULL || strstr(szIP, ".ts") != NULL || strstr(szIP, ".ps") != NULL || strstr(szIP, ".flv") != NULL || strstr(szIP, ".264") != NULL || strstr(szIP, ".265") != NULL)
				{//本地文件
					CltHandle = XHNetSDK_GenerateIdentifier();
					pXHClient = boost::make_shared<CNetClientFFmpegRecv>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient;
				}
			}
			else if (netClientType == NetRevcBaseClient_addPushStreamProxy)
			{//代理推流
				if (memcmp(szIP, "rtsp://", 7) == 0)
				{//hls 暂时不支持 hls 拉流 
					pXHClient = boost::make_shared<CNetClientSendRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (memcmp(szIP, "rtmp://", 7) == 0)
				{
					pXHClient = boost::make_shared<CNetClientSendRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerUDP)
			{//创建GB28181 的udp接收
				pXHClient = boost::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerUDP;
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerTCP_Active)
			{//创建GB28181 的TCP 主动连接方式 
				pXHClient = boost::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient != NULL)
				{
					CNetGB28181RtpServer* gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
					gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength]; //在使用前先准备好内存 
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Active;
					pXHClient->hParent = CltHandle;
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181SendRtpUDP)
			{//创建GB28181 的udp发送
				pXHClient = boost::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpUDP;
			}
			else if (netClientType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
			{//创建GB28181 的tcp发送 
				pXHClient = boost::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpTCP_Connect;
			}
			else if (netClientType == NetBaseNetType_RecordFile_FMP4)
			{//fmp4录像
				pXHClient = boost::make_shared<CStreamRecordFMP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_RecordFile_FMP4;
			}
			else if (netClientType == NetBaseNetType_RecordFile_MP4)
			{//mp4录像
				pXHClient = boost::make_shared<CStreamRecordMP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_RecordFile_MP4;
			}
			else if (netClientType == NetBaseNetType_RecordFile_TS)
			{//ts录像
				pXHClient = boost::make_shared<CStreamRecordTS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == ReadRecordFileInput_ReadFMP4File)
			{//读取fmp4文件
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CReadRecordFileInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = ReadRecordFileInput_ReadFMP4File;
			}
			else if (netClientType == NetBaseNetType_SnapPicture_JPEG)
			{//抓拍图片
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CNetClientSnap>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == NetBaseNetType_HttpClient_None_reader)
			{//事件通知1
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNoneReader = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型  
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Not_found)
			{//事件通知2
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNotFound = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Record_mp4)
			{//事件通知3
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordMp4 = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_arrive)
			{//事件通知4
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_not_arrive)
			{//事件通知5
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNotArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_disconnect)
			{//事件通知6
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientDisconnect = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_record_ts)
			{//事件通知7
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordTS = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Record_Progress)
			{//事件通知8
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordProgress = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_ServerStarted)
			{//事件通知9
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nServerStarted = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_ServerKeepalive)
			{//事件通知10
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nServerKeepalive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_DeleteRecordMp4)
			{//事件通知11
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientDeleteRecordMp4 = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_play)
			{//事件通知12
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nPlay = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_publish)
			{//事件通知13
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nPublish = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_iframe_arrive)
			{//事件通知14
				pXHClient = boost::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nFrameArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RecvRtpPS_TS)
			{//单端口接收国标 
				pXHClient = boost::make_shared<CNetServerRecvRtpTS_PS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181UDPTSStreamInput)
			{//TS 解包形成媒体源
				pXHClient = boost::make_shared<CRtpTSStreamInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181UDPPSStreamInput)
			{//PS 解包形成媒体源
				pXHClient = boost::make_shared<CRtpPSStreamInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerListen)
			{//国标TCP被动接收的Listen 
				pXHClient = boost::make_shared<CNetGB28181Listen>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerListen;
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpSendListen)
			{//国标TCP被动发送的Listen
				pXHClient = boost::make_shared<CNetGB28181Listen>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpSendListen;
				}
			}
			else if (netClientType == NetBaseNetType_RtspServerRecvPushVideo)
			{//接收rtsp推流udp方式视频码流
				pXHClient = boost::make_shared<CNetRtspServerUDP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_RtspServerRecvPushVideo;
				}
			}
			else if (netClientType == NetBaseNetType_RtspServerRecvPushAudio)
			{//接收rtsp推流udp方式音频码流
				pXHClient = boost::make_shared<CNetRtspServerUDP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_RtspServerRecvPushAudio;
				}
			}
			else if (netClientType == NetBaseNetType_NetServerReadMultRecordFile)
			{//连续读取多个录像文件
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CNetServerReadMultRecordFile>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == NetBaseNetType_NetClientWebrtcPlayer)
			{//webrtc播放 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = boost::make_shared<CNetClientWebrtcPlayer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
		} while (pXHClient == NULL);
	}
	catch (const std::exception& e)
	{
		return NULL;
	}

	std::pair<boost::unordered_map<NETHANDLE, CNetRevcBase_ptr>::iterator, bool> ret =
		xh_ABLNetRevcBaseMap.insert(std::make_pair(CltHandle, pXHClient));
	if (!ret.second)
	{
		return NULL;
	}

	return pXHClient;
}


#else
CNetRevcBase_ptr CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptr pXHClient = NULL;
	try
	{
		do
		{
			if (netClientType == NetRevcBaseClient_ServerAccept)
			{
				if (serverHandle == srvhandle_8080)
					pXHClient = std::make_shared<CNetServerHTTP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_554)
					pXHClient = std::make_shared<CNetRtspServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_1935)
					pXHClient = std::make_shared<CNetRtmpServerRecv>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_8088)
					pXHClient = std::make_shared<CNetServerHTTP_FLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_6088)
					pXHClient = std::make_shared<CNetServerWS_FLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_9088)
					pXHClient = std::make_shared<CNetServerHLS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_8089)
					pXHClient = std::make_shared<CNetServerHTTP_MP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_9298)
					pXHClient = std::make_shared<CNetServerRecvAudio>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				else if (serverHandle == srvhandle_10000)
				{//国标单端口输入
					pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient != NULL)
					{
						CNetGB28181RtpServer* gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
						pXHClient->netBaseNetType = NetBaseNetType_GB28181TcpPSInputStream;
						gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength];
						gb28181TCP->m_addStreamProxyStruct.RtpPayloadDataType[0] = 0x31;//PS  
						gb28181TCP->m_addStreamProxyStruct.disableVideo[0] = 0x30;//没有屏蔽视频
						gb28181TCP->m_addStreamProxyStruct.disableAudio[0] = 0x30;//没有屏蔽音频
					}
				}
				else
				{
					CNetRevcBase_ptr gb28181Listen = GetNetRevcBaseClientNoLock(serverHandle);
					if (gb28181Listen && gb28181Listen->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen && gb28181Listen->nMediaClient == 0)
					{//国标TCP 被动方式接入 
						CNetGB28181RtpServer* gb28181TCP = NULL;
						pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, gb28181Listen->m_szShareMediaURL);
						if (pXHClient != NULL)
						{
							pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Server;//国标28181 tcp 方式接收码流 
							gb28181Listen->nMediaClient = CltHandle; //已经有人连接进来

							gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
							if (gb28181TCP)
							{
								strcpy(gb28181TCP->szClientIP, szIP);
								gb28181TCP->nClientPort = nPort;
								gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength]; //在使用前先准备好内存 
							}

							pXHClient->hParent = gb28181Listen->nClient;//记录国标代理句柄号
							pXHClient->m_gbPayload = atoi(gb28181Listen->m_openRtpServerStruct.payload);//更新paylad 
							memcpy((char*)&pXHClient->m_addStreamProxyStruct, (char*)&gb28181Listen->m_addStreamProxyStruct, sizeof(gb28181Listen->m_addStreamProxyStruct));
							memcpy((char*)&pXHClient->m_openRtpServerStruct, (char*)&gb28181Listen->m_openRtpServerStruct, sizeof(gb28181Listen->m_openRtpServerStruct));
							memcpy((char*)&pXHClient->m_h265ConvertH264Struct, (char*)&gb28181Listen->m_h265ConvertH264Struct, sizeof(gb28181Listen->m_h265ConvertH264Struct));//单独指定转码参数
						}
					}
					else if (gb28181Listen && gb28181Listen->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen && gb28181Listen->nMediaClient == 0)
					{//国标 tcp 被动方式 发送 
						pXHClient = std::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, gb28181Listen->m_szShareMediaURL);
						if (pXHClient != NULL)
						{
							pXHClient->hParent = gb28181Listen->nClient;//记录listen的ID 
							pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpTCP_Passive;//国标28181 tcp 被动方式发送码流 
							gb28181Listen->nMediaClient = CltHandle; //已经有人连接进来，只允许一个连接进来 
							memcpy((char*)&pXHClient->m_startSendRtpStruct, (char*)&gb28181Listen->m_startSendRtpStruct, sizeof(pXHClient->m_startSendRtpStruct)); //把listen对象的 m_startSendRtpStruct 拷贝给CNetGB28181RtpClient对象的 m_startSendRtpStruct
							pXHClient->SendFirstRequst();
						}
					}
					else
						return NULL;
				}
			}
			else if (netClientType == NetRevcBaseClient_addStreamProxyControl || netClientType == NetRevcBaseClient_addFFmpegProxyControl)
			{//代理拉流控制包括自研、调用ffmepg 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetClientAddStreamProxy>(netClientType, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->nClient = CltHandle;
			}
			else if (netClientType == NetRevcBaseClient_addPushProxyControl)
			{//代理推流控制 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetClientAddPushProxy>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->nClient = CltHandle;
			}
			else if (netClientType == NetRevcBaseClient_addStreamProxy)
			{//代理拉流
				if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".m3u8") != NULL)
				{//hls 暂时不支持 hls 拉流 
					pXHClient = std::make_shared<CNetClientRecvHttpHLS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".flv") != NULL)
				{//flv 
					pXHClient = std::make_shared<CNetClientRecvFLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (memcmp(szIP, "rtsp://", 7) == 0)
				{//rtsp 
					pXHClient = std::make_shared<CNetClientRecvRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
					if (CltHandle == 0)
					{//连接失败
						WriteLog(Log_Debug, "CreateNetRevcBaseClient()，连接 rtsp 服务器 失败 szURL = %s , szIP = %s ,port = %s ", szIP, pXHClient->m_rtspStruct.szIP, pXHClient->m_rtspStruct.szPort);
						pDisconnectBaseNetFifo.push((unsigned char*)&pXHClient->nClient, sizeof(pXHClient->nClient));
					}
				}
				else if (memcmp(szIP, "rtmp://", 7) == 0)
				{//rtmp
					pXHClient = std::make_shared<CNetClientRecvRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (strstr(szIP, ".mp4") != NULL || strstr(szIP, ".mov") != NULL || strstr(szIP, ".mkv") != NULL || strstr(szIP, ".ts") != NULL || strstr(szIP, ".ps") != NULL || strstr(szIP, ".flv") != NULL || strstr(szIP, ".264") != NULL || strstr(szIP, ".265") != NULL)
				{//本地文件
					CltHandle = XHNetSDK_GenerateIdentifier();
					pXHClient = std::make_shared<CNetClientReadLocalMediaFile>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient;
				}
				else
					return NULL;
			}
			else if (netClientType == NetRevcBaseClient_addFFmpegProxy)
			{//ffmpeg 代理拉流
				if (strstr(szIP, "rtsp://") != NULL || strstr(szIP, "rtmp://") != NULL || strstr(szIP, "http://") != NULL || strstr(szIP, "https://") != NULL || strstr(szIP, ".mp4") != NULL || strstr(szIP, ".mov") != NULL || strstr(szIP, ".mkv") != NULL || strstr(szIP, ".ts") != NULL || strstr(szIP, ".ps") != NULL || strstr(szIP, ".flv") != NULL || strstr(szIP, ".264") != NULL || strstr(szIP, ".265") != NULL)
				{//本地文件
					CltHandle = XHNetSDK_GenerateIdentifier();
					pXHClient = std::make_shared<CNetClientFFmpegRecv>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient;
				}
			}
			else if (netClientType == NetRevcBaseClient_addPushStreamProxy)
			{//代理推流
				if (memcmp(szIP, "rtsp://", 7) == 0)
				{//hls 暂时不支持 hls 拉流 
					pXHClient = std::make_shared<CNetClientSendRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
				else if (memcmp(szIP, "rtmp://", 7) == 0)
				{
					pXHClient = std::make_shared<CNetClientSendRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //把nClient赋值给 CltHandle ,作为关键字 ，如果连接失败，会收到回调通知，在回调通知进行删除即可 
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerUDP)
			{//创建GB28181 的udp接收
				pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerUDP;
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerTCP_Active)
			{//创建GB28181 的TCP 主动连接方式 
				pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient != NULL)
				{
					CNetGB28181RtpServer* gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
					gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength]; //在使用前先准备好内存 
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Active;
					pXHClient->hParent = CltHandle;
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181SendRtpUDP)
			{//创建GB28181 的udp发送
				pXHClient = std::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpUDP;
			}
			else if (netClientType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
			{//创建GB28181 的tcp发送 
				pXHClient = std::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpTCP_Connect;
			}
			else if (netClientType == NetBaseNetType_RecordFile_FMP4)
			{//fmp4录像
				pXHClient = std::make_shared<CStreamRecordFMP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_RecordFile_FMP4;
			}
			else if (netClientType == NetBaseNetType_RecordFile_MP4)
			{//mp4录像
				pXHClient = std::make_shared<CStreamRecordMP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_RecordFile_MP4;
			}
			else if (netClientType == NetBaseNetType_RecordFile_TS)
			{//ts录像
				pXHClient = std::make_shared<CStreamRecordTS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == ReadRecordFileInput_ReadFMP4File)
			{//读取fmp4文件
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CReadRecordFileInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = ReadRecordFileInput_ReadFMP4File;
			}
			else if (netClientType == NetBaseNetType_SnapPicture_JPEG)
			{//抓拍图片
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetClientSnap>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == NetBaseNetType_HttpClient_None_reader)
			{//事件通知1
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNoneReader = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型  
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Not_found)
			{//事件通知2
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNotFound = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Record_mp4)
			{//事件通知3
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordMp4 = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_arrive)
			{//事件通知4
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_not_arrive)
			{//事件通知5
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNotArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_disconnect)
			{//事件通知6
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientDisconnect = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_record_ts)
			{//事件通知7
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordTS = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Record_Progress)
			{//事件通知8
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordProgress = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_ServerStarted)
			{//事件通知9
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nServerStarted = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_ServerKeepalive)
			{//事件通知10
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nServerKeepalive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_DeleteRecordMp4)
			{//事件通知11
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientDeleteRecordMp4 = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_play)
			{//事件通知12
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nPlay = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_publish)
			{//事件通知13
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nPublish = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_iframe_arrive)
			{//事件通知14
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nFrameArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //更新网络类型
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RecvRtpPS_TS)
			{//单端口接收国标 
				pXHClient = std::make_shared<CNetServerRecvRtpTS_PS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181UDPTSStreamInput)
			{//TS 解包形成媒体源
				pXHClient = std::make_shared<CRtpTSStreamInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181UDPPSStreamInput)
			{//PS 解包形成媒体源
				pXHClient = std::make_shared<CRtpPSStreamInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerListen)
			{//国标TCP被动接收的Listen 
				pXHClient = std::make_shared<CNetGB28181Listen>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerListen;
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpSendListen)
			{//国标TCP被动发送的Listen
				pXHClient = std::make_shared<CNetGB28181Listen>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpSendListen;
				}
			}
			else if (netClientType == NetBaseNetType_RtspServerRecvPushVideo)
			{//接收rtsp推流udp方式视频码流
				pXHClient = std::make_shared<CNetRtspServerUDP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_RtspServerRecvPushVideo;
				}
			}
			else if (netClientType == NetBaseNetType_RtspServerRecvPushAudio)
			{//接收rtsp推流udp方式音频码流
				pXHClient = std::make_shared<CNetRtspServerUDP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_RtspServerRecvPushAudio;
				}
			}
			else if (netClientType == NetBaseNetType_NetServerReadMultRecordFile)
			{//连续读取多个录像文件
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetServerReadMultRecordFile>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == NetBaseNetType_NetClientWebrtcPlayer)
			{//webrtc播放 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetClientWebrtcPlayer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
		} while (pXHClient == NULL);
	}
	catch (const std::exception& e)
	{
		return NULL;
	}

	auto ret =
		xh_ABLNetRevcBaseMap.insert(std::make_pair(CltHandle, pXHClient));
	if (!ret.second)
	{
		return NULL;
	}

	return pXHClient;
}


#endif //USE_BOOST
CNetRevcBase_ptr GetNetRevcBaseClient(NETHANDLE CltHandle)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;

	iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		pClient = (*iterator1).second;
		return pClient;
	}
	else
	{
		return NULL;
	}
}

bool  DeleteNetRevcBaseClient(NETHANDLE CltHandle)
{

	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	auto iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);

	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		(*iterator1).second->bRunFlag = false;
		if (((*iterator1).second->netBaseNetType == NetBaseNetType_RtspClientPush || (*iterator1).second->netBaseNetType == NetBaseNetType_RtmpClientPush ||
			(*iterator1).second->netBaseNetType == NetBaseNetType_RtspClientRecv || (*iterator1).second->netBaseNetType == NetBaseNetType_RtmpClientRecv || (*iterator1).second->netBaseNetType == NetBaseNetType_FFmpegRecvNetworkMedia)
			&& (*iterator1).second->bProxySuccessFlag == false)
		{//rtsp\rtmp 代理拉流，rtsp \ rtmp 代理推流
			//如果没有成功过则需要删除父类 
			auto  pParentPtr = GetNetRevcBaseClientNoLock((*iterator1).second->hParent);
			if (pParentPtr && pParentPtr->bProxySuccessFlag == false || (*iterator1).second->m_nXHRtspURLType == XHRtspURLType_RecordPlay)
				pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->hParent, sizeof((*iterator1).second->hParent));
		}

		//关闭国标监听 
		if ((*iterator1).second->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen)
		{
			XHNetSDK_Unlisten((*iterator1).second->nClient);
			if ((*iterator1).second->nMediaClient == 0)
			{//码流没有达到通知
				if (ABL_MediaServerPort.hook_enable == 1 && (*iterator1).second->bUpdateVideoFrameSpeedFlag == false)
				{
					MessageNoticeStruct msgNotice;
					msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_not_arrive;
					sprintf(msgNotice.szMsg, "{\"mediaServerId\":\"%s\",\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"key\":%llu}", ABL_MediaServerPort.mediaServerID, (*iterator1).second->m_addStreamProxyStruct.app, (*iterator1).second->m_addStreamProxyStruct.stream, (*iterator1).second->netBaseNetType, (*iterator1).second->nClient);
					pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
				}
			}
			if ((*iterator1).second->nMediaClient > 0)
				pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->nMediaClient, sizeof((*iterator1).second->nMediaClient));
		}
		else if ((*iterator1).second->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen)
		{
			XHNetSDK_Unlisten((*iterator1).second->nClient);
			WriteLog(Log_Debug, " XHNetSDK_Unlisten() , nMediaClient = %llu  ", (*iterator1).second->nClient);
		}
		else //在此处关闭，确保boost:asio 单线程状态下 close(pSocket) ;
			XHNetSDK_Disconnect((*iterator1).second->nClient);

		//把依赖的父类删除掉
		if ((*iterator1).second->hParent > 0)
		{
			auto  pParentPtr = GetNetRevcBaseClientNoLock((*iterator1).second->hParent);

			if (pParentPtr != NULL)
			{//不是代理推流、代理拉流的就可以删除父类,代理拉流、代理推流的不能删除 ，需要重连次数达到配置文件所配置的数量 
				if (!(pParentPtr->netBaseNetType == NetBaseNetType_addStreamProxyControl || pParentPtr->netBaseNetType == NetBaseNetType_addPushProxyControl))
					pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->hParent, sizeof((*iterator1).second->hParent));
			}
		}

		xh_ABLNetRevcBaseMap.erase(iterator1);
		return true;
	}
	else
	{
		return false;
	}
}

/*
 功能：
	检查端口是否已经使用
参数：
  int   nPort,      端口
  int   nPortType,  类型  1 openRtpServe , 2 sartSendRtp
  bool  bLockFlag   是否锁住
*/
bool  CheckPortAlreadyUsed(int nPort, int nPortType, bool bLockFlag)
{
	if (bLockFlag)
		std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	bool                 bRet = false;
	CNetRevcBase_ptr     pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (nPortType == 1)
		{
			if (pClient->netBaseNetType == NetRevcBaseClient__NetGB28181Proxy &&
				atoi(pClient->m_openRtpServerStruct.port) == nPort
				)
			{//已经占用了 nPort;
				bRet = true;
				break;
			}
		}
		else if (nPortType == 2)
		{
			if ((pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect) &&
				atoi(pClient->m_startSendRtpStruct.src_port) == nPort
				)
			{//已经占用了 nPort;
				bRet = true;
				break;
			}
		}
	}
	WriteLog(Log_Debug, "CheckPortAlreadyUsed() bRet = %d  ", bRet);
	return bRet;
}

/*
功能：
   检查SSRC是否已经使用
参数：
	int   nSSRC,      ssrc
	bool  bLockFlag   是否锁住
*/
bool  CheckSSRCAlreadyUsed(int nSSRC, bool bLockFlag)
{
	if (bLockFlag)
		std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	bool                 bRet = false;
	CNetRevcBase_ptr     pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if ((pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect) &&
			atoi(pClient->m_startSendRtpStruct.ssrc) == nSSRC
			)
		{//已经占用了 nPort;
			bRet = true;
			break;
		}
	}
	WriteLog(Log_Debug, "CheckSSRCAlreadyUsed() bRet = %d  ", bRet);
	return bRet;
}

/*
功能：
检查 dst_url 、 dst_port 是否已经使用
参数：
char  dst_url,    目标IP
int   dst_port    目标端口
bool  bLockFlag   是否锁住
*/
bool  CheckDst_url_portAlreadyUsed(char* dst_url, int dst_port, bool bLockFlag)
{
	if (bLockFlag)
		std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	bool                 bRet = false;
	CNetRevcBase_ptr     pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if ((pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect) &&
			atoi(pClient->m_startSendRtpStruct.dst_port) == dst_port && strcmp(pClient->m_startSendRtpStruct.dst_url, dst_url) == 0
			)
		{//已经占用了 nPort;
			bRet = true;
			break;
		}
	}
	WriteLog(Log_Debug, "CheckDst_url_portAlreadyUsed() bRet = %d  ", bRet);
	return bRet;
}

//查找某一个网络类型的对象总数
int  GetNetRevcBaseClientCountByNetType(NetBaseNetType netType, bool bLockFlag)
{
	if (bLockFlag)
		std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                nCount = 0;
	CNetRevcBase_ptr   pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (pClient->netBaseNetType == netType && pClient->bSnapSuccessFlag == false)
		{//是抓拍对象，并且尚未抓拍成功
			nCount++;
		}
	}
	WriteLog(Log_Debug, "GetNetRevcBaseClientCountByNetType() netType = %d , nCount = %d  ", netType, nCount);
	return nCount;
}

//把所有对象装入链表，准备删除 
int  FillNetRevcBaseClientFifo()
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                nCount = 0;
	CNetRevcBase_ptr   pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
	}
	return nCount;
}

//根据ShareMediaURL、NetBaseNetType 查找对象 
CNetRevcBase_ptr  GetNetRevcBaseClientByNetTypeShareMediaURL(NetBaseNetType netType, char* ShareMediaURL, bool bLockFlag)
{
	if (bLockFlag)
		std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (pClient->netBaseNetType == netType && strcmp(ShareMediaURL, pClient->m_szShareMediaURL) == 0)
		{
			WriteLog(Log_Debug, "GetNetRevcBaseClientByNetTypeShareMediaURL() netType = %d , nClient = %llu ", netType, pClient->nClient);
			return pClient;
		}
	}
	return  NULL;
}

//查找推流url是否存在
bool  QueryMediaSource(char* pushURL)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	bool   bFind = false;
	CNetRevcBase_ptr   pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (pClient->netBaseNetType == NetBaseNetType_addStreamProxyControl)
		{
			if (strcmp(pushURL, pClient->m_addPushProxyStruct.url) == 0)
			{
				bFind = true;
				WriteLog(Log_Debug, "QueryMediaSource() 推流地址已经存在 url = %s ", pushURL);
				break;
			}
		}
	}
	return bFind;
}

//检测网络断线 ，发送M3u8请求 
int  CheckNetRevcBaseClientDisconnect()
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                           nDiconnectCount = 0;
	int                           nPrintCount = 0;
	char                          szLine[string_length_4096] = { 0 };
	char                          szTemp[string_length_512] = { 0 };
	int                           netBaseNetType;

	ABL_nPrintCheckNetRevcBaseClientDisconnect++;
	if (ABL_nPrintCheckNetRevcBaseClientDisconnect % 20 == 0)//1分钟打印一次
		WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() 当前服务器对象总数 nSize = %llu ", xh_ABLNetRevcBaseMap.size());

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		netBaseNetType = ((*iterator1).second)->netBaseNetType;
		if (ABL_nPrintCheckNetRevcBaseClientDisconnect % 30 == 0)//1分钟打印一次
		{
			nPrintCount++;
			sprintf(szTemp, "[ nClient = %llu netType = %d ] , ", ((*iterator1).second)->nClient, ((*iterator1).second)->netBaseNetType);
			strcat(szLine, szTemp);
			if (nPrintCount >= 4)
			{
				WriteLog(Log_Debug, "当前对象信息 %s ", szLine);
				memset(szLine, 0x00, sizeof(szLine));
				nPrintCount = 0;
			}
		}

		//检查事件连接，如果设置hook_enable = 0 立即删除
		if (ABL_MediaServerPort.hook_enable == 0 && ((((*iterator1).second)->netBaseNetType >= NetBaseNetType_HttpClient_None_reader && ((*iterator1).second)->netBaseNetType <= NetBaseNetType_HttpClient_Record_Progress) ||
			(((*iterator1).second)->netBaseNetType >= NetBaseNetType_HttpClient_ServerStarted && ((*iterator1).second)->netBaseNetType <= NetBaseNetType_HttpClient_on_publish)))
			pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));

		if (
			(((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerRecvPush && ((*iterator1).second)->m_RtspNetworkType == RtspNetworkType_TCP) ||   //接收rtsp推流上来(tcp）方式
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerRecvPushVideo || //rtsp 接收视频
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerRecvPushAudio || //rtsp 接收音频
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerSendPush ||   //rtsp 发送
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpServerRecvPush ||   //接收RTMP推流

			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientPush ||   //rtsp 推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientPush ||   //rtmp 推流

			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv ||      //代理接收Rtsp推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientRecv ||      //代理接收Rtmp推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFlvClientRecv ||   //代理接收HttpFlv推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv ||    //代理接收HttpHLS推流

			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP ||   //GB28181 以UDP方式接收 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || //GB28181 以TCP方式接收 

			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || //国标UDP推流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect ||//国标TCP推流 

			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFLVServerSendPush ||//监测http-flv发流 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_WsFLVServerSendPush ||//监测ws-flv发流 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpMP4ServerSendPush || //监测MP4发流 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpServerSendPush ||  //检测rtmp发流 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181UDPTSStreamInput || // 单端口 TS 流接入
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181UDPPSStreamInput || // 单端口 PS 流接入
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || //国标TCP方式接收 tcp主动连接方式 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_SnapPicture_JPEG || //抓拍对象超时检测
			((*iterator1).second)->netBaseNetType == NetBaseNetType_WebSocektRecvAudio || //websocket协议接入pcm音频流
			((*iterator1).second)->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream ||//通过10000端口TCP方式接收国标PS流接入
			((*iterator1).second)->netBaseNetType == NetBaseNetType_FFmpegRecvNetworkMedia ||//通过调用ffmpeg拉取 rtmp,flv,mp4,hls 码流 接入
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetClientWebrtcPlayer //webRTC 播放 
			)
		{//现在检测 HLS 网络断线 ，还可以增加别的类型检测 
			if (((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv)
			{//Hls 主动拉流
				((*iterator1).second)->RequestM3u8File();
			}

			if (((*iterator1).second)->m_bPauseFlag == false && ((*iterator1).second)->nRecvDataTimerBySecond >= (ABL_MediaServerPort.MaxDiconnectTimeoutSecond / 2))
			{//不是国标回放暂停、也不是rtsp回放暂停
				nDiconnectCount++;
				((*iterator1).second)->bRunFlag = false;
				WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() nClient = %llu 检测到网络异常断开1 ", ((*iterator1).second)->nClient);

				pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));

				if (((*iterator1).second)->m_nXHRtspURLType == XHRtspURLType_RecordPlay)
					pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->hParent, sizeof((unsigned char*)&((*iterator1).second)->hParent));
			}
			//发送rtcp包
			if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv)
			{
				CNetClientRecvRtsp* pRtspClient = (CNetClientRecvRtsp*)(*iterator1).second.get();
				if (pRtspClient->bSendRRReportFlag)
				{
					if (GetTickCount64() - pRtspClient->nCurrentTime >= 1000 * 3)
					{
						pRtspClient->SendRtcpReportData();
					}
				}

				//发送options 心跳包 
				if (atoi(pRtspClient->m_addStreamProxyStruct.optionsHeartbeat) == 1)
					pRtspClient->SendOptionsHeartbeat();
			}

			//定期更新动态域名的IP
			if (((*iterator1).second)->tUpdateIPTime - GetTickCount64() >= 1000 * 15)
			{
				((*iterator1).second)->tUpdateIPTime = GetTickCount64();
				((*iterator1).second)->ConvertDemainToIPAddress();
			}

			//快速检测录像下载完毕
			if (((*iterator1).second)->nRecvDataTimerBySecond >= 3 && ((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerSendPush && ((*iterator1).second)->m_bPauseFlag == false && ((*iterator1).second)->nReplayClient > 0)
			{
				char szQuitText[128] = { 0 };
				strcpy(szQuitText, "ABL_ANNOUNCE_QUIT:2021");
				sprintf(((*iterator1).second)->szReponseTemp, "ANNOUNCE RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nContent-Type: text/parameters\r\nContent-Length: %d\r\n\r\n%s", 8, MediaServerVerson, strlen(szQuitText), szQuitText);
				WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() nClient = %llu 录像发送完毕", ((*iterator1).second)->nClient);
				XHNetSDK_Write(((*iterator1).second)->nClient, (unsigned char*)((*iterator1).second)->szReponseTemp, strlen(((*iterator1).second)->szReponseTemp), 1);

				pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
			}

			if (((*iterator1).second)->m_bPauseFlag == false)
				((*iterator1).second)->nRecvDataTimerBySecond++;  //不是暂停，计时
			else
				((*iterator1).second)->nRecvDataTimerBySecond = 0;//已经暂停，不再计时
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen && ((*iterator1).second)->bUpdateVideoFrameSpeedFlag == false)
		{//检测国标代理拉流 TCP 
			if ((GetTickCount64() - ((*iterator1).second)->nCreateDateTime) >= (1000 * (ABL_MediaServerPort.MaxDiconnectTimeoutSecond / 2)))
			{//在超时的时间范围内，码流尚未到达 
				WriteLog(Log_Debug, "国标TCP接收超时 nClient = %llu , app = %s ,stream = %s , port = %s ", ((*iterator1).second)->nClient, ((*iterator1).second)->m_openRtpServerStruct.app, ((*iterator1).second)->m_openRtpServerStruct.stream_id, ((*iterator1).second)->m_openRtpServerStruct.port);
				pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
			}
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen && ((*iterator1).second)->nMediaClient == 0)
		{//检测国标tcp被动方式发流
			if ((GetTickCount64() - ((*iterator1).second)->nCreateDateTime) >= (1000 * (ABL_MediaServerPort.MaxDiconnectTimeoutSecond / 2)))
			{//在超时的时间范围内,没有连接进来
				WriteLog(Log_Debug, "国标gb28181 tcp 被动方式发流超时 nClient = %llu , app = %s ,stream = %s , port = %s ", ((*iterator1).second)->nClient, ((*iterator1).second)->m_startSendRtpStruct.app, ((*iterator1).second)->m_startSendRtpStruct.stream, ((*iterator1).second)->m_startSendRtpStruct.src_port);
				pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
			}
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_addStreamProxyControl || ((*iterator1).second)->netBaseNetType == NetBaseNetType_addPushProxyControl)
		{//控制代理拉流、代理推流,检测代理拉流是否有断线
			CNetRevcBase_ptr pClient = GetNetRevcBaseClientNoLock(((*iterator1).second)->nMediaClient);
			if (pClient == NULL)
			{//已经断线，需要重新连接 
				if (((*iterator1).second)->bRecordProxyDisconnectTimeFlag == false)
				{
					((*iterator1).second)->nProxyDisconnectTime = GetTickCount64();
					((*iterator1).second)->bRecordProxyDisconnectTimeFlag = true;
				}

				if (GetTickCount64() - ((*iterator1).second)->nProxyDisconnectTime >= 1000 * 15)
				{
					((*iterator1).second)->bRecordProxyDisconnectTimeFlag = false;

					((*iterator1).second)->nReConnectingCount++; //重连次数累积 

					if (((*iterator1).second)->nReConnectingCount > ABL_MediaServerPort.nReConnectingCount)
					{
						WriteLog(Log_Debug, "nClient = %llu , nMediaClient = %llu ,url: %s 重连次数已经达到 %llu 次，需要断开 ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReConnectingCount);
						pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
					}
					else
					{
						sprintf(((*iterator1).second)->szResponseBody, "{\"code\":%d,\"memo\":\"Network Connnect[ %s ] Timeout .\",\"key\":%llu}", IndexApiCode_ConnectTimeout, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nClient);
						((*iterator1).second)->ResponseHttp(((*iterator1).second)->nClient_http, ((*iterator1).second)->szResponseBody, false);

						//如果从未成功过则删除代理对象句柄
						if (((*iterator1).second)->bProxySuccessFlag == true && ((*iterator1).second)->m_nXHRtspURLType != XHRtspURLType_RecordPlay)
						{
							WriteLog(Log_Debug, "nClient = %llu , nMediaClient = %llu 检测到网络异常断开2 , %s ，现在执行第 %llu 次重连  ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReConnectingCount);
							pReConnectStreamProxyFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
						}
						else
							pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
					}
				}
			}
			else
			{
				//如果是超时断开的，往往就是原来代理成功过的，需要无限次重连 
				if (((*iterator1).second)->bProxySuccessFlag == false && pClient->bUpdateVideoFrameSpeedFlag == true)
					((*iterator1).second)->bProxySuccessFlag = true;

				//如果成功，重连次数复位 
				if (((*iterator1).second)->nReConnectingCount != 0 && pClient->bUpdateVideoFrameSpeedFlag == true)
				{
					((*iterator1).second)->nReConnectingCount = 0;
					WriteLog(Log_Debug, "nClient = %llu , nMediaClient = %llu ,url %s ，重连次数复位为 0 ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url);
				}
			}
		}

		//单独处理各种代理拉流、推流时，连接超时回复http请求
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv || ((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientRecv || ((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFlvClientRecv || ((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv ||
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientPush || ((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientPush
			)
		{
			if (!((*iterator1).second)->bResponseHttpFlag && GetTickCount64() - ((*iterator1).second)->nCreateDateTime >= 15000)
			{//连接超时9秒，尚未回复http请求，一律回复连接超时
				sprintf(((*iterator1).second)->szResponseBody, "{\"code\":%d,\"memo\":\"Network Connnect[ %s : %s ] Timeout .\",\"key\":%d}", IndexApiCode_ConnectTimeout, ((*iterator1).second)->m_rtspStruct.szIP, ((*iterator1).second)->m_rtspStruct.szPort, 0);
				((*iterator1).second)->ResponseHttp(((*iterator1).second)->nClient_http, ((*iterator1).second)->szResponseBody, false);

				//删除掉代理拉流、推流
				CNetRevcBase_ptr pParentPtr = GetNetRevcBaseClientNoLock(((*iterator1).second)->hParent);
				if (pParentPtr)
				{
					if (pParentPtr->bProxySuccessFlag == false)
						pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->hParent, sizeof(((*iterator1).second)->hParent));
				}
			}
		}

		//抓拍超时检测 
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_SnapPicture_JPEG && ((*iterator1).second)->bSnapSuccessFlag == false)
		{
			if (GetTickCount64() - ((*iterator1).second)->nPrintTime >= 1000 * ((*iterator1).second)->timeout_sec)
				pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
		}

		//抓拍对象超过生存时长检测
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_SnapPicture_JPEG && (GetTickCount64() - ((*iterator1).second)->nPrintTime) >= 1000 * ABL_MediaServerPort.snapObjectDuration)
		{
			WriteLog(Log_Debug, "抓拍对象已经超过最长的生存时长 %d 秒 ,加入删除链表等待销毁，nClient = %llu ", ABL_MediaServerPort.snapObjectDuration, ((*iterator1).second)->nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
		}

		//消息通知超时检测
		if ((netBaseNetType == NetBaseNetType_HttpClient_None_reader || netBaseNetType == NetBaseNetType_HttpClient_Not_found || netBaseNetType == NetBaseNetType_HttpClient_Record_mp4 || netBaseNetType == NetBaseNetType_HttpClient_Record_Progress || netBaseNetType == NetBaseNetType_HttpClient_on_stream_arrive ||
			netBaseNetType == NetBaseNetType_HttpClient_on_stream_not_arrive || netBaseNetType == NetBaseNetType_HttpClient_on_stream_disconnect || netBaseNetType == NetBaseNetType_HttpClient_on_record_ts || netBaseNetType == NetBaseNetType_HttpClient_ServerStarted || netBaseNetType == NetBaseNetType_HttpClient_ServerKeepalive || netBaseNetType == NetBaseNetType_HttpClient_DeleteRecordMp4 ||
			netBaseNetType == NetBaseNetType_HttpClient_on_play || netBaseNetType == NetBaseNetType_HttpClient_on_publish || netBaseNetType == NetBaseNetType_HttpClient_on_iframe_arrive) && GetTickCount64() - ((*iterator1).second)->nCreateDateTime >= 15000)
		{
			WriteLog(Log_Debug, "消息通知已经超过 15 秒 ,加入删除链表等待销毁，nClient = %llu ", ((*iterator1).second)->nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
		}
	}

	//服务器心跳
	if (ABL_MediaServerPort.hook_enable == 1 && (GetTickCount64() - ABL_MediaServerPort.nServerKeepaliveTime) >= 1000 * ABL_MediaServerPort.keepaliveDuration)
	{
		ABL_MediaServerPort.nServerKeepaliveTime = GetTickCount64();

		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_ServerKeepalive;

#ifdef OS_System_Windows
		SYSTEMTIME st;
		GetLocalTime(&st);
		sprintf(msgNotice.szMsg, "{\"localipAddress\":\"%s\",\"mediaServerId\":\"%s\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.mediaServerID, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
		time_t now;
		time(&now);
		struct tm* local;
		local = localtime(&now);
		sprintf(msgNotice.szMsg, "{\"localipAddress\":\"%s\",\"mediaServerId\":\"%s\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.mediaServerID, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
#endif
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}

	if (strlen(szLine))
		WriteLog(Log_Debug, "当前对象信息 %s ", szLine);

	return nDiconnectCount;
}

void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	void* address)
{
	char           temp[256] = { 0 };
	unsigned short nPort = 5567;
	uint64_t       hParent;
	int            nAcceptNumvber;

	if (address)
	{
		sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(address);
		sprintf(temp, "%s", ::inet_ntoa(addr->sin_addr));
		nPort = ::ntohs(addr->sin_port);
	}

	if (CreateNetRevcBaseClient(NetRevcBaseClient_ServerAccept, srvhandle, clihandle, temp, nPort, "") == NULL)
		XHNetSDK_Disconnect(clihandle);
}
#
void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address)
{

	CNetRevcBase_ptr  pBasePtr = GetNetRevcBaseClient(clihandle);
	if (pBasePtr != NULL)
	{
		pBasePtr->InputNetData(srvhandle, clihandle, data, datasize, address);
		pBasePtr->ProcessNetData();
	}
}

void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle)
{
	WriteLog(Log_Debug, "onclose() nClient = %llu 客户端断开 srvhandle = %llu", clihandle, srvhandle);

	DeleteNetRevcBaseClient(clihandle);
}

void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result, uint16_t nLocalPort)
{
	if (result == 0)
	{
		CNetRevcBase_ptr pClient = GetNetRevcBaseClient(clihandle);
		if (pClient)
		{
			WriteLog(Log_Debug, "clihandle = %llu ,URL: %s ,连接失败 result: %d ", clihandle, pClient->m_rtspStruct.szSrcRtspPullUrl, result);
			if (pClient->netBaseNetType == NetBaseNetType_RtspClientRecv || pClient->netBaseNetType == NetBaseNetType_RtmpClientRecv || pClient->netBaseNetType == NetBaseNetType_HttpFlvClientRecv ||
				pClient->netBaseNetType == NetBaseNetType_HttpHLSClientRecv || pClient->netBaseNetType == NetBaseNetType_RtspClientPush || pClient->netBaseNetType == NetBaseNetType_RtmpClientPush ||
				pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active)
			{//rtsp 代理拉流失败
				sprintf(pClient->szResponseBody, "{\"code\":%d,\"memo\":\"Network Connect [%s : %s] Failed .\",\"key\":%llu}", IndexApiCode_ConnectFail, pClient->m_rtspStruct.szIP, pClient->m_rtspStruct.szPort, pClient->hParent);
				pClient->ResponseHttp(pClient->nClient_http, pClient->szResponseBody, false);

				//判断是否成功过，如果尚未成功，则立即删除 ，如果成功过则无限次重连
				CNetRevcBase_ptr pParent = GetNetRevcBaseClient(pClient->hParent);
				if (pParent != NULL)
				{
					if (pParent->bProxySuccessFlag == false)
						pDisconnectBaseNetFifo.push((unsigned char*)&pClient->hParent, sizeof(pClient->hParent));
				}
			}
		}

		pDisconnectBaseNetFifo.push((unsigned char*)&clihandle, sizeof(clihandle));
	}
	else if (result == 1)
	{//如果链接成功，发送第一个请求
		CNetRevcBase_ptr pClient = GetNetRevcBaseClient(clihandle);
		if (pClient)
		{
			WriteLog(Log_Debug, "clihandle = %llu ,URL: %s , 连接成功 result: %d ", clihandle, pClient->m_rtspStruct.szSrcRtspPullUrl, result);
			pClient->bConnectSuccessFlag = true;//连接成功
			pClient->nClientPort = ntohs(nLocalPort);//记下本地端口号
			pClient->SendFirstRequst();
		}
	}
}
#ifdef USE_BOOST
//创建 事件通知http Client 对象 
boost::shared_ptr<CNetRevcBase> CreateHttpClientFunc(int nMsgType)
{
	boost::shared_ptr<CNetRevcBase> pMsgClient = NULL;
#else
//创建 事件通知http Client 对象 
std::shared_ptr<CNetRevcBase> CreateHttpClientFunc(int nMsgType)
{
	std::shared_ptr<CNetRevcBase> pMsgClient = NULL;
#endif//USE_BOOST

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_None_reader && strlen(ABL_MediaServerPort.on_stream_none_reader) > 20 && memcmp(ABL_MediaServerPort.on_stream_none_reader, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_None_reader, 0, 0, ABL_MediaServerPort.on_stream_none_reader, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_Not_found && strlen(ABL_MediaServerPort.on_stream_not_found) > 20 && memcmp(ABL_MediaServerPort.on_stream_not_found, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_Not_found, 0, 0, ABL_MediaServerPort.on_stream_not_found, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_Record_mp4 && strlen(ABL_MediaServerPort.on_record_mp4) > 20 && memcmp(ABL_MediaServerPort.on_record_mp4, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_Record_mp4, 0, 0, ABL_MediaServerPort.on_record_mp4, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_DeleteRecordMp4 && strlen(ABL_MediaServerPort.on_delete_record_mp4) > 20 && memcmp(ABL_MediaServerPort.on_delete_record_mp4, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_DeleteRecordMp4, 0, 0, ABL_MediaServerPort.on_delete_record_mp4, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_Record_Progress && strlen(ABL_MediaServerPort.on_record_progress) > 20 && memcmp(ABL_MediaServerPort.on_record_progress, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_Record_Progress, 0, 0, ABL_MediaServerPort.on_record_progress, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_stream_arrive && strlen(ABL_MediaServerPort.on_stream_arrive) > 20 && memcmp(ABL_MediaServerPort.on_stream_arrive, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_stream_arrive, 0, 0, ABL_MediaServerPort.on_stream_arrive, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_stream_not_arrive && strlen(ABL_MediaServerPort.on_stream_not_arrive) > 20 && memcmp(ABL_MediaServerPort.on_stream_not_arrive, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_stream_not_arrive, 0, 0, ABL_MediaServerPort.on_stream_not_arrive, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_stream_disconnect && strlen(ABL_MediaServerPort.on_stream_disconnect) > 20 && memcmp(ABL_MediaServerPort.on_stream_disconnect, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_stream_disconnect, 0, 0, ABL_MediaServerPort.on_stream_disconnect, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_record_ts && strlen(ABL_MediaServerPort.on_record_ts) > 20 && memcmp(ABL_MediaServerPort.on_record_ts, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_record_ts, 0, 0, ABL_MediaServerPort.on_record_ts, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_ServerStarted && strlen(ABL_MediaServerPort.on_server_started) > 20 && memcmp(ABL_MediaServerPort.on_server_started, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_ServerStarted, 0, 0, ABL_MediaServerPort.on_server_started, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_ServerKeepalive && strlen(ABL_MediaServerPort.on_server_keepalive) > 20 && memcmp(ABL_MediaServerPort.on_server_keepalive, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_ServerKeepalive, 0, 0, ABL_MediaServerPort.on_server_keepalive, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_play && strlen(ABL_MediaServerPort.on_play) > 20 && memcmp(ABL_MediaServerPort.on_play, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_play, 0, 0, ABL_MediaServerPort.on_play, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_publish && strlen(ABL_MediaServerPort.on_publish) > 20 && memcmp(ABL_MediaServerPort.on_publish, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_publish, 0, 0, ABL_MediaServerPort.on_publish, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_iframe_arrive && strlen(ABL_MediaServerPort.on_stream_iframe_arrive) > 20 && memcmp(ABL_MediaServerPort.on_stream_iframe_arrive, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_iframe_arrive, 0, 0, ABL_MediaServerPort.on_stream_iframe_arrive, 0, "");

	return pMsgClient;
}

//一些事务处理 
void* ABLMedisServerProcessThread(void* lpVoid)
{
	int nDeleteBreakTimer = 0;
	int nCheckNetRevcBaseClientDisconnectTime = 0;
	int nReConnectStreamProxyTimer = 0;
	int nCreateHttpClientTimer = 0;
	int DeleteExpireM3u8FileTimer = 0;
	ABL_bExitMediaServerRunFlag = false;
	unsigned char* pData = NULL;
	char           szDeleteMediaSource[512] = { 0 };
	int            nLength;
	uint64_t       nClient;
	MessageNoticeStruct msgNotice;

	while (ABL_bMediaServerRunFlag)
	{
		//检测网络异常断开，执行一些清理工作 
		if (nCheckNetRevcBaseClientDisconnectTime >= 20)
		{
			nCheckNetRevcBaseClientDisconnectTime = 0;
			CheckNetRevcBaseClientDisconnect();
		}

		//处理消息通知
		while ((pData = pMessageNoticeFifo.pop(&nLength)) != NULL)
		{
			if (nLength > 0)
			{
				memset((char*)&msgNotice, 0x00, sizeof(msgNotice));
				memcpy((char*)&msgNotice, pData, nLength);

				auto pMsgClient = CreateHttpClientFunc(msgNotice.nClient);
				if (pMsgClient != NULL)
					memcpy((char*)&pMsgClient->msgNotice, (char*)&msgNotice, sizeof(msgNotice));
			}
			pMessageNoticeFifo.pop_front();
		}

		//代理拉流重连
		if (nReConnectStreamProxyTimer >= 10)
		{
			nReConnectStreamProxyTimer = 0;
			while ((pData = pReConnectStreamProxyFifo.pop(&nLength)) != NULL)
			{
				if (nLength == sizeof(nClient))
				{
					memcpy((char*)&nClient, pData, sizeof(nClient));
					if (nClient > 0)
					{
						CNetRevcBase_ptr pClient = GetNetRevcBaseClient(nClient);
						if (pClient)
							pClient->SendFirstRequst(); //执行重连
					}
				}

				pReConnectStreamProxyFifo.pop_front();
			}
		}

		//删除过期的M3u8文件 
		if (DeleteExpireM3u8FileTimer >= 10 * 180)
		{
			DeleteExpireM3u8FileTimer = 0;
			DeleteExpireM3u8File();
		}

		nDeleteBreakTimer++;
		nCheckNetRevcBaseClientDisconnectTime++;
		nReConnectStreamProxyTimer++;
		nCreateHttpClientTimer++;
		DeleteExpireM3u8FileTimer++;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//Sleep(100);
	}

	FillNetRevcBaseClientFifo();//把所有对象装入链表，准备删除

	while ((pData = pDisconnectBaseNetFifo.pop(&nLength)) != NULL)
	{
		if (nLength == sizeof(nClient))
		{
			memcpy((char*)&nClient, pData, sizeof(nClient));
			if (nClient >= 0)
			{
				DeleteClientMediaStreamSource(nClient);//移除媒体拷贝
				DeleteNetRevcBaseClient(nClient);//执行删除 
			}
		}

		pDisconnectBaseNetFifo.pop_front();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		//Sleep(5);
	}

	ABL_bExitMediaServerRunFlag = true;
	return 0;
}

//列表里面的对象增加静音
void  SendToMapFromMutePacketList()
{
	for (int i = 0; i < nMaxAddMuteListNumber; i++)
	{
		if (ArrayAddMutePacketList[i] > 0)
		{
			CNetRevcBase_ptr  pClient = GetNetRevcBaseClient(ArrayAddMutePacketList[i]);
			if (pClient != NULL)
			{
				pClient->AddMuteAACBuffer();
				pClient->SendAudio();
			}
		}
	}
}

//快速删除资源线程
void* ABLMedisServerFastDeleteThread(void* lpVoid)
{
	unsigned char* pData = NULL;
	int            nLength;
	uint64_t       nClient;

	while (ABL_bMediaServerRunFlag)
	{
		//快速删除
		while ((pData = pDisconnectBaseNetFifo.pop(&nLength)) != NULL)
		{
			if (nLength == sizeof(nClient))
			{
				memcpy((char*)&nClient, pData, sizeof(nClient));
				if (nClient >= 0)
				{
					DeleteNetRevcBaseClient(nClient);//执行删除 
				}
			}

			pDisconnectBaseNetFifo.pop_front();
		}

		SendToMapFromMutePacketList();
		std::this_thread::sleep_for(std::chrono::milliseconds(64));
		//Sleep(64);
	}
	return 0;
}

//获取当前路径
#ifdef OS_System_Windows

void malloc_trim(int n)
{
}

bool   ABLDeleteFile(char* szFileName)
{
	return ::DeleteFile(szFileName);
}

bool GetMediaServerCurrentPath(char* szCurPath)
{
	char    szPath[255] = { 0 };
	string  strTemp;
	int     nPos;

	GetModuleFileName(NULL, szPath, sizeof(szPath));
	strTemp = szPath;

	nPos = strTemp.rfind("\\", strlen(szPath));
	if (nPos >= 0)
	{
		memcpy(szCurPath, szPath, nPos + 1);
		return true;
	}
	else
		return false;
}

bool GetLocalAdaptersInfo(string& strIPList)
{
	//IP_ADAPTER_INFO结构体
	PIP_ADAPTER_INFO pIpAdapterInfo = NULL;
	pIpAdapterInfo = new IP_ADAPTER_INFO;

	//结构体大小
	unsigned long ulSize = sizeof(IP_ADAPTER_INFO);

	//获取适配器信息
	int nRet = GetAdaptersInfo(pIpAdapterInfo, &ulSize);

	if (ERROR_BUFFER_OVERFLOW == nRet)
	{
		//空间不足，删除之前分配的空间
		delete[]pIpAdapterInfo;

		//重新分配大小
		pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[ulSize];

		//获取适配器信息
		nRet = GetAdaptersInfo(pIpAdapterInfo, &ulSize);

		//获取失败
		if (ERROR_SUCCESS != nRet)
		{
			if (pIpAdapterInfo != NULL)
			{
				delete[]pIpAdapterInfo;
			}
			return FALSE;
		}
	}

	//MAC 地址信息
	char szMacAddr[20];
	//赋值指针
	PIP_ADAPTER_INFO pIterater = pIpAdapterInfo;
	while (pIterater)
	{
		//cout << "网卡名称：" << pIterater->AdapterName << endl;

		//cout << "网卡描述：" << pIterater->Description << endl;

		sprintf_s(szMacAddr, 20, "%02X-%02X-%02X-%02X-%02X-%02X",
			pIterater->Address[0],
			pIterater->Address[1],
			pIterater->Address[2],
			pIterater->Address[3],
			pIterater->Address[4],
			pIterater->Address[5]);

		//cout << "MAC 地址：" << szMacAddr << endl;
		//cout << "IP地址列表：" << endl << endl;

		//指向IP地址列表
		PIP_ADDR_STRING pIpAddr = &pIterater->IpAddressList;
		while (pIpAddr)
		{
			//cout << "IP地址：  " << pIpAddr->IpAddress.String << endl;
			//cout << "子网掩码：" << pIpAddr->IpMask.String << endl;

			if (!(strcmp(pIpAddr->IpAddress.String, "127.0.0.1") == 0 || strcmp(pIpAddr->IpAddress.String, "0.0.0.0") == 0))
			{
				strIPList += pIpAddr->IpAddress.String;
				strIPList += ",";
			}

			//指向网关列表
			PIP_ADDR_STRING pGateAwayList = &pIterater->GatewayList;
			while (pGateAwayList)
			{
				//cout << "网关：    " << pGateAwayList->IpAddress.String << endl;
				pGateAwayList = pGateAwayList->Next;
			}
			pIpAddr = pIpAddr->Next;
		}
		//cout << endl << "--------------------------------------------------" << endl;
		pIterater = pIterater->Next;
	}

	//清理
	if (pIpAdapterInfo)
	{
		delete[]pIpAdapterInfo;
	}

	WriteLog(Log_Debug, "strIPList = %s ", strIPList.c_str());
	return true;
}

//根据录像路径查找所有录像文件 - windows
void FindHistoryRecordFile(char* szRecordPath)
{
	char tempFileFind[MAX_PATH];
	WIN32_FIND_DATA fd = { 0 };
	WIN32_FIND_DATA fd2 = { 0 };
	WIN32_FIND_DATA fd3 = { 0 };
	bool bFindFlag = true;
	char szApp[256] = { 0 }, szStream[string_length_512] = { 0 };//现在只支持2级路径
	char szDeleteFile[string_length_512] = { 0 };

	//查找文件
	sprintf(tempFileFind, "%s%s", szRecordPath, "*.*");

	HANDLE hFind = FindFirstFile(tempFileFind, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		WriteLog(Log_Debug, "FindHistoryRecordFile ，配置的录像路径 %s ,没有找到任何文件！ ", szRecordPath);
		return;
	}

	while (bFindFlag)
	{
		bFindFlag = FindNextFile(hFind, &fd);
		if (bFindFlag && !(strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) && fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			//WriteLog(Log_Debug, "FindHistoryRecordFile ，路径 %s%s  ", szRecordPath, fd.cFileName);

			sprintf(tempFileFind, "%s%s\\%s", szRecordPath, fd.cFileName, "*.*");
			HANDLE hFind2 = FindFirstFile(tempFileFind, &fd2);
			bool bFindFlag2 = true;
			while (bFindFlag2)
			{
				bFindFlag2 = FindNextFile(hFind2, &fd2);
				{
					if (bFindFlag2 && !(strcmp(fd2.cFileName, ".") == 0 || strcmp(fd2.cFileName, "..") == 0) && fd2.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
					{
						//WriteLog(Log_Debug, "FindHistoryRecordFile ，路径 %s%s\\%s  ", szRecordPath, fd.cFileName, fd2.cFileName);

						sprintf(tempFileFind, "%s%s\\%s\\%s", szRecordPath, fd.cFileName, fd2.cFileName, "*.*");
						HANDLE hFind3 = FindFirstFile(tempFileFind, &fd3);
						bool bFindFlag3 = true;

						CRecordFileSource_ptr pRecord = CreateRecordFileSource(fd.cFileName, fd2.cFileName);
						if (hFind3 && pRecord && strstr(fd3.cFileName, ".mp4") != NULL)
							pRecord->AddRecordFile(fd3.cFileName);

						//删除掉 .m3u8 文件
						if (hFind3 && strstr(fd3.cFileName, ".m3u8") != NULL)
						{
							sprintf(szDeleteFile, "%s%s\\%s\\%s", szRecordPath, fd.cFileName, fd2.cFileName, fd3.cFileName);
							ABLDeleteFile(szDeleteFile);
						}

						while (bFindFlag3 && pRecord)
						{
							bFindFlag3 = FindNextFile(hFind3, &fd3);
							if (bFindFlag3 && !(strcmp(fd3.cFileName, ".") == 0 || strcmp(fd3.cFileName, "..") == 0) && fd3.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE)
							{
								if (pRecord && strstr(fd3.cFileName, ".mp4") != NULL)
								{
									pRecord->AddRecordFile(fd3.cFileName);
								}
								//删除掉 .m3u8 文件
								if (bFindFlag3 && strstr(fd3.cFileName, ".m3u8") != NULL)
								{
									sprintf(szDeleteFile, "%s%s\\%s\\%s", szRecordPath, fd.cFileName, fd2.cFileName, fd3.cFileName);
									ABLDeleteFile(szDeleteFile);
								}
								//WriteLog(Log_Debug, "FindHistoryRecordFile ，文件 %s%s\\%s\\%s  ", szRecordPath, fd.cFileName, fd2.cFileName, fd3.cFileName);
							}
						}
						FindClose(hFind3);

						if (pRecord)
							pRecord->Sort();
					}
				}
			}
			FindClose(hFind2);
		}
	}
	FindClose(hFind);
}

//根据图片路径查找所有图片文件 - windows
void FindHistoryPictureFile(char* szPicturePath)
{
	char tempFileFind[MAX_PATH];
	WIN32_FIND_DATA fd = { 0 };
	WIN32_FIND_DATA fd2 = { 0 };
	WIN32_FIND_DATA fd3 = { 0 };
	bool bFindFlag = true;
	char szApp[256] = { 0 }, szStream[string_length_512] = { 0 };//现在只支持2级路径
												   //查找文件
	sprintf(tempFileFind, "%s%s", szPicturePath, "*.*");

	HANDLE hFind = FindFirstFile(tempFileFind, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		WriteLog(Log_Debug, "FindHistoryPictureFile ，配置的录像路径 %s ,没有找到任何文件！ ", szPicturePath);
		return;
	}

	while (bFindFlag)
	{
		bFindFlag = FindNextFile(hFind, &fd);
		if (bFindFlag && !(strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) && fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			//WriteLog(Log_Debug, "FindHistoryPictureFile ，路径 %s%s  ", szPicturePath, fd.cFileName);

			sprintf(tempFileFind, "%s%s\\%s", szPicturePath, fd.cFileName, "*.*");
			HANDLE hFind2 = FindFirstFile(tempFileFind, &fd2);
			bool bFindFlag2 = true;
			while (bFindFlag2)
			{
				bFindFlag2 = FindNextFile(hFind2, &fd2);
				{
					if (bFindFlag2 && !(strcmp(fd2.cFileName, ".") == 0 || strcmp(fd2.cFileName, "..") == 0) && fd2.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
					{
						//WriteLog(Log_Debug, "FindHistoryPictureFile ，路径 %s%s\\%s  ", szPicturePath, fd.cFileName, fd2.cFileName);

						sprintf(tempFileFind, "%s%s\\%s\\%s", szPicturePath, fd.cFileName, fd2.cFileName, "*.*");
						HANDLE hFind3 = FindFirstFile(tempFileFind, &fd3);
						bool bFindFlag3 = true;

						CPictureFileSource_ptr pPicture = CreatePictureFileSource(fd.cFileName, fd2.cFileName);

						while (bFindFlag3 && pPicture)
						{
							bFindFlag3 = FindNextFile(hFind3, &fd3);
							if (bFindFlag3 && !(strcmp(fd3.cFileName, ".") == 0 || strcmp(fd3.cFileName, "..") == 0) && fd3.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE)
							{
								if (pPicture)
								{
									pPicture->AddPictureFile(fd3.cFileName);
								}
								//WriteLog(Log_Debug, "FindHistoryPictureFile ，文件 %s%s\\%s\\%s  ", szPicturePath, fd.cFileName, fd2.cFileName, fd3.cFileName);
							}
						}
						FindClose(hFind3);

						if (pPicture)
							pPicture->Sort();
					}
				}
			}
			FindClose(hFind2);
		}
	}
	FindClose(hFind);
}

#else

//删除文件
bool  ABLDeleteFile(char* szFileName)
{
	int nRet = unlink(szFileName);
	return (nRet == 0) ? true : false;
}

//根据录像路径查找所有录像文件 - linux 
void FindHistoryRecordFile(char* szRecordPath)
{
	struct dirent* filename;    // return value for readdir()
	DIR* dir;                   // return value for opendir()
	dir = opendir(szRecordPath);
	char  szTempPath[512] = { 0 };
	char  szDeleteFile[1024] = { 0 };

	while ((filename = readdir(dir)) != NULL)
	{
		// get rid of "." and ".."
		if (strcmp(filename->d_name, ".") == 0 ||
			strcmp(filename->d_name, "..") == 0)
			continue;

		if (strlen(filename->d_name) > 0)
		{
			//WriteLog(Log_Debug, "FindHistoryRecordFile ，路径 %s ", filename->d_name);

			struct dirent* filename2;
			DIR* dir2;
			memset(szTempPath, 0x00, sizeof(szTempPath));
			sprintf(szTempPath, "%s%s", szRecordPath, filename->d_name);
			dir2 = opendir(szTempPath);

			while ((filename2 = readdir(dir2)) != NULL)
			{
				if (!(strcmp(filename2->d_name, ".") == 0 || strcmp(filename2->d_name, "..") == 0))
				{
					//WriteLog(Log_Debug, "FindHistoryRecordFile ，第2级路径 %s ", filename2->d_name);

					struct dirent* filename3;
					DIR* dir3;
					memset(szTempPath, 0x00, sizeof(szTempPath));
					sprintf(szTempPath, "%s%s/%s", szRecordPath, filename->d_name, filename2->d_name);
					dir3 = opendir(szTempPath);

					CRecordFileSource_ptr pRecord = CreateRecordFileSource(filename->d_name, filename2->d_name);

					while ((filename3 = readdir(dir3)) != NULL && pRecord)
					{
						if (!(strcmp(filename3->d_name, ".") == 0 || strcmp(filename3->d_name, "..") == 0))
						{
							//WriteLog(Log_Debug, "FindHistoryRecordFile ,录像文件名字 %s ", filename3->d_name);
							if (pRecord && strstr(filename3->d_name, ".mp4") != NULL)
							{
								pRecord->AddRecordFile(filename3->d_name);
							}
							else if (strstr(filename3->d_name, ".m3u8") != NULL)
							{//删除废弃的 m3u8 文件 
								sprintf(szDeleteFile, "%s%s/%s/%s", szRecordPath, filename->d_name, filename2->d_name, filename3->d_name);
								ABLDeleteFile(szDeleteFile);
							}
						}
					}
					closedir(dir3);

					if (pRecord)
						pRecord->Sort();
				}
			}//while ((filename2 = readdir(dir2)) != NULL)
			closedir(dir2);

		}
	}//while ((filename = readdir(dir)) != NULL)
	closedir(dir);
}

//根据图片路径查找所有图片文件 - linux 
void FindHistoryPictureFile(char* szPicturePath)
{
	struct dirent* filename;    // return value for readdir()
	DIR* dir;                   // return value for opendir()
	dir = opendir(szPicturePath);
	char  szTempPath[512] = { 0 };

	while ((filename = readdir(dir)) != NULL)
	{
		// get rid of "." and ".."
		if (strcmp(filename->d_name, ".") == 0 ||
			strcmp(filename->d_name, "..") == 0)
			continue;

		if (strlen(filename->d_name) > 0)
		{
			//WriteLog(Log_Debug, "FindHistoryPictureFile ，路径 %s ", filename->d_name);

			struct dirent* filename2;
			DIR* dir2;
			memset(szTempPath, 0x00, sizeof(szTempPath));
			sprintf(szTempPath, "%s%s", szPicturePath, filename->d_name);
			dir2 = opendir(szTempPath);

			while ((filename2 = readdir(dir2)) != NULL)
			{
				if (!(strcmp(filename2->d_name, ".") == 0 || strcmp(filename2->d_name, "..") == 0))
				{
					//WriteLog(Log_Debug, "FindHistoryPictureFile ，第2级路径 %s ", filename2->d_name);

					struct dirent* filename3;
					DIR* dir3;
					memset(szTempPath, 0x00, sizeof(szTempPath));
					sprintf(szTempPath, "%s%s/%s", szPicturePath, filename->d_name, filename2->d_name);
					dir3 = opendir(szTempPath);

					CPictureFileSource_ptr pPicture = CreatePictureFileSource(filename->d_name, filename2->d_name);

					while ((filename3 = readdir(dir3)) != NULL && pPicture)
					{
						if (!(strcmp(filename3->d_name, ".") == 0 || strcmp(filename3->d_name, "..") == 0))
						{
							//WriteLog(Log_Debug, "FindHistoryPictureFile ,录像文件名字 %s ", filename3->d_name);
							if (pPicture)
							{
								pPicture->AddPictureFile(filename3->d_name);
							}
						}
					}
					closedir(dir3);

					if (pPicture)
						pPicture->Sort();
				}
			}//while ((filename2 = readdir(dir2)) != NULL)
			closedir(dir2);
		}
	}//while ((filename = readdir(dir)) != NULL)
	closedir(dir);
}

#endif

void WebRtcCallBack(const char* callbackJson, void* pUserHandle)
{
	WriteLog(Log_Debug, " ----- WebRtcCallBack ----- ：\r\n%s ", callbackJson);
	if (strlen(callbackJson) > 0 /* && callbackJson[0] == '{' && callbackJson[strlen(callbackJson) - 1] == '}' */)
	{
		WebRtcCallStruct callbackStruct;
		rapidjson::Document doc;
		doc.Parse<0>((char*)callbackJson);
		int listSize;
		if (!doc.HasParseError())
		{
			if (strstr(callbackJson, "eventID") != NULL)
				callbackStruct.eventID = doc["eventID"].GetInt64();
			if (strstr(callbackJson, "media") != NULL)
				strcpy(callbackStruct.media, doc["media"].GetString());
			if (strstr(callbackJson, "playerID") != NULL)
				strcpy(callbackStruct.playerID, doc["playerID"].GetString());
			if (strstr(callbackJson, "stream") != NULL)
				strcpy(callbackStruct.stream, doc["stream"].GetString());
		}

		if (callbackStruct.eventID == 2)
		{//创建webrtc播放
			CMediaStreamSource_ptr pMediaSource = GetMediaStreamSource(callbackStruct.stream, false);
			if (pMediaSource == NULL)
				WriteLog(Log_Debug, "不存在流 %s ", callbackStruct.stream);
			else
			{
				if ( pMediaSource->bCreateWebRtcPlaySourceFlag.load() == false)
				{
					CNetRevcBase_ptr pClient = CreateNetRevcBaseClient(NetBaseNetType_NetClientWebrtcPlayer, 0, 0, "", 0, callbackStruct.stream);
					if (pClient != NULL)
					{
						WriteLog(Log_Debug, "创建webrtc播放  %s ", callbackStruct.stream);
						memcpy((char*)&pClient->webRtcCallStruct, (char*)&callbackStruct, sizeof(WebRtcCallStruct));
					}
					pMediaSource->nWebRtcPlayerCount++;
				}
				else
				{
					if (pMediaSource->bCreateWebRtcPlaySourceFlag.load() == true)
					{
						pMediaSource->nWebRtcPlayerCount++;
						WriteLog(Log_Debug, "媒体源 %s 的视频格式为 %s ,已经创建了webrtc 播放媒体源 ", callbackStruct.stream, pMediaSource->m_mediaCodecInfo.szVideoName);
					}
					else
						WriteLog(Log_Debug, "媒体源 %s 的视频格式为 %s ,不支持WebRTC播放，必须为H264 ", callbackStruct.stream, pMediaSource->m_mediaCodecInfo.szVideoName);
				}
			}
		}
		else if (callbackStruct.eventID == 5)
		{//删除webrtc播放
			CMediaStreamSource_ptr pMediaSource = GetMediaStreamSource(callbackStruct.stream, false);
			if (pMediaSource != NULL)
			{
				pMediaSource->nWebRtcPlayerCount--;
				if (pMediaSource->nWebRtcPlayerCount <= 0)
				{//统计出无人观看时
					pMediaSource->nWebRtcPlayerCount = 0;
					WriteLog(Log_Debug, "媒体源 %s , pMediaSource->nWebRtcPlayerCount = %d  已经无人观看，把 nClient = %llu 从 发送线程池移除  ", callbackStruct.stream, pMediaSource->nWebRtcPlayerCount, pMediaSource->nWebRtcPushStreamID);
				}
			}
		}
		else if (callbackStruct.eventID == 7)
		{//删除webrtc媒体源

		}
	}
};
#define VERSION	 "1.0.001.0425" //   001是当日第几个版本       最后的是日期
void  printfVersion()
{

	WriteLog(Log_Debug, "******************************************");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*           当前版本 : %s      *", VERSION);
	WriteLog(Log_Debug, "*    %s    *", MediaServerVerson);
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "*                                        *");
	WriteLog(Log_Debug, "******************************************");

}


#ifdef OS_System_Windows
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	pcm16_alaw_tableinit();
	pcm16_ulaw_tableinit();

	//获取cpu物理核心数 
	ABL_nCurrentSystemCpuCount = std::thread::hardware_concurrency();
	if (ABL_nCurrentSystemCpuCount <= 4)
		ABL_nCurrentSystemCpuCount = 4;
	else if (ABL_nCurrentSystemCpuCount > 256)
		ABL_nCurrentSystemCpuCount = 256;

ABL_Restart:
	unsigned char nGet;
	int nBindHttp, nBindRtsp, nBindRtmp, nBindWsFlv, nBindHttpFlv, nBindHls, nBindMp4, nBindRecvAudio, nBingPS10000;

	memset(ABL_MediaSeverRunPath, 0x00, sizeof(ABL_MediaSeverRunPath));
	memset(ABL_wwwMediaPath, 0x00, sizeof(ABL_wwwMediaPath));
	memset(ABL_szLocalIP, 0x00, sizeof(ABL_szLocalIP));

	nMaxAddMuteListNumber = 0;
	for (int i = 0; i < 8192; i++)
		ArrayAddMutePacketList[i] = 0;

#ifdef OS_System_Windows

	//鼠标点击控制台窗口，不会再卡住 
	DWORD mode;
	HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hstdin, &mode);
	mode &= ~ENABLE_QUICK_EDIT_MODE;
	mode &= ~ENABLE_INSERT_MODE;
	mode &= ~ENABLE_MOUSE_INPUT;
	SetConsoleMode(hstdin, mode);

	strcpy(szConfigFileName, "ABLMediaServer.exe_554");
	HANDLE hRunAsOne = ::CreateMutex(NULL, FALSE, szConfigFileName);
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return -1;
	}
	memset(szConfigFileName, 0x00, sizeof(szConfigFileName));

	StartLogFile("ABLMediaServer", "ABLMediaServer_00*.log", 5);
	srand(GetTickCount());

	GetMediaServerCurrentPath(ABL_MediaSeverRunPath);
	sprintf(szConfigFileName, "%s%s", ABL_MediaSeverRunPath, "ABLMediaServer.ini");
	if (ABL_ConfigFile.LoadFile(szConfigFileName) != SI_OK)
	{
		WriteLog(Log_Error, "没有找到配置文件 ：%s ", szConfigFileName);
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		return -1;
	}

	//获取用户配置的IP地址 
	strcpy(ABL_szLocalIP, ABL_ConfigFile.GetValue("ABLMediaServer", "localipAddress", ""));

	//自动获取IP地址
	if (strlen(ABL_szLocalIP) == 0)
	{
		string strIPTemp;
		GetLocalAdaptersInfo(strIPTemp);
		int nPos = strIPTemp.find(",", 0);
		if (nPos > 0)
			memcpy(ABL_szLocalIP, strIPTemp.c_str(), nPos);
		else
			strcpy(ABL_szLocalIP, "127.0.0.1");
	}
	strcpy(ABL_MediaServerPort.ABL_szLocalIP, ABL_szLocalIP);
	WriteLog(Log_Debug, "本机IP地址 ABL_szLocalIP : %s ", ABL_szLocalIP);
	WriteLog(Log_Debug, "本机cpu物理核心数量 nCurrentSystemCpuCount %d ", ABL_nCurrentSystemCpuCount);


	strcpy(ABL_MediaServerPort.secret, ABL_ConfigFile.GetValue("ABLMediaServer", "secret", "035c73f7-bb6b-4889-a715-d9eb2d1925cc111"));
	ABL_MediaServerPort.nHttpServerPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpServerPort", 8081);
	ABL_MediaServerPort.nRtspPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "rtspPort", 554);
	ABL_MediaServerPort.nRtmpPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "rtmpPort", 1935);
	ABL_MediaServerPort.nHttpFlvPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpFlvPort", 8088);
	ABL_MediaServerPort.nWSFlvPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "wsFlvPort", 6088);
	ABL_MediaServerPort.nHttpMp4Port = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpMp4Port", 8089);
	ABL_MediaServerPort.ps_tsRecvPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ps_tsRecvPort", 10000);
	ABL_MediaServerPort.nHlsPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hlsPort", 9081);
	ABL_MediaServerPort.WsRecvPcmPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "WsRecvPcmPort", 9298);
	ABL_MediaServerPort.nHlsEnable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hls_enable", 5);
	ABL_MediaServerPort.nHLSCutType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hlsCutType", 1);
	ABL_MediaServerPort.nH265CutType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "h265CutType", 1);
	ABL_MediaServerPort.hlsCutTime = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hlsCutTime", 1);
	ABL_MediaServerPort.nMaxTsFileCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "maxTsFileCount", 10);
	strcpy(ABL_MediaServerPort.wwwPath, ABL_ConfigFile.GetValue("ABLMediaServer", "wwwPath", ""));

	ABL_MediaServerPort.nRecvThreadCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "RecvThreadCount", 64);
	ABL_MediaServerPort.nSendThreadCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "SendThreadCount", 64);
	ABL_MediaServerPort.nRecordReplayThread = ABL_ConfigFile.GetLongValue("ABLMediaServer", "RecordReplayThread", 32);
	ABL_MediaServerPort.nGBRtpTCPHeadType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "GB28181RtpTCPHeadType", 0);
	ABL_MediaServerPort.nEnableAudio = ABL_ConfigFile.GetLongValue("ABLMediaServer", "enable_audio", 0);
	ABL_MediaServerPort.nIOContentNumber = ABL_ConfigFile.GetLongValue("ABLMediaServer", "IOContentNumber", 16);
	ABL_MediaServerPort.nThreadCountOfIOContent = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ThreadCountOfIOContent", 16);
	ABL_MediaServerPort.nReConnectingCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ReConnectingCount", 48000);

	strcpy(ABL_MediaServerPort.recordPath, ABL_ConfigFile.GetValue("ABLMediaServer", "recordPath", ""));
	ABL_MediaServerPort.pushEnable_mp4 = ABL_ConfigFile.GetLongValue("ABLMediaServer", "pushEnable_mp4", 0);
	ABL_MediaServerPort.fileSecond = ABL_ConfigFile.GetLongValue("ABLMediaServer", "fileSecond", 180);
	ABL_MediaServerPort.videoFileFormat = ABL_ConfigFile.GetLongValue("ABLMediaServer", "videoFileFormat", 1);
	ABL_MediaServerPort.fileKeepMaxTime = ABL_ConfigFile.GetLongValue("ABLMediaServer", "fileKeepMaxTime", 12);
	ABL_MediaServerPort.enable_GetFileDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "enable_GetFileDuration", 0);
	ABL_MediaServerPort.fileRepeat = ABL_ConfigFile.GetLongValue("ABLMediaServer", "fileRepeat", 0);
	ABL_MediaServerPort.httpDownloadSpeed = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpDownloadSpeed", 6);

	ABL_MediaServerPort.maxTimeNoOneWatch = ABL_ConfigFile.GetLongValue("ABLMediaServer", "maxTimeNoOneWatch", 2);
	ABL_MediaServerPort.nG711ConvertAAC = ABL_ConfigFile.GetLongValue("ABLMediaServer", "G711ConvertAAC",0);

	strcpy(ABL_MediaServerPort.picturePath, ABL_ConfigFile.GetValue("ABLMediaServer", "picturePath", ""));
	

	ABL_MediaServerPort.pictureMaxCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "pictureMaxCount", 30);
	ABL_MediaServerPort.captureReplayType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "captureReplayType",1);
	ABL_MediaServerPort.deleteSnapPicture = ABL_ConfigFile.GetLongValue("ABLMediaServer", "deleteSnapPicture", 0);
	ABL_MediaServerPort.iframeArriveNoticCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "iframeArriveNoticCount", 30);
	ABL_MediaServerPort.maxSameTimeSnap = ABL_ConfigFile.GetLongValue("ABLMediaServer", "maxSameTimeSnap", 16);
	ABL_MediaServerPort.snapObjectDestroy = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapObjectDestroy", 1);
	ABL_MediaServerPort.snapObjectDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapObjectDuration", 120);
	ABL_MediaServerPort.snapOutPictureWidth = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapOutPictureWidth", 0);
	ABL_MediaServerPort.snapOutPictureHeight = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapOutPictureHeight", 0);

	ABL_MediaServerPort.H265ConvertH264_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "H265ConvertH264_enable", 1);
	ABL_MediaServerPort.H265DecodeCpuGpuType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "H265DecodeCpuGpuType", 0);
	ABL_MediaServerPort.convertOutWidth = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertOutWidth", 7210);
	ABL_MediaServerPort.convertOutHeight = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertOutHeight", 1480);
	ABL_MediaServerPort.convertMaxObject = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertMaxObject", 214);
	ABL_MediaServerPort.convertOutBitrate = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertOutBitrate", 123);
	ABL_MediaServerPort.H264DecodeEncode_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "H264DecodeEncode_enable", 0);
	ABL_MediaServerPort.filterVideo_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "filterVideo_enable", 0);
	strcpy(ABL_MediaServerPort.filterVideoText, ABL_ConfigFile.GetValue("ABLMediaServer", "filterVideo_text", ""));
	ABL_MediaServerPort.nFilterFontSize = ABL_ConfigFile.GetLongValue("ABLMediaServer", "FilterFontSize", 12);
	strcpy(ABL_MediaServerPort.nFilterFontColor, ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontColor","red"));
	ABL_MediaServerPort.nFilterFontLeft = ABL_ConfigFile.GetLongValue("ABLMediaServer", "FilterFontLeft", 5);
	ABL_MediaServerPort.nFilterFontTop = ABL_ConfigFile.GetLongValue("ABLMediaServer", "FilterFontTop", 5);
	ABL_MediaServerPort.nFilterFontAlpha = ABL_ConfigFile.GetLongValue("ABLMediaServer", "FilterFontAlpha", 0.5);
	ABL_MediaServerPort.MaxDiconnectTimeoutSecond = ABL_ConfigFile.GetLongValue("ABLMediaServer", "MaxDiconnectTimeoutSecond", 18);
	ABL_MediaServerPort.ForceSendingIFrame = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ForceSendingIFrame", 0);
	ABL_MediaServerPort.gb28181LibraryUse = ABL_ConfigFile.GetLongValue("ABLMediaServer", "gb28181LibraryUse", 1);
	ABL_MediaServerPort.httqRequstClose = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httqRequstClose", 0);
	ABL_MediaServerPort.flvPlayAddMute = ABL_ConfigFile.GetLongValue("ABLMediaServer", "flvPlayAddMute", 1);

	//读取事件通知配置
	ABL_MediaServerPort.hook_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hook_enable", 0);
	ABL_MediaServerPort.noneReaderDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "noneReaderDuration", 32);
	strcpy(ABL_MediaServerPort.on_server_started, ABL_ConfigFile.GetValue("ABLMediaServer", "on_server_started", ""));
	strcpy(ABL_MediaServerPort.on_server_keepalive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_server_keepalive", ""));
	strcpy(ABL_MediaServerPort.on_stream_arrive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_arrive", ""));
	strcpy(ABL_MediaServerPort.on_stream_not_arrive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_not_arrive", ""));
	strcpy(ABL_MediaServerPort.on_stream_none_reader, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_none_reader", ""));
	strcpy(ABL_MediaServerPort.on_stream_disconnect, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_disconnect", ""));
	strcpy(ABL_MediaServerPort.on_stream_not_found, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_not_found", ""));
	strcpy(ABL_MediaServerPort.on_record_mp4, ABL_ConfigFile.GetValue("ABLMediaServer", "on_record_mp4", ""));
	strcpy(ABL_MediaServerPort.on_delete_record_mp4, ABL_ConfigFile.GetValue("ABLMediaServer", "on_delete_record_mp4", ""));
	strcpy(ABL_MediaServerPort.on_record_progress, ABL_ConfigFile.GetValue("ABLMediaServer", "on_record_progress", ""));
	strcpy(ABL_MediaServerPort.on_record_ts, ABL_ConfigFile.GetValue("ABLMediaServer", "on_record_ts", ""));
	strcpy(ABL_MediaServerPort.mediaServerID, ABL_ConfigFile.GetValue("ABLMediaServer", "mediaServerID", "ABLMediaServer_00001"));
	strcpy(ABL_MediaServerPort.on_play, ABL_ConfigFile.GetValue("ABLMediaServer", "on_play", ""));
	strcpy(ABL_MediaServerPort.on_publish, ABL_ConfigFile.GetValue("ABLMediaServer", "on_publish", ""));
	strcpy(ABL_MediaServerPort.on_stream_iframe_arrive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_iframe_arrive", ""));

	ABL_MediaServerPort.keepaliveDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "keepaliveDuration", 20);
	ABL_MediaServerPort.nWebRtcPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "webrtcPort", 8000);

	
	ABL_MediaServerPort.nUseWvp = ABL_ConfigFile.GetLongValue("api", "usewvp", 0);
	strcpy(ABL_MediaServerPort.port_range, ABL_ConfigFile.GetValue("rtp_proxy", "port_range", ""));
	strcpy(ABL_MediaServerPort.listeningip, ABL_ConfigFile.GetValue("rtc", "listening-ip", ""));
	strcpy(ABL_MediaServerPort.externalip, ABL_ConfigFile.GetValue("rtc", "external-ip", ""));
	ABL_MediaServerPort.listeningport = ABL_ConfigFile.GetLongValue("rtc", "listening-port", 3478);
	ABL_MediaServerPort.minport = ABL_ConfigFile.GetLongValue("rtc", "min-port", 49152);
	ABL_MediaServerPort.maxport = ABL_ConfigFile.GetLongValue("rtc", "max-port", 65535);
	strcpy(ABL_MediaServerPort.realm, ABL_ConfigFile.GetValue("rtc", "realm", ""));
	strcpy(ABL_MediaServerPort.user, ABL_ConfigFile.GetValue("rtc", "user", ""));




	if (ABL_MediaServerPort.httpDownloadSpeed > 10)
		ABL_MediaServerPort.httpDownloadSpeed = 10;
	else if (ABL_MediaServerPort.httpDownloadSpeed <= 0)
		ABL_MediaServerPort.httpDownloadSpeed = 1;

	if (strlen(ABL_MediaServerPort.recordPath) == 0)
		strcpy(ABL_MediaServerPort.recordPath, ABL_MediaSeverRunPath);
	else
	{//用户设置的路径,防止用户没有创建好路径
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.recordPath;
		while (true)
		{
			nPos = strPath.find("\\", nPos + 3);
			if (nPos > 0)
			{
				memcpy(szTempPath, ABL_MediaServerPort.recordPath, nPos);
				::CreateDirectory(szTempPath, NULL);
			}
			else
			{
				::CreateDirectory(ABL_MediaServerPort.recordPath, NULL);
				break;
			}
		}
	}

	//设置图片抓拍路径
	if (strlen(ABL_MediaServerPort.picturePath) == 0)
		strcpy(ABL_MediaServerPort.picturePath, ABL_MediaSeverRunPath);
	else
	{//用户设置的路径,防止用户没有创建好路径
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.picturePath;
		while (true)
		{
			nPos = strPath.find("\\", nPos + 3);
			if (nPos > 0)
			{
				memcpy(szTempPath, ABL_MediaServerPort.picturePath, nPos);
				::CreateDirectory(szTempPath, NULL);
			}
			else
			{
				::CreateDirectory(ABL_MediaServerPort.picturePath, NULL);
				break;
			}
		}
	}

	//设置切片路径
	if (strlen(ABL_MediaServerPort.wwwPath) == 0)
		strcpy(ABL_MediaServerPort.wwwPath, ABL_MediaSeverRunPath);
	else
	{//用户设置的路径,防止用户没有创建好路径
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.wwwPath;
		while (true)
		{
			nPos = strPath.find("\\", nPos + 3);
			if (nPos > 0)
			{
				memcpy(szTempPath, ABL_MediaServerPort.wwwPath, nPos);
				::CreateDirectory(szTempPath, NULL);
			}
			else
			{
				::CreateDirectory(ABL_MediaServerPort.wwwPath, NULL);
				break;
			}
		}
	}

	//创建子路径 record 
	if (ABL_MediaServerPort.recordPath[strlen(ABL_MediaServerPort.recordPath) - 1] != '\\')
		strcat(ABL_MediaServerPort.recordPath, "\\");
	strcat(ABL_MediaServerPort.recordPath, "record\\");
	::CreateDirectory(ABL_MediaServerPort.recordPath, NULL);

	//创建子路径 picture 
	if (ABL_MediaServerPort.picturePath[strlen(ABL_MediaServerPort.picturePath) - 1] != '\\')
		strcat(ABL_MediaServerPort.picturePath, "\\");
	strcat(ABL_MediaServerPort.picturePath, "picture\\");
	::CreateDirectory(ABL_MediaServerPort.picturePath, NULL);

	sprintf(ABL_MediaServerPort.debugPath, "%s%s\\", ABL_MediaSeverRunPath, "debugFile");
	::CreateDirectory(ABL_MediaServerPort.debugPath, NULL);

	WriteLog(Log_Debug, "创建成功录像路径：%s ,创建图片路径: %s  ", ABL_MediaServerPort.recordPath, ABL_MediaServerPort.picturePath);
	//把历史录像文件装入 list  
	FindHistoryRecordFile(ABL_MediaServerPort.recordPath);
	//把历史图片装入list 
	FindHistoryPictureFile(ABL_MediaServerPort.picturePath);

	rtp_packet_setsize(65535);

	if (ABL_MediaServerPort.H265ConvertH264_enable == 1 && ABL_MediaServerPort.H265DecodeCpuGpuType == 1 && ABL_bInitCudaSDKFlag == false)
	{///英伟达
		if (true/*不能根据显卡名称来判断，有些电脑有双显卡 strstr(pD3DName.Description, "GeForce") != NULL || strstr(pD3DName.Description, "NVIDIA") != NULL*/)
		{//英伟达显卡硬编码
			hCudaDecodeInstance = ::LoadLibrary("cudaCodecDLL.dll");
			if (hCudaDecodeInstance != NULL)
			{
				cudaCodec_Init = (ABL_cudaDecode_Init)::GetProcAddress(hCudaDecodeInstance, "cudaCodec_Init");
				cudaCodec_GetDeviceGetCount = (ABL_cudaDecode_GetDeviceGetCount) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceGetCount");
				cudaCodec_GetDeviceName = (ABL_cudaDecode_GetDeviceName) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceName");
				cudaCodec_GetDeviceUse = (ABL_cudaDecode_GetDeviceUse) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetDeviceUse");
				cudaCodec_CreateVideoDecode = (ABL_CreateVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_CreateVideoDecode");
				cudaCodec_CudaVideoDecode = (ABL_CudaVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_CudaVideoDecode");

				cudaCodec_DeleteVideoDecode = (ABL_DeleteVideoDecode) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_DeleteVideoDecode");
				cudaCodec_GetCudaDecodeCount = (ABL_GetCudaDecodeCount) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_GetCudaDecodeCount");
				cudaCodec_UnInit = (ABL_VideoDecodeUnInit) ::GetProcAddress(hCudaDecodeInstance, "cudaCodec_UnInit");

			}
			if (cudaCodec_Init)
				ABL_bCudaFlag = cudaCodec_Init();
			if (cudaCodec_GetDeviceGetCount)
				ABL_nCudaCount = cudaCodec_GetDeviceGetCount();

			if (ABL_bCudaFlag == false || ABL_nCudaCount <= 0)
				ABL_MediaServerPort.H265DecodeCpuGpuType = 0; //恢复cpu软解
			else
			{//cuda 资源已经启动过 
				ABL_bInitCudaSDKFlag = true;
				WriteLog(Log_Debug, "测试英伟达显卡 ABL_bCudaFlag = %d, 英伟达显卡总数 : %d  ", ABL_bCudaFlag, ABL_nCudaCount);
			}
		}
		else
			ABL_MediaServerPort.H265DecodeCpuGpuType = 0; //恢复cpu软解
	}
	else if (ABL_MediaServerPort.H265ConvertH264_enable == 1 && ABL_MediaServerPort.H265DecodeCpuGpuType == 2)
	{//amd 
		ABL_MediaServerPort.H265DecodeCpuGpuType = 0; //恢复cpu软解
	}

	if (ABL_MediaServerPort.H265ConvertH264_enable == 1)
		WriteLog(Log_Debug, "ABL_MediaServerPort.H265DecodeCpuGpuType = %d ", ABL_MediaServerPort.H265DecodeCpuGpuType);
#else
	InitLogFile();
	if (argc >= 3)
		WriteLog(Log_Debug, "argc = %d, argv[0] = %s ,argv[1] = %s ,argv[2] = %s ", argc, argv[0], argv[1], argv[2]);

	strcpy(ABL_MediaSeverRunPath, get_current_dir_name());
	if (argc >= 3 && strcmp(argv[1], "-c") == 0)
	{//带配置文件启动
		strcpy(szConfigFileName, argv[2]);
	}
	else
		sprintf(szConfigFileName, "%s/%s", ABL_MediaSeverRunPath, "ABLMediaServer.ini");
	WriteLog(Log_Debug, "ABLMediaServer.ini : %s ", szConfigFileName);
	if (access(szConfigFileName, F_OK) != 0)
	{
		WriteLog(Log_Debug, "当前路径 %s 没有配置文件 ABLMediaServer.ini，请检查。", ABL_MediaSeverRunPath);
		return -1;
	}
	if (ABL_ConfigFile.LoadFile(szConfigFileName) != SI_OK)
	{
		WriteLog(Log_Debug, "读取配置文件 %s 失败 ！", szConfigFileName);
		return -1;
	}

	//获取用户配置的IP地址 
	strcpy(ABL_szLocalIP, ABL_ConfigFile.GetValue("ABLMediaServer", "localipAddress", ""));
	WriteLog(Log_Debug, "读取到配置文件的IP : %s ", ABL_szLocalIP);
	WriteLog(Log_Debug, "本机cpu物理核心数量 nCurrentSystemCpuCount %d ", ABL_nCurrentSystemCpuCount);

	//获取IP地址 
	if (strlen(ABL_szLocalIP) == 0)
	{
		string strIPTemp;
		GetLocalAdaptersInfo(strIPTemp);
		int nPos = strIPTemp.find(",", 0);
		if (nPos > 0)
			memcpy(ABL_szLocalIP, strIPTemp.c_str(), nPos);
		else
			strcpy(ABL_szLocalIP, "127.0.0.1");
	}
	strcpy(ABL_MediaServerPort.ABL_szLocalIP, ABL_szLocalIP);
	WriteLog(Log_Debug, "本机IP地址 ABL_szLocalIP : %s ", ABL_szLocalIP);
	
	strcpy(ABL_MediaServerPort.secret, ABL_ConfigFile.GetValue("ABLMediaServer", "secret",""));
	ABL_MediaServerPort.nHttpServerPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpServerPort",0);
	ABL_MediaServerPort.nRtspPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "rtspPort",0);
	ABL_MediaServerPort.nRtmpPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "rtmpPort",0);
	ABL_MediaServerPort.nHttpFlvPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpFlvPort",0);
	ABL_MediaServerPort.nWSFlvPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "wsFlvPort",0);
	ABL_MediaServerPort.nHttpMp4Port = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpMp4Port",0);
	ABL_MediaServerPort.ps_tsRecvPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ps_tsRecvPort",0);
	ABL_MediaServerPort.nHlsPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hlsPort",0);
	ABL_MediaServerPort.WsRecvPcmPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "WsRecvPcmPort",0);
	ABL_MediaServerPort.nHlsEnable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hls_enable",0);
	ABL_MediaServerPort.nHLSCutType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hlsCutType",0);
	ABL_MediaServerPort.nH265CutType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "h265CutType",0);
	ABL_MediaServerPort.hlsCutTime = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hlsCutTime",0);
	ABL_MediaServerPort.nMaxTsFileCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "maxTsFileCount",0);
	strcpy(ABL_MediaServerPort.wwwPath, ABL_ConfigFile.GetValue("ABLMediaServer", "wwwPath",""));

	ABL_MediaServerPort.nRecvThreadCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "RecvThreadCount",0);
	ABL_MediaServerPort.nSendThreadCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "SendThreadCount",0);
	ABL_MediaServerPort.nRecordReplayThread = ABL_ConfigFile.GetLongValue("ABLMediaServer", "RecordReplayThread",0);
	ABL_MediaServerPort.nGBRtpTCPHeadType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "GB28181RtpTCPHeadType",0);
	ABL_MediaServerPort.nEnableAudio = ABL_ConfigFile.GetLongValue("ABLMediaServer", "enable_audio",0);
	ABL_MediaServerPort.nIOContentNumber = ABL_ConfigFile.GetLongValue("ABLMediaServer", "IOContentNumber",0);
	ABL_MediaServerPort.nThreadCountOfIOContent = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ThreadCountOfIOContent",0);
	ABL_MediaServerPort.nReConnectingCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ReConnectingCount",0);

	strcpy(ABL_MediaServerPort.recordPath, ABL_ConfigFile.GetValue("ABLMediaServer", "recordPath",""));
	ABL_MediaServerPort.fileSecond = ABL_ConfigFile.GetLongValue("ABLMediaServer", "fileSecond",0);
	ABL_MediaServerPort.videoFileFormat = ABL_ConfigFile.GetLongValue("ABLMediaServer", "videoFileFormat",0);
	ABL_MediaServerPort.pushEnable_mp4 = ABL_ConfigFile.GetLongValue("ABLMediaServer", "pushEnable_mp4",0);
	ABL_MediaServerPort.fileKeepMaxTime = ABL_ConfigFile.GetLongValue("ABLMediaServer", "fileKeepMaxTime",0);
	ABL_MediaServerPort.fileRepeat = ABL_ConfigFile.GetLongValue("ABLMediaServer", "fileRepeat",0);
	ABL_MediaServerPort.enable_GetFileDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "enable_GetFileDuration",0);
	ABL_MediaServerPort.httpDownloadSpeed = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpDownloadSpeed",0);

	ABL_MediaServerPort.maxTimeNoOneWatch = ABL_ConfigFile.GetLongValue("ABLMediaServer", "maxTimeNoOneWatch",0);
	ABL_MediaServerPort.nG711ConvertAAC = ABL_ConfigFile.GetLongValue("ABLMediaServer", "G711ConvertAAC",0); 

	strcpy(ABL_MediaServerPort.picturePath, ABL_ConfigFile.GetValue("ABLMediaServer", "picturePath",""));
	ABL_MediaServerPort.pictureMaxCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "pictureMaxCount",0);
	ABL_MediaServerPort.captureReplayType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "captureReplayType",0);
	ABL_MediaServerPort.maxSameTimeSnap = ABL_ConfigFile.GetLongValue("ABLMediaServer", "maxSameTimeSnap",0);
	ABL_MediaServerPort.snapObjectDestroy = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapObjectDestroy",0);
	ABL_MediaServerPort.deleteSnapPicture = ABL_ConfigFile.GetLongValue("ABLMediaServer", "deleteSnapPicture",0);
	ABL_MediaServerPort.iframeArriveNoticCount = ABL_ConfigFile.GetLongValue("ABLMediaServer", "iframeArriveNoticCount",0);
	ABL_MediaServerPort.snapObjectDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapObjectDuration",0);
	ABL_MediaServerPort.snapOutPictureWidth = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapOutPictureWidth",0);
	ABL_MediaServerPort.snapOutPictureHeight = ABL_ConfigFile.GetLongValue("ABLMediaServer", "snapOutPictureHeight",0);
	
	ABL_MediaServerPort.H265ConvertH264_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "H265ConvertH264_enable",0);
	ABL_MediaServerPort.H265DecodeCpuGpuType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "H265DecodeCpuGpuType",0);
	ABL_MediaServerPort.convertOutWidth = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertOutWidth",0);
	ABL_MediaServerPort.convertOutHeight = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertOutHeight",0);
	ABL_MediaServerPort.convertMaxObject = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertMaxObject",0);
	ABL_MediaServerPort.convertOutBitrate = ABL_ConfigFile.GetLongValue("ABLMediaServer", "convertOutBitrate",0);
	ABL_MediaServerPort.H264DecodeEncode_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "H264DecodeEncode_enable",0);
	ABL_MediaServerPort.filterVideo_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "filterVideo_enable",0);
	strcpy(ABL_MediaServerPort.filterVideoText, ABL_ConfigFile.GetValue("ABLMediaServer", "filterVideo_text",""));
	ABL_MediaServerPort.nFilterFontSize = ABL_ConfigFile.GetLongValue("ABLMediaServer", "FilterFontSize",0);
	strcpy(ABL_MediaServerPort.nFilterFontColor, ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontColor",""));
	ABL_MediaServerPort.nFilterFontLeft = ABL_ConfigFile.GetLongValue("ABLMediaServer", "FilterFontLeft",0);
	ABL_MediaServerPort.nFilterFontTop = ABL_ConfigFile.GetLongValue("ABLMediaServer", "FilterFontTop",0);
	ABL_MediaServerPort.nFilterFontAlpha = atof(ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontAlpha",""));
	ABL_MediaServerPort.httqRequstClose = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httqRequstClose",0);
 
	//读取事件通知配置
	ABL_MediaServerPort.hook_enable = ABL_ConfigFile.GetLongValue("ABLMediaServer", "hook_enable",0);
	ABL_MediaServerPort.noneReaderDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "noneReaderDuration",0);
	strcpy(ABL_MediaServerPort.on_server_started, ABL_ConfigFile.GetValue("ABLMediaServer", "on_server_started",""));
	strcpy(ABL_MediaServerPort.on_server_keepalive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_server_keepalive",""));
	strcpy(ABL_MediaServerPort.on_stream_arrive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_arrive",""));
	strcpy(ABL_MediaServerPort.on_stream_not_arrive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_not_arrive",""));
	strcpy(ABL_MediaServerPort.on_stream_none_reader, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_none_reader",""));
	strcpy(ABL_MediaServerPort.on_stream_disconnect, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_disconnect",""));
	strcpy(ABL_MediaServerPort.on_stream_not_found, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_not_found",""));
	strcpy(ABL_MediaServerPort.on_record_mp4, ABL_ConfigFile.GetValue("ABLMediaServer", "on_record_mp4","")); 
	strcpy(ABL_MediaServerPort.on_record_progress, ABL_ConfigFile.GetValue("ABLMediaServer", "on_record_progress",""));
	strcpy(ABL_MediaServerPort.on_record_ts, ABL_ConfigFile.GetValue("ABLMediaServer", "on_record_ts",""));
	strcpy(ABL_MediaServerPort.on_delete_record_mp4, ABL_ConfigFile.GetValue("ABLMediaServer", "on_delete_record_mp4",""));
	strcpy(ABL_MediaServerPort.mediaServerID, ABL_ConfigFile.GetValue("ABLMediaServer", "mediaServerID",""));
	strcpy(ABL_MediaServerPort.on_play, ABL_ConfigFile.GetValue("ABLMediaServer", "on_play",""));
	strcpy(ABL_MediaServerPort.on_publish, ABL_ConfigFile.GetValue("ABLMediaServer", "on_publish",""));
	strcpy(ABL_MediaServerPort.on_stream_iframe_arrive, ABL_ConfigFile.GetValue("ABLMediaServer", "on_stream_iframe_arrive",""));
 	
	ABL_MediaServerPort.MaxDiconnectTimeoutSecond = ABL_ConfigFile.GetLongValue("ABLMediaServer", "MaxDiconnectTimeoutSecond",0);
	ABL_MediaServerPort.ForceSendingIFrame = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ForceSendingIFrame",0);
	ABL_MediaServerPort.gb28181LibraryUse = ABL_ConfigFile.GetLongValue("ABLMediaServer", "gb28181LibraryUse",0);
	ABL_MediaServerPort.keepaliveDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "keepaliveDuration",0);
	ABL_MediaServerPort.flvPlayAddMute = ABL_ConfigFile.GetLongValue("ABLMediaServer", "flvPlayAddMute",0);
	ABL_MediaServerPort.nWebRtcPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "webrtcPort",8289);

	//strcpy(ABL_MediaServerPort.defaultSnap, ABL_ConfigFile.GetValue("api", "defaultSnap", ""));
	ABL_MediaServerPort.nUseWvp = ABL_ConfigFile.GetLongValue("api", "usewvp", 0);
	strcpy(ABL_MediaServerPort.port_range, ABL_ConfigFile.GetValue("rtp_proxy", "port_range", ""));
	strcpy(ABL_MediaServerPort.listeningip, ABL_ConfigFile.GetValue("rtc", "listening-ip", ""));
	strcpy(ABL_MediaServerPort.externalip, ABL_ConfigFile.GetValue("rtc", "external-ip", ""));
	ABL_MediaServerPort.listeningport = ABL_ConfigFile.GetLongValue("rtc", "listening-port", 3478);
	ABL_MediaServerPort.minport = ABL_ConfigFile.GetLongValue("rtc", "min-port", 49152);
	ABL_MediaServerPort.maxport = ABL_ConfigFile.GetLongValue("rtc", "max-port", 65535);
	strcpy(ABL_MediaServerPort.realm, ABL_ConfigFile.GetValue("rtc", "realm", ""));
	strcpy(ABL_MediaServerPort.user, ABL_ConfigFile.GetValue("rtc", "user", ""));

	if (ABL_MediaServerPort.httpDownloadSpeed > 10)
		ABL_MediaServerPort.httpDownloadSpeed = 10;
	else if (ABL_MediaServerPort.httpDownloadSpeed <= 0)
		ABL_MediaServerPort.httpDownloadSpeed = 1;

	if (strlen(ABL_MediaServerPort.recordPath) == 0)
		strcpy(ABL_MediaServerPort.recordPath, ABL_MediaSeverRunPath);
	else
	{//用户设置的路径,防止用户没有创建好路径
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.recordPath;
		while (true)
		{
			nPos = strPath.find("/", nPos + 1);
			if (nPos > 0)
			{
				memcpy(szTempPath, ABL_MediaServerPort.recordPath, nPos);
				umask(0);
				mkdir(szTempPath, 777);

				WriteLog(Log_Debug, "创建子路径：%s ", szTempPath);
			}
			else
			{
				umask(0);
				mkdir(ABL_MediaServerPort.recordPath, 777);
				WriteLog(Log_Debug, "创建子路径：%s ", ABL_MediaServerPort.recordPath);
				break;
			}
		}
	}

	if (strlen(ABL_MediaServerPort.picturePath) == 0)
		strcpy(ABL_MediaServerPort.picturePath, ABL_MediaSeverRunPath);
	else
	{//用户设置的路径,防止用户没有创建好路径
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.picturePath;
		while (true)
		{
			nPos = strPath.find("/", nPos + 1);
			if (nPos > 0)
			{
				memcpy(szTempPath, ABL_MediaServerPort.picturePath, nPos);
				umask(0);
				mkdir(szTempPath, 777);

				WriteLog(Log_Debug, "创建子路径：%s ", szTempPath);
			}
			else
			{
				umask(0);
				mkdir(ABL_MediaServerPort.picturePath, 777);
				WriteLog(Log_Debug, "创建子路径：%s ", ABL_MediaServerPort.picturePath);
				break;
			}
		}
	}

	if (strlen(ABL_MediaServerPort.wwwPath) == 0)
		strcpy(ABL_MediaServerPort.wwwPath, ABL_MediaSeverRunPath);
	else
	{//用户设置的路径,防止用户没有创建好路径
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.wwwPath;
		while (true)
		{
			nPos = strPath.find("/", nPos + 1);
			if (nPos > 0)
			{
				memcpy(szTempPath, ABL_MediaServerPort.wwwPath, nPos);
				umask(0);
				mkdir(szTempPath, 777);

				WriteLog(Log_Debug, "创建子路径：%s ", szTempPath);
			}
			else
			{
				umask(0);
				mkdir(ABL_MediaServerPort.wwwPath, 777);
				WriteLog(Log_Debug, "创建子路径：%s ", ABL_MediaServerPort.wwwPath);
				break;
			}
		}
	}
	//创建子路径 record 
	if (ABL_MediaServerPort.recordPath[strlen(ABL_MediaServerPort.recordPath) - 1] != '/')
		strcat(ABL_MediaServerPort.recordPath, "/");
	strcat(ABL_MediaServerPort.recordPath, "record/");
	umask(0);
	mkdir(ABL_MediaServerPort.recordPath, 777);

	//创建调试文件路径
	sprintf(ABL_MediaServerPort.debugPath, "%s/debugFile/", ABL_MediaSeverRunPath);
	umask(0);
	mkdir(ABL_MediaServerPort.debugPath, 777);

	//创建子路径 picture 
	if (ABL_MediaServerPort.picturePath[strlen(ABL_MediaServerPort.picturePath) - 1] != '/')
		strcat(ABL_MediaServerPort.picturePath, "/");
	strcat(ABL_MediaServerPort.picturePath, "picture/");
	umask(0);
	mkdir(ABL_MediaServerPort.picturePath, 777);
	WriteLog(Log_Debug, "创建成功录像路径：%s ,创建图片路径成功：%s ", ABL_MediaServerPort.recordPath, ABL_MediaServerPort.picturePath);

	struct rlimit rlim, rlim_new;
	if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim_new) != 0) {
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			setrlimit(RLIMIT_CORE, &rlim_new);
		}
		WriteLog(Log_Debug, "设置core文件大小为: %llu ", rlim_new.rlim_cur);
	}

	if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_NOFILE, &rlim_new) != 0) {
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			setrlimit(RLIMIT_NOFILE, &rlim_new);
		}
		WriteLog(Log_Debug, "设置最大Socket套接字最大连接数: %llu ", rlim_new.rlim_cur);
	}

	//把历史录像文件装入 list  
	FindHistoryRecordFile(ABL_MediaServerPort.recordPath);
	//把历史图片装入list 
	FindHistoryPictureFile(ABL_MediaServerPort.picturePath);

	rtp_packet_setsize(65535);
	
	if(!ABL_bInitCudaSDKFlag)
	{
		pCudaDecodeHandle = dlopen("libcudaCodecDLL.so",RTLD_LAZY);
		if(pCudaDecodeHandle != NULL)
		{
		  ABL_bInitCudaSDKFlag = true ;	
		  WriteLog(Log_Debug, " dlopen libcudaCodecDLL.so success , NVIDIA graphics card installed  ");
		  
		 cudaCodec_Init = (ABL_cudaDecode_Init)dlsym(pCudaDecodeHandle, "cudaCodec_Init");
			if (cudaCodec_Init != NULL)
				WriteLog(Log_Debug, " dlsym cudaCodec_Init success ");
			cudaCodec_GetDeviceGetCount = (ABL_cudaDecode_GetDeviceGetCount)dlsym(pCudaDecodeHandle, "cudaCodec_GetDeviceGetCount");
			cudaCodec_GetDeviceName = (ABL_cudaDecode_GetDeviceName)dlsym(pCudaDecodeHandle, "cudaCodec_GetDeviceName");
			cudaCodec_GetDeviceUse = (ABL_cudaDecode_GetDeviceUse)dlsym(pCudaDecodeHandle, "cudaCodec_GetDeviceUse");
			cudaCodec_CreateVideoDecode = (ABL_CreateVideoDecode)dlsym(pCudaDecodeHandle, "cudaCodec_CreateVideoDecode");
			cudaCodec_CudaVideoDecode = (ABL_CudaVideoDecode)dlsym(pCudaDecodeHandle, "cudaCodec_CudaVideoDecode");
			cudaCodec_DeleteVideoDecode = (ABL_DeleteVideoDecode)dlsym(pCudaDecodeHandle, "cudaCodec_DeleteVideoDecode");
			cudaCodec_GetCudaDecodeCount = (ABL_GetCudaDecodeCount)dlsym(pCudaDecodeHandle, "cudaCodec_GetCudaDecodeCount");
			cudaCodec_UnInit = (ABL_VideoDecodeUnInit)dlsym(pCudaDecodeHandle, "cudaCodec_UnInit");
		}else
		  WriteLog(Log_Debug, " dlopen libcudaCodecDLL.so failed , NVIDIA graphics card is not installed  ");

		pCudaEncodeHandle = dlopen("libcudaEncodeDLL.so",RTLD_LAZY);
		if(pCudaEncodeHandle != NULL)
		{
		  WriteLog(Log_Debug, " dlopen libcudaEncodeDLL.so success , NVIDIA graphics card installed  ");
		  cudaEncode_Init = (ABL_cudaEncode_Init)dlsym(pCudaEncodeHandle, "cudaEncode_Init");
		  cudaEncode_GetDeviceGetCount = (ABL_cudaEncode_GetDeviceGetCount)dlsym(pCudaEncodeHandle, "cudaEncode_GetDeviceGetCount");
		  cudaEncode_GetDeviceName = (ABL_cudaEncode_GetDeviceName)dlsym(pCudaEncodeHandle, "cudaEncode_GetDeviceName");
		  cudaEncode_CreateVideoEncode = (ABL_cudaEncode_CreateVideoEncode)dlsym(pCudaEncodeHandle, "cudaEncode_CreateVideoEncode");
		  cudaEncode_DeleteVideoEncode = (ABL_cudaEncode_DeleteVideoEncode)dlsym(pCudaEncodeHandle, "cudaEncode_DeleteVideoEncode");
		  cudaEncode_CudaVideoEncode= (ABL_cudaEncode_CudaVideoEncode)dlsym(pCudaEncodeHandle, "cudaEncode_CudaVideoEncode");
		  cudaEncode_UnInit = (ABL_cudaEncode_UnInit)dlsym(pCudaEncodeHandle, "cudaEncode_UnInit");
		}  
		else
		  WriteLog(Log_Debug, " dlopen libcudaEncodeDLL.so failed , NVIDIA graphics card is not installed  ");
	  
	   if(ABL_bInitCudaSDKFlag)
	   {
	     bool bRet1 = cudaCodec_Init();
         bool bRet2 = cudaEncode_Init();
 	     WriteLog(Log_Debug, "cudaCodec_Init()= %d , cudaEncode_Init() = %d ", bRet1,bRet2);
		 if(!(bRet1 && bRet2 ))
			ABL_MediaServerPort.H265DecodeCpuGpuType = 0 ;//恢复为软解码 
	   }else 
		  ABL_MediaServerPort.H265DecodeCpuGpuType = 0 ;//恢复为软解码

	   WriteLog(Log_Debug, " H265DecodeCpuGpuType = %d ",ABL_MediaServerPort.H265DecodeCpuGpuType );
	}
#endif
	WriteLog(Log_Debug, "....高性能流媒体服务器 ABLMediaServer Start ....");
	printfVersion();
	WriteLog(Log_Debug, "从配置文件中读取到 \r\n运行参数：http = %d, rtsp = %d , rtmp = %d ,http-flv = %d ,ws-flv = %d ,http-mp4 = %d, nHlsPort = %d , nHlsEnable = %d nHLSCutType = %d \r\n 网络接收线程数量 RecvThreadCount = %d 网络发送线程数量 SendThreadCount = %d ",
		                     ABL_MediaServerPort.nHttpServerPort,ABL_MediaServerPort.nRtspPort, ABL_MediaServerPort.nRtmpPort, ABL_MediaServerPort.nHttpFlvPort, ABL_MediaServerPort.nWSFlvPort, ABL_MediaServerPort.nHttpMp4Port, ABL_MediaServerPort.nHlsPort, ABL_MediaServerPort.nHlsEnable , ABL_MediaServerPort.nHLSCutType,
		                     ABL_MediaServerPort.nRecvThreadCount, ABL_MediaServerPort.nSendThreadCount);

	if (ABL_MediaServerPort.hlsCutTime <= 0)
		ABL_MediaServerPort.hlsCutTime = 1 ;
	else if (ABL_MediaServerPort.hlsCutTime > 120)
		ABL_MediaServerPort.hlsCutTime = 120;
	
	//限制网络超时时长
	if (ABL_MediaServerPort.MaxDiconnectTimeoutSecond < 5)
		ABL_MediaServerPort.MaxDiconnectTimeoutSecond = 16;
	else if (ABL_MediaServerPort.MaxDiconnectTimeoutSecond > 200)
		ABL_MediaServerPort.MaxDiconnectTimeoutSecond = 200;
	if (ABL_MediaServerPort.keepaliveDuration <= 0)
		ABL_MediaServerPort.keepaliveDuration = 20;

	ABL_bMediaServerRunFlag = true;
	pDisconnectBaseNetFifo.InitFifo(1024 * 1024 * 4);
	pReConnectStreamProxyFifo.InitFifo(1024 * 1024 * 4);
	pMessageNoticeFifo.InitFifo(1024 * 1024 * 4);
	pNetBaseObjectFifo.InitFifo(1024 * 1024 * 2);

	//创建www子路径 
#ifdef OS_System_Windows
	if (ABL_MediaServerPort.wwwPath[strlen(ABL_MediaServerPort.wwwPath) - 1] != '\\')
		strcat(ABL_MediaServerPort.wwwPath, "\\");
	sprintf(ABL_wwwMediaPath, "%swww", ABL_MediaServerPort.wwwPath);
	::CreateDirectory(ABL_wwwMediaPath, NULL);
#else
	if (ABL_MediaServerPort.wwwPath[strlen(ABL_MediaServerPort.wwwPath) - 1] != '/')
		strcat(ABL_MediaServerPort.wwwPath, "/");
	sprintf(ABL_wwwMediaPath, "%swww", ABL_MediaServerPort.wwwPath);
	umask(0);
	mkdir(ABL_wwwMediaPath, 777);
#endif
	WriteLog(Log_Debug, "www 路径为 %s ", ABL_wwwMediaPath);

	//防止用户乱填写
	if ((ABL_MediaServerPort.snapOutPictureWidth == 0 && ABL_MediaServerPort.snapOutPictureHeight != 0) || (ABL_MediaServerPort.snapOutPictureWidth != 0 && ABL_MediaServerPort.snapOutPictureHeight == 0))
		ABL_MediaServerPort.snapOutPictureWidth = ABL_MediaServerPort.snapOutPictureHeight = 0;
	if (ABL_MediaServerPort.snapOutPictureWidth > 1920)
		ABL_MediaServerPort.snapOutPictureWidth = 1920;
	if (ABL_MediaServerPort.snapOutPictureHeight > 1080)
		ABL_MediaServerPort.snapOutPictureHeight = 1080;

	//用于网络数据接收
	NetBaseThreadPool = new CNetBaseThreadPool(ABL_nCurrentSystemCpuCount);

	//录像回放线程池
	RecordReplayThreadPool = new CNetBaseThreadPool(ABL_MediaServerPort.nRecordReplayThread);

	//消息发送线程池
	MessageSendThreadPool = new CNetBaseThreadPool(6);

	int nRet = -1;
	if (!ABL_bInitXHNetSDKFlag) //保证只初始化1次
	{
		if (ABL_nCurrentSystemCpuCount > 0 && ABL_nCurrentSystemCpuCount <= 4)
			nRet = XHNetSDK_Init(ABL_nCurrentSystemCpuCount * 4, 1);
		else if (ABL_nCurrentSystemCpuCount > 4 && ABL_nCurrentSystemCpuCount <= 8)
			nRet = XHNetSDK_Init(ABL_nCurrentSystemCpuCount * 3, 1);
		else if (ABL_nCurrentSystemCpuCount > 8 && ABL_nCurrentSystemCpuCount <= 32)
			nRet = XHNetSDK_Init(ABL_nCurrentSystemCpuCount * 2, 1);
		else
			nRet = XHNetSDK_Init(ABL_nCurrentSystemCpuCount, 1);

		ABL_bInitXHNetSDKFlag = true;
		WriteLog(Log_Debug, "Network Init = %d \r\n", nRet);
	}

	nBindHttp = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHttpServerPort, &srvhandle_8080, onaccept, onread, onclose, true);
	nBindRtsp = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nRtspPort, &srvhandle_554, onaccept, onread, onclose, true);
	nBindRtmp = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nRtmpPort, &srvhandle_1935, onaccept, onread, onclose, true);
	nBindWsFlv = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nWSFlvPort, &srvhandle_6088, onaccept, onread, onclose, true);
	nBindHttpFlv = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHttpFlvPort, &srvhandle_8088, onaccept, onread, onclose, true);
	nBindHls = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHlsPort, &srvhandle_9088, onaccept, onread, onclose, true);
	nBindMp4 = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHttpMp4Port, &srvhandle_8089, onaccept, onread, onclose, true);
	nBindRecvAudio = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.WsRecvPcmPort, &srvhandle_9298, onaccept, onread, onclose, true);
	nBingPS10000 = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.ps_tsRecvPort, &srvhandle_10000, onaccept, onread, onclose, true);

	WriteLog(Log_Debug, (nBindHttp == 0) ? "绑定http 端口%d 成功(success) " : "绑定http 端口 %d 失败(fail) ", ABL_MediaServerPort.nHttpServerPort);
	WriteLog(Log_Debug, (nBindRtsp == 0) ? "绑定rtsp端口 %d 成功(success)  " : "绑定rtsp端口 %d 失败(fail) ", ABL_MediaServerPort.nRtspPort);
	WriteLog(Log_Debug, (nBindRtmp == 0) ? "绑定rtmp端口 %d 成功(success)  " : "绑定rtmp端口 %d 失败(fail) ", ABL_MediaServerPort.nRtmpPort);
	WriteLog(Log_Debug, (nBindWsFlv == 0) ? "绑定wsflv端口 %d 成功(success)  " : "绑定wsflv端口 %d 失败(fail) ", ABL_MediaServerPort.nWSFlvPort);
	WriteLog(Log_Debug, (nBindHttpFlv == 0) ? "绑定HttpFlvP端口 %d 成功(success)  " : "绑定HttpFlvP端口 %d 失败(fail) ", ABL_MediaServerPort.nHttpFlvPort);
	WriteLog(Log_Debug, (nBindHls == 0) ? "绑定Hls端口 %d 成功(success)  " : "绑定Hls端口 %d 失败(fail) ", ABL_MediaServerPort.nHlsPort);
	WriteLog(Log_Debug, (nBindMp4 == 0) ? "绑定HttpMp4Por端口 %d 成功(success)  " : "绑定HttpMp4Por端口 %d 失败(fail) ", ABL_MediaServerPort.nHttpMp4Port);
	WriteLog(Log_Debug, (nBindRecvAudio == 0) ? "绑定WsRecvPcmPort端口 %d 成功(success)  " : "绑定WsRecvPcmPort端口 %d 失败(fail) ", ABL_MediaServerPort.WsRecvPcmPort);
	WriteLog(Log_Debug, (nBingPS10000 == 0) ? "绑定ps_tsRecvPort端口 %d(tcp) 成功(success)  " : "绑定ps_tsRecvPort端口 %d(tcp) 失败(fail) ", ABL_MediaServerPort.ps_tsRecvPort);

	alaw_pcm16_tableinit();
	ulaw_pcm16_tableinit();

	ABL_MediaServerPort.nServerStartTime = GetCurrentSecond();
	ABL_MediaServerPort.nServerKeepaliveTime = GetTickCount64();

#if  0 //测试 hls 客户端  http://190.168.24.112:8082/live/Camera_00001.m3u8  \   http://190.15.240.36:9088/Media/Camera_00001.m3u8 \ http://190.15.240.36:9088/Media/Camera_00001/hls.m3u8
	//CreateNetRevcBaseClient(0, 0, "http://190.168.24.112:8082/live/Camera_00001.m3u8", 0);
	CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, "http://190.15.240.36:9088/Media/Camera_00001.m3u8", 0, "/Media/Camera_00002");
#endif
#if   0 //测试 rtmp 客户端  rtmp://10.0.0.239:1936/Media/Camera_00001  \   rtmp://10.0.0.239:1936/Media/Camera_00001
	CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, "rtmp://190.15.240.36:1935/Media/Camera_00001", 0, "/Media/Camera_00002");
#endif
#if   0 //测试 flv  http://190.15.240.36:8088/Media/Camera_00001.flv
	CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, "http://190.15.240.36:8088/Media/Camera_00001.flv", 0, "/Media/Camera_00002");
#endif
#if   0 //测试 rtmp 客户端  rtmp://10.0.0.239:1936/Media/Camera_00001  \   rtmp://10.0.0.239:1936/Media/Camera_00001
	CNetRevcBase_ptr rtmpClient = CreateNetRevcBaseClient(NetRevcBaseClient_addPushStreamProxy, 0, 0, "rtmp://190.15.240.36:1935/Media/Camera_00001", 0, "/Media/Camera_00001");
	nTestRtmpPushID = rtmpClient->nClient;
#endif
#if  0 //测试 rtmp 客户端  rtmp://10.0.0.239:1936/Media/Camera_00001  \   rtmp://10.0.0.239:1936/Media/Camera_00001
	CNetRevcBase_ptr rtmpClient = CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy, 0, 0, "rtsp://admin:abldyjh2020@192.168.1.120:554", 0, "/Media/Camera_00001");
	nTestRtmpPushID = rtmpClient->nClient;
#endif
#if   0 //测试 rtsp 客户端  
	CreateNetRevcBaseClient(NetRevcBaseClient_addPushStreamProxy, 0, 0, "rtsp://10.0.0.238:554/Media/Camera_00001", 0, "/Media/Camera_00002");
#endif
#if 0
	CreateNetRevcBaseClient(NetBaseNetType_HttpMP4ServerSendPush, 0, 0, "rtsp://10.0.0.238:554/Media/Camera_00001", 0, "/Media/Camera_00002");
#endif
#if 0
	CreateNetRevcBaseClient(ReadRecordFileInput_ReadFMP4File, 0, 0, "D:\\video\\20220118165107.mp4", 0, "/Media/Camera_00001");
	//	CreateNetRevcBaseClient(ReadRecordFileInput_ReadFMP4File, 0, 0, "D:\\video\\20220119161822.mp4", 0, "/Media/Camera_00001");
		//CreateNetRevcBaseClient(ReadRecordFileInput_ReadFMP4File, 0, 0, "D:\\video\\20220119162324.mp4", 0, "/Media/Camera_00001");
#endif
#if   0//测试消息通知  http://10.0.0.238:7088/index/hook/on_stream_none_reader
	CreateNetRevcBaseClient(NetBaseNetType_HttpClient_None_reader, 0, 0, "http://10.0.0.238:7088/index/hook/on_stream_none_reader", 0, "");
#endif

	//创建单端口国标接收 
	CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RecvRtpPS_TS, 0, 0, "", 0, "");

	//启动业务处理线程
#ifdef  OS_System_Windows
	unsigned long dwThread, dwThread2;
	::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ABLMedisServerProcessThread, (LPVOID)NULL, 0, &dwThread);
	::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ABLMedisServerFastDeleteThread, (LPVOID)NULL, 0, &dwThread2);
#else
	pthread_t  hMedisServerProcessThread;
	pthread_create(&hMedisServerProcessThread, NULL, ABLMedisServerProcessThread, (void*)NULL);
	pthread_t  hMedisServerProcessThread2;
	pthread_create(&hMedisServerProcessThread2, NULL, ABLMedisServerFastDeleteThread, (void*)NULL);
#endif

	char szWebrtcConfig[512] = { 0 };
	sprintf(szWebrtcConfig, "{\"webrtcPort\":%d}", ABL_MediaServerPort.nWebRtcPort);
	WebRtcEndpoint::getInstance().init(szWebrtcConfig, [=](const char* callbackJson, void* pUserHandle) {
		WebRtcCallBack(callbackJson, pUserHandle);
		});
	WriteLog(Log_Debug, "初始化webrtc完毕 ，端口为 %d", ABL_MediaServerPort.nWebRtcPort);

	while (ABL_bMediaServerRunFlag)
	{
		//服务器上线通知 
		if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.bNoticeStartEvent == false)
		{
			if ((GetCurrentSecond() - ABL_MediaServerPort.nServerStartTime) > 6 && (GetCurrentSecond() - ABL_MediaServerPort.nServerStartTime) <= 15)
			{
				ABL_MediaServerPort.bNoticeStartEvent = true;
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = NetBaseNetType_HttpClient_ServerStarted;

#ifdef OS_System_Windows
				SYSTEMTIME st;
				GetLocalTime(&st);
				sprintf(msgNotice.szMsg, "{\"localipAddress\":\"%s\",\"mediaServerId\":\"%s\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.mediaServerID, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
				time_t now;
				time(&now);
				struct tm* local;
				local = localtime(&now);
				sprintf(msgNotice.szMsg, "{\"localipAddress\":\"%s\",\"mediaServerId\":\"%s\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.mediaServerID, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
#endif
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
 	}
 
	ABL_bMediaServerRunFlag = false;
	while (!ABL_bExitMediaServerRunFlag)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//Sleep(100);
	XHNetSDK_Unlisten(srvhandle_8080);
	XHNetSDK_Unlisten(srvhandle_554);
	XHNetSDK_Unlisten(srvhandle_1935);
	XHNetSDK_Unlisten(srvhandle_8088);
	XHNetSDK_Unlisten(srvhandle_6088);
	XHNetSDK_Unlisten(srvhandle_8089);
	XHNetSDK_Unlisten(srvhandle_9088);
	XHNetSDK_Unlisten(srvhandle_9298);
	XHNetSDK_Unlisten(srvhandle_10000);

	delete NetBaseThreadPool;
	NetBaseThreadPool = NULL;

	delete RecordReplayThreadPool;
	RecordReplayThreadPool = NULL;

	delete MessageSendThreadPool;
	MessageSendThreadPool = NULL;

	pDisconnectBaseNetFifo.FreeFifo();
	pReConnectStreamProxyFifo.FreeFifo();
	pMessageNoticeFifo.FreeFifo();
	pNetBaseObjectFifo.FreeFifo();

	xh_ABLRecordFileSourceMap.clear();
	xh_ABLPictureFileSourceMap.clear();

#ifdef OS_System_Windows
	CloseHandle(hRunAsOne);
	StopLogFile();
#else
	ExitLogFile();
#endif
	WebRtcEndpoint::getInstance().Uninit();

	WriteLog(Log_Debug, "--------------------ABLMediaServer End .... --------------------");

	malloc_trim(0);

	ABL_bMediaServerRunFlag = true;
	if (ABL_bRestartServerFlag)
	{
		ABL_MediaServerPort.nServerStartTime = GetCurrentSecond();
		ABL_MediaServerPort.nServerKeepaliveTime = GetTickCount64();
		ABL_MediaServerPort.bNoticeStartEvent = false;
		ABL_bRestartServerFlag = false;
		memset((char*)&ABL_MediaServerPort, 0x00, sizeof(ABL_MediaServerPort));//把配置文件全部清除，重新读取 
		goto ABL_Restart;
	}
	XHNetSDK_Deinit();

	//cuda 硬件解码，编码资源释放 
#ifdef OS_System_Windows
	if (ABL_bInitCudaSDKFlag)
		cudaCodec_UnInit();
#else
	if (ABL_bInitCudaSDKFlag && pCudaDecodeHandle != NULL && pCudaEncodeHandle != NULL)
	{
		cudaCodec_UnInit();
		cudaEncode_UnInit();

		dlclose(pCudaDecodeHandle);
		dlclose(pCudaEncodeHandle);
	}
#endif

	return 0;
}

