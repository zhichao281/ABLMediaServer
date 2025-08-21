/*
���ܣ�
        ��������ý�������(ABLMediaServer)�����

		ý�����뷽ʽ             ֧������Ƶ��ʽ
----------------------------------------------------------------------------------------
				1��rtsp          ��ƵH264��H265����ƵAAC��G711A��G711U)
				2��rtmp          ��ƵH264��      ��ƵAAC��G711A��G711U)      
				3������GB28181   ��ƵH264��H265����ƵAAC��G711A��G711U)
				4��WebRTC        ��ƵH264��H265����ƵAAC��G711A��G711U)
				5��
		ý���������ʽ
----------------------------------------------------------------------------------------
		        1��rtsp          ��ƵH264��H265����ƵAAC��G711A��G711U)
				2��rtmp          ��ƵH264��H265����ƵAAC��G711A��G711U) 
				3��http-flv      ��ƵH264��H265����ƵAAC��G711A��G711U) 
				4��m3u8          ��ƵH264��H265����ƵAAC��G711A��G711U) 
				5��fmp4          ��ƵH264��H265����ƵAAC��G711A��G711U) 
				6������GB28181   ��ƵH264��H265����ƵAAC��G711A��G711U) 
				7��WebRTC        ��ƵH264��H265����ƵAAC��G711A��G711U) 
				8��ws-flv        ��ƵH264��H265����ƵAAC��G711A��G711U) 

����    2021-04-02
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "../webrtc-streamer/inc/rtc_obj_sdk.h"

NETHANDLE srvhandle_8080,srvhandle_554, srvhandle_1935, srvhandle_6088, srvhandle_8088, srvhandle_8089, srvhandle_9088, srvhandle_9298, srvhandle_10000, srvhandle_1078, srvhandle_8192;
#ifdef USE_BOOST
typedef boost::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef boost::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;
CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;
std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
CNetBaseThreadPool*                                              NetBaseThreadPool;
CNetBaseThreadPool*                                              RecordReplayThreadPool;//¼��ط��̳߳�
CNetBaseThreadPool*                                              MessageSendThreadPool;//��Ϣ�����̳߳�
CNetBaseThreadPool*                                              HttpProcessThreadPool;//ר�Ŵ��������̳߳أ���Ϊ¼���ѯ��Ҫ�ȵ�¼���������ɲŷ��ؽ�� 

/* ý�����ݴ洢 -------------------------------------------------------------------------------------*/
typedef boost::shared_ptr<CMediaStreamSource>                    CMediaStreamSource_ptr;
typedef boost::unordered_map<string, CMediaStreamSource_ptr>     CMediaStreamSource_ptrMap;
CMediaStreamSource_ptrMap                                        xh_ABLMediaStreamSourceMap;
std::mutex                                                       ABL_CMediaStreamSourceMapLock;

/* ¼���ļ��洢 -------------------------------------------------------------------------------------*/
typedef boost::shared_ptr<CRecordFileSource>                     CRecordFileSource_ptr;
typedef boost::unordered_map<string, CRecordFileSource_ptr>      CRecordFileSource_ptrMap;
CRecordFileSource_ptrMap                                         xh_ABLRecordFileSourceMap;
std::mutex                                                       ABL_CRecordFileSourceMapLock;

/* ͼƬ�ļ��洢 -------------------------------------------------------------------------------------*/
typedef boost::shared_ptr<CPictureFileSource>                    CPictureFileSource_ptr;
typedef boost::unordered_map<string, CPictureFileSource_ptr>     CPictureFileSource_ptrMap;
CPictureFileSource_ptrMap                                        xh_ABLPictureFileSourceMap;
std::mutex                                                       ABL_CPictureFileSourceMapLock;

uint64_t                                                         ArrayAddMutePacketList[8192];//���Ӿ��������б�
uint64_t                                                         nMaxAddMuteListNumber = 0; //���һ�������������ڵ����

volatile bool                                                    ABL_bMediaServerRunFlag = true ;
volatile bool                                                    ABL_bExitMediaServerRunFlag = false; //�˳������̱߳�־ 
CMediaFifo                                                       pDisconnectBaseNetFifo;             //������ѵ����� 
CMediaFifo                                                       pReConnectStreamProxyFifo;          //��Ҫ�������Ӵ���ID 
CMediaFifo                                                       pMessageNoticeFifo;          //��Ϣ֪ͨFIFO
CMediaFifo                                                       pNetBaseObjectFifo;          //�洢�������ID
CMediaFifo                                                       pDisconnectMediaSource;      //�������ý��Դ 
char                                                             ABL_MediaSeverRunPath[256] = { 0 }; //��ǰ·��
char                                                             ABL_wwwMediaPath[256] = { 0 }; //www ��·��
uint64_t                                                         ABL_nBaseCookieNumber = 100; //Cookie ��� 
char                                                             ABL_szLocalIP[128] = { 0 };
uint64_t                                                         ABL_nPrintCheckNetRevcBaseClientDisconnect = 0;
unsigned int                                                     ABL_nCurrentSystemCpuCount = 4;//��ǰϵͳcpu��������� 
CNetRevcBase_ptr                                                 GetNetRevcBaseClient(NETHANDLE CltHandle);
bool 	                                                         ABL_bCudaFlag  = false ;
int                                                              ABL_nCudaCount = 0 ;
volatile bool                                                    ABL_bRestartServerFlag = false;
volatile bool                                                    ABL_bInitXHNetSDKFlag = false;
volatile bool                                                    ABL_bInitCudaSDKFlag = false;
char                                                             szConfigFileName[512] = { 0 };
CNetRevcBase_ptr                                                 CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
CMediaStreamSource_ptr                                           GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound );

#else
typedef std::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef std::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;
CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;
std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
CNetBaseThreadPool* NetBaseThreadPool;
CNetBaseThreadPool* RecordReplayThreadPool;//¼��ط��̳߳�
CNetBaseThreadPool* MessageSendThreadPool;//��Ϣ�����̳߳�
CNetBaseThreadPool* HttpProcessThreadPool;//ר�Ŵ��������̳߳أ���Ϊ¼���ѯ��Ҫ�ȵ�¼���������ɲŷ��ؽ�� 

/* ý�����ݴ洢 -------------------------------------------------------------------------------------*/
typedef std::shared_ptr<CMediaStreamSource>                    CMediaStreamSource_ptr;
typedef std::unordered_map<string, CMediaStreamSource_ptr>     CMediaStreamSource_ptrMap;
CMediaStreamSource_ptrMap                                        xh_ABLMediaStreamSourceMap;
std::mutex                                                       ABL_CMediaStreamSourceMapLock;

/* ¼���ļ��洢 -------------------------------------------------------------------------------------*/
typedef std::shared_ptr<CRecordFileSource>                     CRecordFileSource_ptr;
typedef std::unordered_map<string, CRecordFileSource_ptr>      CRecordFileSource_ptrMap;
CRecordFileSource_ptrMap                                         xh_ABLRecordFileSourceMap;
std::mutex                                                       ABL_CRecordFileSourceMapLock;

/* ͼƬ�ļ��洢 -------------------------------------------------------------------------------------*/
typedef std::shared_ptr<CPictureFileSource>                    CPictureFileSource_ptr;
typedef std::unordered_map<string, CPictureFileSource_ptr>     CPictureFileSource_ptrMap;
CPictureFileSource_ptrMap                                        xh_ABLPictureFileSourceMap;
std::mutex                                                       ABL_CPictureFileSourceMapLock;

uint64_t                                                         ArrayAddMutePacketList[8192];//���Ӿ��������б�
uint64_t                                                         nMaxAddMuteListNumber = 0; //���һ�������������ڵ����

volatile bool                                                    ABL_bMediaServerRunFlag = true;
volatile bool                                                    ABL_bExitMediaServerRunFlag = false; //�˳������̱߳�־ 
CMediaFifo                                                       pDisconnectBaseNetFifo;             //������ѵ����� 
CMediaFifo                                                       pReConnectStreamProxyFifo;          //��Ҫ�������Ӵ���ID 
CMediaFifo                                                       pMessageNoticeFifo;          //��Ϣ֪ͨFIFO
CMediaFifo                                                       pNetBaseObjectFifo;          //�洢�������ID
CMediaFifo                                                       pDisconnectMediaSource;      //�������ý��Դ 
char                                                             ABL_MediaSeverRunPath[256] = { 0 }; //��ǰ·��
char                                                             ABL_wwwMediaPath[256] = { 0 }; //www ��·��
uint64_t                                                         ABL_nBaseCookieNumber = 100; //Cookie ��� 
char                                                             ABL_szLocalIP[128] = { 0 };
uint64_t                                                         ABL_nPrintCheckNetRevcBaseClientDisconnect = 0;
unsigned int                                                     ABL_nCurrentSystemCpuCount = 4;//��ǰϵͳcpu��������� 
CNetRevcBase_ptr                                                 GetNetRevcBaseClient(NETHANDLE CltHandle);
bool 	                                                         ABL_bCudaFlag = false;
int                                                              ABL_nCudaCount = 0;
volatile bool                                                    ABL_bRestartServerFlag = false;
volatile bool                                                    ABL_bInitXHNetSDKFlag = false;
volatile bool                                                    ABL_bInitCudaSDKFlag = false;
char                                                             szConfigFileName[512] = { 0 };
CNetRevcBase_ptr                                                 CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
CMediaStreamSource_ptr                                           GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound);

#endif  //USE_BOOST

#ifdef OS_System_Windows
//cuda ���� 
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

//����Ҫ����
bool   AddClientToMapAddMutePacketList(uint64_t nClient)
{
	bool bRet = false ;

	for (int i = 0; i < 8192; i++)
	{
		if (bRet == false && ArrayAddMutePacketList[i] == 0)
		{
			ArrayAddMutePacketList[i] = nClient;
			bRet = true;
 		}

		//�����������
		if (ArrayAddMutePacketList[i] > 0 && (i + 1) > nMaxAddMuteListNumber)
			nMaxAddMuteListNumber = i + 1;
	}
 
	return bRet;
}

bool   DelClientToMapFromMutePacketList(uint64_t nClient)
{
	bool bRet = false;
	int  nMaxNumber = 0 ;
	for (int i = 0; i < nMaxAddMuteListNumber ; i++)
	{
		if (bRet == false && ArrayAddMutePacketList[i] == nClient)
		{
			ArrayAddMutePacketList[i] = 0;
			bRet = true;
		}

		//�����������
		if (ArrayAddMutePacketList[i] > 0 && (i + 1) > nMaxNumber)
			nMaxNumber = i + 1;
	}
	nMaxAddMuteListNumber = nMaxNumber;

	return bRet;
}

//����������ת��Ϊ��
uint64_t GetCurrentSecondByTime(char* szDateTime)
{
	if (szDateTime == NULL || strlen(szDateTime) < 14)
		return 0;

	time_t clock;
	struct tm tm ;
 
 	sscanf(szDateTime, "%04d%02d%02d%02d%02d%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
 	tm.tm_year = tm.tm_year - 1900;
	tm.tm_mon = tm.tm_mon - 1;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	return clock;
}

//����������ת��Ϊ������ 
char szCurrentDateTime[256] = { 0 };
char* GetDateTimeBySeconds(uint64_t nSeconds)
{
	struct tm *local;
	local = localtime((time_t*)&nSeconds);
	sprintf(szCurrentDateTime, "%04d%02d%02d%02d%02d%02d",local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

	return szCurrentDateTime;
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

BOOL GBK2UTF8(char *szGbk, char *szUtf8, int Len)
{
	// �Ƚ����ֽ�GBK��CP_ACP��ANSI��ת���ɿ��ַ�UTF-16  
	// �õ�ת��������Ҫ���ڴ��ַ���  
	int n = MultiByteToWideChar(CP_ACP, 0, szGbk, -1, NULL, 0);
	// �ַ������� sizeof(WCHAR) �õ��ֽ���  
	WCHAR *str1 = new WCHAR[sizeof(WCHAR) * n];
	// ת��  
	MultiByteToWideChar(CP_ACP,  // MultiByte�Ĵ���ҳCode Page  
		0,            //���ӱ�־���������й�  
		szGbk,        // �����GBK�ַ���  
		-1,           // �����ַ������ȣ�-1��ʾ�ɺ����ڲ�����  
		str1,         // ���  
		n             // ������������ڴ�  
	);

	// �ٽ����ַ���UTF-16��ת�����ֽڣ�UTF-8��  
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
unsigned short                                                   ABL_nGB28181Port = 35001 ;
uint32_t                                                         ABL_nSSRC = 28888;//ssrcĬ�ϵĿ�ʼֵ

#ifndef OS_System_Windows

//�ж�·���Ƿ���� 
bool isPathExist(char* szPath)
{
	if(strlen(szPath) > 0 && szPath[strlen(szPath) - 1] != '/')
	   strcat(szPath,"/");	

	DIR* dir = opendir(szPath) ;
	
	if(dir != NULL)
	{
		closedir(dir);
		return true ;
	}else
	{
		return false ;
	}
}

//����·��Ȩ�� 
void  ABL_SetPathAuthority(char* szPath)
{
    char szCmd[2048]={0};
 	sprintf(szCmd,"cd %s",szPath);
	system(szCmd) ;
	system("chmod -R 777 *");
}

int GB2312ToUTF8(char* szSrc, size_t iSrcLen, char* szDst, size_t iDstLen)
{
      iconv_t cd = iconv_open("utf-8//IGNORE", "gb2312//IGNORE");
      if(0 == cd)
         return -2;
      memset(szDst, 0, iDstLen);
      char **src = &szSrc;
      char **dst = &szDst;
      if(-1 == (int)iconv(cd, src, &iSrcLen, dst, &iDstLen))
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
	return (tv.tv_sec) ;
}

//��ʱ
void  Sleep(int mMicroSecond)
{
	if (mMicroSecond > 0)
		usleep(mMicroSecond * 1000);
	else
		usleep(5 * 1000);
}

bool GetLocalAdaptersInfo(string& strIPList)
{
	struct ifaddrs *ifaddr, *ifa;

	int family, s;

	char szAllIPAddress[4096]={0};
	char host[NI_MAXHOST] = {0};

	if (getifaddrs(&ifaddr) == -1) 
	{ //ͨ��getifaddrs�����õ�����������Ϣ
  		return false ;
 	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
	{ //��������ѭ��

		if (ifa->ifa_addr == NULL) //�жϵ�ַ�Ƿ�Ϊ��
 			continue;

		family = ifa->ifa_addr->sa_family; //�õ�IP��ַ��Э����
 
		if (family == AF_INET ) 
		{ //�ж�Э������AF_INET����AF_INET6
	        memset(host,0x00, NI_MAXHOST);
			 
 		    //ͨ��getnameinfo�����õ���Ӧ��IP��ַ��NI_MAXHOSTΪ�궨�壬ֵΪ1025. NI_NUMERICHOST�궨�壬��NI_NUMERICSERV��Ӧ������һ�¾�֪���ˡ�
 			s = getnameinfo(ifa->ifa_addr,
 				(family == AF_INET) ? sizeof(struct sockaddr_in) :
 				sizeof(struct sockaddr_in6),
 				host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			 if( !(strcmp(host,"127.0.0.1") == 0 || strcmp(host, "0.0.0.0") == 0) )
			 {
			   strcat(szAllIPAddress,host);
			   strcat(szAllIPAddress,",");
			 }	
			 
			 if (s != 0) 
			 {
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
  			 }
       	}
	}
	WriteLog(Log_Debug, "szAllIPAddress = %s ",szAllIPAddress);
	strIPList = szAllIPAddress;
	
    return true ;
}

#endif

//�������ң�������Ѿ�����
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
	//��ȡ�ṩԴ
	CNetRevcBase_ptr pSourceClient = GetNetRevcBaseClient(nClient);
	if (pSourceClient == NULL)
	{
		WriteLog(Log_Debug, "ý��Դ�ṩ�߲�����  nClient = %llu ", nClient);
		return NULL;
	}
	strcpy(pSourceClient->m_szShareMediaURL, szURL);
	pSourceClient->SplitterAppStream(pSourceClient->m_szShareMediaURL);

	std::lock_guard<std::mutex> lock(ABL_CMediaStreamSourceMapLock);

	
	CMediaStreamSource_ptr pXHClient = NULL;
	string                 strURL = szURL;
 
	//�Ȳ����Ƿ���ڣ���������򷵻�ԭ�����ڵģ���֤�����ָ����ɱ���
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
 		   pXHClient = std::make_shared<CMediaStreamSource>(szURL,nClient, nSourceType, nDuration, h265ConvertH264Struct);
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
#ifdef OS_System_Windows
	string      szRecordPathName = pSourceClient->m_addStreamProxyStruct.url;

#ifdef USE_BOOST
	replace_all(szRecordPathName, "\\", "/");

#else
	ABL::replace_all(szRecordPathName, "\\", "/");

#endif


	strcpy(pXHClient->sourceURL, szRecordPathName.c_str());
#else 
	strcpy(pXHClient->sourceURL, pSourceClient->m_addStreamProxyStruct.url);
#endif		

	return pXHClient;
}

CMediaStreamSource_ptr GetMediaStreamSource(char* szURL,bool bNoticeStreamNoFound=false)
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
		//�����Ҳ���
		if (ABL_MediaServerPort.hook_enable == 1 && bNoticeStreamNoFound && strstr(szURL, RecordFileReplaySplitter) == NULL)
		{
			int      nPos2 = 0;
			char     szApp[512] = { 0 };
			char     szStream[512] = { 0 };
			string   strMediaSource = szURL;

			nPos2 =  strMediaSource.find("/", 1);
			if (nPos2 > 0 && nPos2 != string::npos && strlen(szURL) < 512 )
			{
 				memcpy(szApp, szURL + 1, nPos2 - 1);
				memcpy(szStream, szURL + nPos2 + 1, strlen(szURL) - nPos2 - 1);
			}

			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_Not_found;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_not_found\",\"app\":\"%s\",\"stream\":\"%s\",\"mediaServerId\":\"%s\"}", szApp, szStream, ABL_MediaServerPort.mediaServerID);
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

	CMediaStreamSource_ptrMap::iterator iterator1;

	iterator1 = xh_ABLMediaStreamSourceMap.find(szURL);
	if (iterator1 != xh_ABLMediaStreamSourceMap.end())
	{
		(*iterator1).second->bEnableFlag = false ;
		
		//ɾ������������
		(*iterator1).second->addClientToDisconnectFifo();

		//ý�����ʱ֪ͨ
		if (ABL_MediaServerPort.hook_enable == 1 && (strlen((*iterator1).second->m_mediaCodecInfo.szVideoName) > 0 || strlen((*iterator1).second->m_mediaCodecInfo.szAudioName) > 0) )
		{
			if (ABL_MediaServerPort.nReConnectingCount == 0)
			{//���߲���Ҫ������ֱ��ɾ��ý��Դ�ṩ�ߣ����������������յ�����֪ͨ����Ҫ�ٴν����������ʧ�ܣ���ʾ /app/stream ����ʹ��
				WriteLog(Log_Debug, "������������Ϊ 0 ʱ��ֱ��ɾ��ý��Դ�ṩ�� nClient = %llu ", (*iterator1).second->nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->nClient, sizeof((*iterator1).second->nClient));
			}

			MessageNoticeStruct msgNotice;
			msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_disconnect;
			sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_disconnect\",\"app\":\"%s\",\"stream\":\"%s\",\"sourceType\":%d,\"sourceURL\":\"%s\",\"mediaServerId\":\"%s\",\"mediaSourceCount\":%d,\"networkType\":%d,\"key\":%llu,\"errorCause\":\"%s\",\"initiative\":%s}", (*iterator1).second->app, (*iterator1).second->stream, (*iterator1).second->nMediaSourceType, (*iterator1).second->sourceURL, ABL_MediaServerPort.mediaServerID, (*iterator1).second->mediaSourceCount - 1, (*iterator1).second->netBaseNetType, (*iterator1).second->nClient, (*iterator1).second->initiative ? "User actively disconnects" : "Front end network abnormal disconnection", (*iterator1).second->initiative ? "true":"false");
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

//�ѿͻ���ID����ý����Դ�Ƴ������ٿ��� 
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

//����ý���ṩ�ߵĿͻ���ID�����ҳ�ý��Դ 
CMediaStreamSource_ptr GetMediaStreamSourceByClientID(uint64_t nClient)
{
	std::lock_guard<std::mutex> lock(ABL_CMediaStreamSourceMapLock);
	CMediaStreamSource_ptrMap::iterator iterator1;
	CMediaStreamSource_ptr   pFindMediaSource = NULL;
 
	for (iterator1 = xh_ABLMediaStreamSourceMap.begin(); iterator1 != xh_ABLMediaStreamSourceMap.end(); ++iterator1)
	{
		pFindMediaSource = (*iterator1).second;
		if (pFindMediaSource->nClient == nClient)
		{
			return pFindMediaSource ;
 		}
	}
	return NULL ;
}

//ɾ��ý��Դ
int  CloseMediaStreamSource(closeStreamsStruct closeStruct)
{
 	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);
	int  nDeleteCount = 0;

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;
	bool               bDeleteFlag = false;
	char               szMediaSourceURL[string_length_1024] = {0} ;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
 		pClient = (*iterator1).second;
		bDeleteFlag = false;
		if (pClient->netBaseNetType == NetBaseNetType_RtspServerRecvPushVideo ||
			pClient->netBaseNetType == NetBaseNetType_RtspServerRecvPushAudio ||
			pClient->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream ||
			pClient->netBaseNetType == NetBaseNetType_WebSocektRecvAudio ||
			pClient->netBaseNetType == NetBaseNetType_RtmpServerRecvPush ||
			pClient->netBaseNetType == NetBaseNetType_RtspServerRecvPush ||
			pClient->netBaseNetType == NetBaseNetType_addStreamProxyControl ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Client ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RecvRtpPS_TS ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181UDPPSStreamInput ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181UDPTSStreamInput ||
			pClient->netBaseNetType == NetBaseNetType_NetServerReadMultRecordFile ||
			pClient->netBaseNetType == ReadRecordFileInput_ReadTSFile || 
			pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive ||
			pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen
 		 )
		{
			if ( strlen(closeStruct.app) > 0 && strlen(closeStruct.stream) > 0)
			{//ǿ�ƹر�
				if (strcmp(pClient->app, closeStruct.app) == 0 && strcmp(pClient->stream, closeStruct.stream) == 0)
				{
					nDeleteCount++;
					bDeleteFlag = true ;
				}
			}
			else if ( strlen(closeStruct.app) > 0 && strlen(closeStruct.stream) == 0)
			{//ǿ�ƹر�
				if (strcmp(pClient->app, closeStruct.app) == 0)
				{
					nDeleteCount++;
					bDeleteFlag = true;
				}
			}
			else if ( strlen(closeStruct.app) == 0 && strlen(closeStruct.stream) > 0)
			{//ǿ�ƹر�
				if (strcmp(pClient->stream, closeStruct.stream) == 0)
				{
					nDeleteCount++;
					bDeleteFlag = true;
				}
			}
			else if ( strlen(closeStruct.app) == 0 && strlen(closeStruct.stream) == 0 && !(pClient->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream || pClient->netBaseNetType == NetBaseNetType_NetGB28181RecvRtpPS_TS))
			{//ǿ�ƹر�
				nDeleteCount++;
				bDeleteFlag = true;
			}
		}

		if (bDeleteFlag)
		{
			sprintf(szMediaSourceURL, "/%s/%s", closeStruct.app, closeStruct.stream);
			CMediaStreamSource_ptr pMediaSource = GetMediaStreamSourceNoLock(szMediaSourceURL);
			if (pMediaSource)
			{
				pMediaSource->initiative = true;
 				WriteLog(Log_Debug, "CloseMediaStreamSource() netBaseNetType = %d , initiative = %d ", pClient->netBaseNetType, pMediaSource->initiative);
			}
 			WriteLog(Log_Debug, "close_streams() nClient = %llu \r\n", pClient->nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
		}
	}

	return  nDeleteCount;
}

//�Ȱѻ������Ƶ�������IDȫ��װ��FIFO����������һ��lock���ž��ⲿ���ҵ���http api ����������� 
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
			pClient->netBaseNetType == NetBaseNetType_NetGB28181UDPPSStreamInput || (pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP && strlen(pClient->m_startSendRtpStruct.recv_app) > 0 && strlen(pClient->m_startSendRtpStruct.recv_stream) > 0) || (pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect  && strlen(pClient->m_startSendRtpStruct.recv_app) > 0 && strlen(pClient->m_startSendRtpStruct.recv_stream) > 0) ||
			(pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive && strlen(pClient->m_startSendRtpStruct.recv_app) > 0 && strlen(pClient->m_startSendRtpStruct.recv_stream) > 0) || pClient->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream)
		{//����������rtsp,rtmp,flv,hls ��,rtsp������rtmp������gb28181��webrtc 
			pNetBaseObjectFifo.push((unsigned char*)&(*iterator1).second->nClient, sizeof((*iterator1).second->nClient));
		}
	}

	return pNetBaseObjectFifo.GetSize();
}

//��ȡý��Դ
int GetAllMediaStreamSource(char* szMediaSourceInfo, getMediaListStruct mediaListStruct)
{
	int              nMediaCount = 0;
	char             szTemp2[1024*48] = { 0 };
	char             szShareMediaURL[string_length_2048];
	bool             bAddFlag = false;
	uint64_t         nNoneReadDuration = 0;
	unsigned short   nClientPort;
	char             szApp[string_length_256] = { 0 };
	char             szStream[string_length_512] = { 0 };
	unsigned char*   pData;
	uint64_t         nClient;
	int              nLength;
	string           szRecordPathName;
	uint64_t         nGetMediaListStartTime = GetTickCount64();

	WriteLog(Log_Debug, "getMediaList() start .... getTickCount = %llu ", nGetMediaListStartTime );

	//��ȡ�������Ƶ�������ID
	GetAllNetBaseObjectToFifo();

	if (xh_ABLMediaStreamSourceMap.size() > 0)
 		strcpy(szMediaSourceInfo, "{\"code\":0,\"memo\":\"success\",\"mediaList\":[");
 
	while ((pData = pNetBaseObjectFifo.pop(&nLength)) != NULL)
	{
		memcpy((char*)&nClient, pData, sizeof(uint64_t));
		CNetRevcBase_ptr pClient = GetNetRevcBaseClient(nClient);

		if (pClient != NULL )
		{
			if(pClient->netBaseNetType != NetBaseNetType_NetGB28181RtpServerListen && pClient->netBaseNetType != NetBaseNetType_NetGB28181RtpSendListen)
			{//����listen�������
				if (pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
				{
					sprintf(szShareMediaURL, "/%s/%s", pClient->m_startSendRtpStruct.recv_app, pClient->m_startSendRtpStruct.recv_stream); //����ȫ˫��
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
						if (pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server)//����TCP���룬��Ҫ�滻Ϊ�󶨵Ķ˿� 
						{
							CNetRevcBase_ptr pParent = GetNetRevcBaseClient(pClient->hParent);
							if (pParent != NULL)
							{
								nClientPort = pParent->nClientPort;
								sprintf(pClient->m_addStreamProxyStruct.url, "rtp://%s:%d%s", pClient->szClientIP, nClientPort, szShareMediaURL);
							}
						}

						if (tmpMediaSource->nMediaSourceType == MediaSourceType_LiveMedia)
						{//ʵ������
							sprintf(szTemp2, "{\"key\":%llu,\"port\":%d,\"app\":\"%s\",\"stream\":\"%s\",\"sourceType\":%d,\"duration\":%llu,\"sim\":\"%s\",\"status\":%s,\"enable_hls\":%s,\"transcodingStatus\":%s,\"sourceURL\":\"%s\",\"networkType\":%d,\"readerCount\":%d,\"noneReaderDuration\":%llu,\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"videoBitrate\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"audioBitrate\":%d,\"url\":{\"rtsp\":\"%s://%s:%d/%s/%s\",\"rtmp\":\"%s://%s:%d/%s/%s\",\"http-flv\":\"%s://%s:%d/%s/%s.flv\",\"ws-flv\":\"%s://%s:%d/%s/%s.flv\",\"http-mp4\":\"%s://%s:%d/%s/%s.mp4\",\"http-hls\":\"%s://%s:%d/%s/%s.m3u8\",\"webrtc\":\"%s://%s:%d/rtc/v1/whep/?app=%s&stream=%s\"}},", tmpMediaSource->nClient, nClientPort, szApp, szStream, tmpMediaSource->nMediaSourceType, (GetTickCount64() - tmpMediaSource->nCreateDateTime) / 1000, tmpMediaSource->sim, tmpMediaSource->enable_mp4 == true ? "true" : "false", tmpMediaSource->enable_hls == true ? "true" : "false", tmpMediaSource->H265ConvertH264_enable == true ? "true" : "false", tmpMediaSource->sourceURL, pClient->netBaseNetType, tmpMediaSource->mediaSendMap.size(), nNoneReadDuration,
								tmpMediaSource->m_mediaCodecInfo.szVideoName, tmpMediaSource->m_mediaCodecInfo.nVideoFrameRate, tmpMediaSource->m_mediaCodecInfo.nWidth, tmpMediaSource->m_mediaCodecInfo.nHeight, tmpMediaSource->m_mediaCodecInfo.nVideoBitrate, tmpMediaSource->m_mediaCodecInfo.szAudioName, tmpMediaSource->m_mediaCodecInfo.nChannels, tmpMediaSource->m_mediaCodecInfo.nSampleRate, tmpMediaSource->m_mediaCodecInfo.nAudioBitrate,
								ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, szApp, szStream,
								ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, szApp, szStream,
								ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, szApp, szStream,
								ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, szApp, szStream,
								ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, szApp, szStream,
								ABL_MediaServerPort.nHlsPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, szApp, szStream,
								ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, szApp, szStream);
						}
						else
						{//¼��㲥
							szRecordPathName = pClient->m_addStreamProxyStruct.url;
	#ifdef OS_System_Windows
							

#ifdef USE_BOOST
							replace_all(szRecordPathName, "\\", "/");

#else
							ABL::replace_all(szRecordPathName, "\\", "/");

#endif
	#endif					 
							sprintf(szTemp2, "{\"key\":%llu,\"port\":%d,\"app\":\"%s\",\"stream\":\"%s\",\"sourceType\":%d,\"duration\":%llu,\"sim\":\"%s\",\"status\":%s,\"enable_hls\":%s,\"transcodingStatus\":%s,\"sourceURL\":\"%s\",\"networkType\":%d,\"readerCount\":%d,\"noneReaderDuration\":%llu,\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"videoBitrate\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"audioBitrate\":%d,\"url\":{\"rtsp\":\"%s://%s:%d/%s/%s\",\"rtmp\":\"%s://%s:%d/%s/%s\",\"http-flv\":\"%s://%s:%d/%s/%s.flv\",\"ws-flv\":\"%s://%s:%d/%s/%s.flv\",\"http-mp4\":\"%s://%s:%d/%s/%s.mp4\",\"http-hls\":\"%s://%s:%d/%s/%s.m3u8\",\"webrtc\":\"%s://%s:%d/rtc/v1/whep/?app=%s&stream=%s\"}},", tmpMediaSource->nClient, nClientPort, szApp, szStream, tmpMediaSource->nMediaSourceType, (GetTickCount64() - tmpMediaSource->nCreateDateTime) / 1000, tmpMediaSource->sim, tmpMediaSource->enable_mp4 == true ? "true" : "false", "false", tmpMediaSource->enable_mp4 == true ? "true" : "false", szRecordPathName.c_str(), pClient->netBaseNetType, tmpMediaSource->mediaSendMap.size(), nNoneReadDuration,
								tmpMediaSource->m_mediaCodecInfo.szVideoName, tmpMediaSource->m_mediaCodecInfo.nVideoFrameRate, tmpMediaSource->m_mediaCodecInfo.nWidth, tmpMediaSource->m_mediaCodecInfo.nHeight, tmpMediaSource->m_mediaCodecInfo.nVideoBitrate, tmpMediaSource->m_mediaCodecInfo.szAudioName, tmpMediaSource->m_mediaCodecInfo.nChannels, tmpMediaSource->m_mediaCodecInfo.nSampleRate, tmpMediaSource->m_mediaCodecInfo.nAudioBitrate,
								ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, szApp, szStream,
								ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, szApp, szStream,
								ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, szApp, szStream,
								ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, szApp, szStream,
								ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, szApp, szStream,
								ABL_MediaServerPort.nHlsPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, szApp, szStream,
								ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, szApp, szStream);
						}

						strcat(szMediaSourceInfo, szTemp2);

						nMediaCount++;
					}//if (strlen(tmpMediaSource->m_mediaCodecInfo.szVideoName) > 0 || strlen(tmpMediaSource->m_mediaCodecInfo.szAudioName) > 0)
				}//if (bAddFlag == true && tmpMediaSource != NULL)
			}//if(pClient->netBaseNetType != NetBaseNetType_NetGB28181RtpServerListen && pClient->netBaseNetType != NetBaseNetType_NetGB28181RtpSendListen)
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
	
	WriteLog(Log_Debug, "getMediaList() end .... getTickCount = %llu , ����ʱ�� = %llu ���� ", GetTickCount64(), GetTickCount64() - nGetMediaListStartTime );
	return nMediaCount;
}

//��ȡ������ռ�ö˿�
int GetALLListServerPort(char* szMediaSourceInfo, ListServerPortStruct  listServerPortStruct)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int   nMediaCount = 0;
	char  szTemp2[string_length_8192] = { 0 };
	uint64_t nClient = 0 ;

	if (xh_ABLNetRevcBaseMap.size() > 0)
	{
		strcpy(szMediaSourceInfo, "{\"code\":0,\"memo\":\"success\",\"data\":[");
	}

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		CNetRevcBase_ptr    pClient = (*iterator1).second;

		//WriteLog(Log_Debug,  "GetALLListServerPort netBaseNetType = %d, nClient = %llu , app = %s ,string = %s ,nClientPort = %d",pClient->netBaseNetType, pClient->nClient, pClient->m_addStreamProxyStruct.app,pClient->m_addStreamProxyStruct.stream,pClient->nClientPort);

		if (strlen(pClient->m_addStreamProxyStruct.app) > 0 && strlen(pClient->m_addStreamProxyStruct.stream) > 0 && pClient->nClientPort > 0 )
		{
			if(pClient->nClientPort >= 0 && !(pClient->netBaseNetType == NetBaseNetType_addStreamProxyControl || pClient->netBaseNetType == NetBaseNetType_addPushProxyControl || pClient->netBaseNetType == NetBaseNetType_NetServerHTTP ||
			   pClient->netBaseNetType == NetBaseNetType_RecordFile_FMP4 || pClient->netBaseNetType == NetBaseNetType_RecordFile_MP4 || pClient->netBaseNetType == RtspPlayerType_RecordReplay || pClient->netBaseNetType == NetBaseNetType_SnapPicture_JPEG ||
			  pClient->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream
				))
			{ 
				memset(szTemp2, 0x00, sizeof(szTemp2));

				if (pClient->netBaseNetType >= NetBaseNetType_RtspClientRecv && pClient->netBaseNetType <= NetBaseNetType_GB28181ClientPushUDP)
					nClient = pClient->hParent;//�д�����Ķ���Ҫ ���ظ���ID 
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
				else if (strlen(listServerPortStruct.app) == 0 && strlen(listServerPortStruct.stream) >  0 && strcmp(pClient->m_addStreamProxyStruct.stream, listServerPortStruct.stream) == 0)
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

//��ȡ�������ⷢ�͵���
int GetAllOutList(char* szMediaSourceInfo, char* szOutType)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;
	int   nMediaCount = 0;
	char  szTemp2[string_length_8192] = { 0 };
	uint64_t nClient = 0 ;
 
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
			pClient->netBaseNetType == NetBaseNetType_HttpMP4ServerSendPush || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
		{
			if (pClient->netBaseNetType >= NetBaseNetType_RtspClientRecv && pClient->netBaseNetType <= NetBaseNetType_GB28181ClientPushUDP)
				nClient = pClient->hParent;//�д�����Ķ���Ҫ ���ظ���ID 
			else
				nClient = pClient->nClient;

 			if (strlen(pClient->mediaCodecInfo.szVideoName) > 0 || strlen(pClient->mediaCodecInfo.szAudioName) > 0)
			{
				sprintf(szTemp2, "{\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s\",\"sourceURL\":\"%s\",\"videoCodec\":\"%s\",\"videoFrameSpeed\":%d,\"width\":%d,\"height\":%d,\"audioCodec\":\"%s\",\"audioChannels\":%d,\"audioSampleRate\":%d,\"networkType\":%d,\"duration\":%llu,\"dst_url\":\"%s\",\"dst_port\":%d},", nClient, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream, pClient->m_addStreamProxyStruct.url,
					pClient->mediaCodecInfo.szVideoName, pClient->mediaCodecInfo.nVideoFrameRate, pClient->mediaCodecInfo.nWidth, pClient->mediaCodecInfo.nHeight, pClient->mediaCodecInfo.szAudioName, pClient->mediaCodecInfo.nChannels, pClient->mediaCodecInfo.nSampleRate, pClient->netBaseNetType,(GetTickCount64()- pClient->nCreateDateTime)/1000, pClient->szClientIP,pClient->nClientPort);

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

//���app,stream �Ƿ�ռ�� 
bool CheckAppStreamExisting(char* szAppStreamURL)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;
	bool   bAppStreamExisting = false ;
	char   szTemp2[string_length_8192] = { 0 };

	if (xh_ABLNetRevcBaseMap.size() <= 0)
	{
		return false;
	}

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		pClient = (*iterator1).second;
		if (pClient != NULL && (pClient->netBaseNetType != NetBaseNetType_NetServerHTTP &&
			pClient->netBaseNetType != NetBaseNetType_RtspServerSendPush &&
			pClient->netBaseNetType != NetBaseNetType_RtmpServerSendPush &&
			pClient->netBaseNetType != NetBaseNetType_HttpFLVServerSendPush &&
			pClient->netBaseNetType != NetBaseNetType_WsFLVServerSendPush &&
			pClient->netBaseNetType != NetBaseNetType_HttpMP4ServerSendPush &&
			pClient->netBaseNetType != NetBaseNetType_HttpHLSServerSendPush && 
			pClient->netBaseNetType != NetBaseNetType_SnapPicture_JPEG
 			) 
		 )
		{
			sprintf(szTemp2, "/%s/%s", pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream);
			if (strcmp(szTemp2, szAppStreamURL) == 0)
			{
				bAppStreamExisting = true ;
				WriteLog(Log_Debug, "CheckAppStreamExisting(), nClient = %llu  ,netBaseNetType = %d,url = %s �Ѿ����� ,���ڽ��� !",pClient->nClient, pClient->netBaseNetType, szAppStreamURL);
				break;
			}
		}
	}
	if(!bAppStreamExisting)
	  WriteLog(Log_Debug, "CheckAppStreamExisting(), url = %s  ��δʹ�� !", szAppStreamURL);

	return bAppStreamExisting ;
}

/* ý�����ݴ洢 -------------------------------------------------------------------------------------*/

/* ¼���ļ��洢 -------------------------------------------------------------------------------------*/
//ɾ�����ڵ�m3u8�ļ�
int  DeleteExpireM3u8File()
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptrMap::iterator it;
	CRecordFileSource_ptr   pRecord = NULL;
	int                     nDeleteCount = 0;

	for (it = xh_ABLRecordFileSourceMap.begin() ;it != xh_ABLRecordFileSourceMap.end(); ++it)
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

CRecordFileSource_ptr CreateRecordFileSource(char* app,char* stream)
{
	char szShareURL[string_length_1024] = { 0 };
	sprintf(szShareURL, "/%s/%s", app, stream);
	CRecordFileSource_ptr pReordFile = GetRecordFileSource(szShareURL);
	if (pReordFile != NULL)
	{
		WriteLog(Log_Debug, "CreateRecordFileSource ʧ�� , app = %s ,stream = %s �Ѿ����� ", app, stream);
		return NULL;
	}

	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);

	CRecordFileSource_ptr pRecord = NULL;
	 
	try
	{
		do
		{
#ifdef USE_BOOST
			pRecord = std::make_shared<CRecordFileSource>(app, stream);
#else
			pRecord = std::make_shared<CRecordFileSource>(app, stream);
#endif
	
		} while (pRecord == NULL);
	}
	catch (const std::exception &e)
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

//����һ��¼���ļ���¼��ý��Դ
bool AddRecordFileToRecordSource(char* szShareURL,char* szFileName)
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
		return false ;
	}
}
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
std::string generate_uuid() {
	// Use high-resolution clock for better seeding
	auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::mt19937 gen(static_cast<uint32_t>(seed));
	std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

	std::stringstream ss;
	ss << std::hex << std::setfill('0');

	// Generate 32-bit random parts for UUID
	ss << std::setw(8) << dist(gen) << "-";
	ss << std::setw(4) << (dist(gen) >> 16) << "-";
	ss << std::setw(4) << ((dist(gen) & 0x0FFF) | 0x4000) << "-"; // 4xxx indicates version 4 UUID
	ss << std::setw(4) << ((dist(gen) & 0x3FFF) | 0x8000) << "-"; // 8xxx indicates variant 1 UUID
	ss << std::setw(4) << dist(gen) << std::setw(8) << dist(gen);

	return ss.str();
}

extern  const struct mov_buffer_t* mov_file_buffer(void);
//��ѯ¼��
int queryRecordListByTime(char* szMediaSourceInfo, queryRecordListStruct queryStruct,char* szOutRecordPlayURL)
{
	std::lock_guard<std::mutex> lock(ABL_CRecordFileSourceMapLock);
	WriteLog(Log_Debug, "��ѯ¼��ʼ app = %s , stream = %s , starttime  = %s ,endtime = %s ", queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);

	CRecordFileSource_ptrMap::iterator iterator1;
	CRecordFileSource_ptr   pRecord = NULL;
	list<uint64_t>::iterator it2;

	int   nMediaCount = 0;
	char  szTemp2[string_length_48K] = { 0 };
	char  szTemp1[string_length_2048] = { 0 };
	char  szShareMediaURL[string_length_2048] = { 0 };
	bool  bAddFlag = false;
 	char           szFileName[string_length_1024] = { 0 };
 	AVFormatContext *  pFormatCtx2=NULL;
	char           szFileNameUTF8[string_length_1024];
	uint64_t       duration = 0 ;//¼��ط�ʱ��ȡ��¼���ļ�����
	FILE*          fileM3U8=NULL;
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
	uint64_t       tEndTime;
	char           szDateTime[256] = { 0 };
	uint64_t       tDuration;
#ifdef USE_BOOST
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	string tmp_uuid = boost::uuids::to_string(a_uuid);
#else
	string tmp_uuid = generate_uuid();
#endif //USE_BOOST

	if (xh_ABLRecordFileSourceMap.size() > 0)
	{
		nTime1 = GetCurrentSecondByTime(queryStruct.starttime);
		nTime2 = GetCurrentSecondByTime(queryStruct.endtime);
		tDuration = nTime2 - nTime1;
		if (ABL_MediaServerPort.videoFileFormat == 3)
		{
#ifdef  OS_System_Windows
			sprintf(m3u8FileName, "%s%s\\%s\\%s_%s.m3u8", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
			sprintf(mapm3u8FileName, "\\%s\\%s\\%s_%s.m3u8", queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
#else
			sprintf(m3u8FileName, "%s%s/%s/%s_%s.m3u8", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
			sprintf(mapm3u8FileName, "/%s/%s/%s_%s.m3u8",  queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
#endif 
			fileM3U8 = fopen(m3u8FileName, "wb");
			sprintf(szTemp1, "\"http-hls\":\"%s://%s:%d/%s/%s%s%s_%s.m3u8\"", ABL_MediaServerPort.nHlsPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, queryStruct.starttime, queryStruct.endtime);
		}

		if (fileM3U8 != NULL)
		{//ts ��Ƭ
			sprintf(szRecordURL, "\"url\":{\"rtsp\": \"%s://%s:%d/%s/%s_%s_%s-%s\",\"rtmp\": \"%s://%s:%d/%s/%s_%s_%s-%s\",\"http-flv\": \"%s://%s:%d/%s/%s_%s_%s-%s.flv\",\"ws-flv\": \"%s://%s:%d/%s/%s_%s_%s-%s.flv\",\"http-mp4\": \"%s://%s:%d/%s/%s_%s_%s-%s.mp4\",\"download\": \"%s://%s:%d/%s/%s_%s_%s-%s.mp4?download_speed=%d\",\"webrtc\": \"%s://%s:%d/rtc/v1/whep/?app=%s&stream=%s_%s_%s-%s\",%s}",
				ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
				ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime, ABL_MediaServerPort.httpDownloadSpeed,
				ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
				szTemp1
			);
		}
		else
		{//��TS 
			if (ABL_MediaServerPort.nHlsEnable == 1)
			{//hls��Ƭ
				sprintf(szRecordURL, "\"url\":{\"rtsp\": \"%s://%s:%d/%s/%s_%s_%s-%s\",\"rtmp\": \"%s://%s:%d/%s/%s_%s_%s-%s\",\"http-flv\": \"%s://%s:%d/%s/%s_%s_%s-%s.flv\",\"ws-flv\": \"%s://%s:%d/%s/%s_%s_%s-%s.flv\",\"http-mp4\": \"%s://%s:%d/%s/%s_%s_%s-%s.mp4\",\"http-hls\": \"%s://%s:%d/%s/%s_%s_%s-%s.m3u8\",\"download\": \"%s://%s:%d/%s/%s_%s_%s-%s.mp4?download_speed=%d\",\"webrtc\": \"%s://%s:%d/rtc/v1/whep/?app=%s&stream=%s_%s_%s-%s\"}",
					ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nHlsPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHlsPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime, ABL_MediaServerPort.httpDownloadSpeed,
					ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime
				);
			}
			else
			{
				sprintf(szRecordURL, "\"url\":{\"rtsp\": \"%s://%s:%d/%s/%s_%s_%s-%s\",\"rtmp\": \"%s://%s:%d/%s/%s_%s_%s-%s\",\"http-flv\": \"%s://%s:%d/%s/%s_%s_%s-%s.flv\",\"ws-flv\": \"%s://%s:%d/%s/%s_%s_%s-%s.flv\",\"http-mp4\": \"%s://%s:%d/%s/%s_%s_%s-%s.mp4\",\"download\": \"%s://%s:%d/%s/%s_%s_%s-%s.mp4?download_speed=%d\",\"webrtc\": \"%s://%s:%d/rtc/v1/whep/?app=%s&stream=%s_%s_%s-%s\"}",
					ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime,
					ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime, ABL_MediaServerPort.httpDownloadSpeed,
					ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nWebRtcPort, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime	
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
			uint64_t  nTime3 = GetCurrentSecondByTime(queryStruct.endtime);
 			bFlag1 = false;
			bFlag2 = false;
			if (nFileOrder == 0)
			{
				if (nTime1 >= nTime2 || (nTime1 + ABL_MediaServerPort.fileSecond) >= nTime2) {

					if (nTime1 <= nTime3) {
						bFlag1 = true;//��һ�������������ļ� 
					}
					else if ((nTime1 + ABL_MediaServerPort.fileSecond) <= nTime3) {
						bFlag1 = true;//��һ�������������ļ� 
					}
				}			
			}
			if (nFileOrder >= 1 && *it2 <= atoll(queryStruct.endtime)) {
				bFlag2 = true;//��������������ļ�
			}

			//����������mp4�ļ� 
			if (bFlag1 || bFlag2)
			{
				memset(szTemp2, 0x00, sizeof(szTemp2));

				nFileOrder ++;

				if (fileM3U8 != NULL)
				{//����m3u8�ļ�
					if (nFileOrder == 1)
					{
						sprintf(szTemp2, "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%d\n#EXT-X-MEDIA-SEQUENCE:%llu\n#EXT-X-ALLOW-CACHE:NO\n", ABL_MediaServerPort.fileSecond, *it2);
						fwrite(szTemp2, 1, strlen(szTemp2), fileM3U8);
						memset(szTemp2, 0x00, sizeof(szTemp2));
					}
				    sprintf(szTemp1, "#EXTINF:%d.000,\n/%s/%s%s%llu.mp4\n", ABL_MediaServerPort.fileSecond, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter,*it2);
					fwrite(szTemp1, 1, strlen(szTemp1), fileM3U8);
					fflush(fileM3U8);
				}

#ifdef  OS_System_Windows
				sprintf(szFileName, "%s%s\\%s\\%llu.mp4", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, *it2);
#else
				sprintf(szFileName, "%s%s/%s/%llu.mp4", ABL_MediaServerPort.recordPath, queryStruct.app, queryStruct.stream, *it2);
#endif 
				if (ABL_MediaServerPort.enable_GetFileDuration != 0)
				{//��Ҫ��ȡ¼���ļ���ʵʱ�� 
#ifdef OS_System_Windows 
					GBK2UTF8(szFileName, szFileNameUTF8, sizeof(szFileNameUTF8));
#else
					GB2312ToUTF8(szFileName, strlen(szFileName), szFileNameUTF8, sizeof(szFileNameUTF8));
#endif
					if (avformat_open_input(&pFormatCtx2, szFileNameUTF8, NULL, NULL) >= 0)
					{//�õ���ʵ����
						if (avformat_find_stream_info(pFormatCtx2, NULL) >= 0)
 							duration = pFormatCtx2->duration / 1000;
						else 
							duration = ABL_MediaServerPort.fileSecond * 1000;

						avformat_close_input(&pFormatCtx2);
						pFormatCtx2 = NULL;
						if (duration > ABL_MediaServerPort.fileSecond * 1000 )
							duration = ABL_MediaServerPort.fileSecond * 1000;
					}
					else
						duration = ABL_MediaServerPort.fileSecond * 1000;

					memset(szFileNameUTF8, 0x00, sizeof(szFileNameUTF8));
				}
				else
					duration = ABL_MediaServerPort.fileSecond * 1000 ;

				if (nFileOrder == 1)
				{
					sprintf(szRecordPlayURL, "/%s/%s_%s_%s-%s", queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime);
					strcpy(szOutRecordPlayURL, szRecordPlayURL);

					//û�д���ý��Դ�ٴ���
 					if( (pMediaStreamPtr = GetMediaStreamSourceNoLock(szRecordPlayURL)) == NULL )
 						mutlRecordPlay = CreateNetRevcBaseClient(NetBaseNetType_NetServerReadMultRecordFile, *it2, 0, szFileName, 0, szRecordPlayURL,false);
					if (mutlRecordPlay != NULL)
					{
						memcpy((char*)&mutlRecordPlay->m_queryRecordListStruct, (char*)&queryStruct, sizeof(queryStruct));
						sprintf(szMediaSourceInfo, "{\"code\":0,\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s_%s_%s-%s\",\"starttime\":\"%s\",\"endtime\":\"%s\",\"duration\":%llu,%s,\"recordFileList\":[", mutlRecordPlay->nClient, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime, queryStruct.starttime, queryStruct.endtime, tDuration, szRecordURL);
					}
					else
					{
						if(pMediaStreamPtr != NULL )
						   sprintf(szMediaSourceInfo, "{\"code\":0,\"key\":%llu,\"app\":\"%s\",\"stream\":\"%s_%s_%s-%s\",\"starttime\":\"%s\",\"endtime\":\"%s\",%s,\"duration\":%llu,\"recordFileList\":[", pMediaStreamPtr->nClient, queryStruct.app, queryStruct.stream, tmp_uuid.c_str(), queryStruct.starttime, queryStruct.endtime, queryStruct.starttime, queryStruct.endtime, tDuration, szRecordURL);
					}
				}
				if (mutlRecordPlay != NULL)
					mutlRecordPlay->mutliRecordPlayNameList.push_back(szFileName);

				sprintf(szDateTime, "%llu", *it2);
				tEndTime = GetCurrentSecondByTime(szDateTime);
				tEndTime += duration / 1000 ;
  				sprintf(szTemp2, "{\"file\":\"%llu.mp4\",\"starttime\":\"%llu\",\"endtime\":\"%s\",\"duration\":%llu,\"url\":{\"rtsp\":\"%s://%s:%d/%s/%s%s%llu\",\"rtmp\":\"%s://%s:%d/%s/%s%s%llu\",\"http-flv\":\"%s://%s:%d/%s/%s%s%llu.flv\",\"ws-flv\":\"%s://%s:%d/%s/%s%s%llu.flv\",\"http-mp4\":\"%s://%s:%d/%s/%s%s%llu.mp4\",\"download\":\"%s://%s:%d/%s/%s%s%llu.mp4?download_speed=%d\"}},", *it2, nFileOrder == 1 ? atoll(queryStruct.starttime) :*it2 , GetDateTimeBySeconds(tEndTime), duration/1000,
					ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_szLocalIP, ABL_MediaServerPort.nRtspPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_szLocalIP, ABL_MediaServerPort.nRtmpPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss" : "ws", ABL_szLocalIP, ABL_MediaServerPort.nWSFlvPort, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2,
					ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https" : "http", ABL_szLocalIP, ABL_MediaServerPort.nHttpMp4Port, queryStruct.app, queryStruct.stream, RecordFileReplaySplitter, *it2, ABL_MediaServerPort.httpDownloadSpeed);

				strcat(szMediaSourceInfo, szTemp2);
				nMediaCount++;
 			}

			//�����mp4�ļ����ٷ������� ����Ҫ�жϲ�ѯ 
			if (*it2 > atoll(queryStruct.endtime))
			{//���һ���ļ��б�Ľ���ʱ���޸�Ϊ��ѯ¼��Ľ���ʱ�� 
				string strQueryRecordList = szMediaSourceInfo;
#ifdef USE_BOOST
				replace_all(strQueryRecordList, GetDateTimeBySeconds(tEndTime), queryStruct.endtime);

#else
				ABL::replace_all(strQueryRecordList, GetDateTimeBySeconds(tEndTime), queryStruct.endtime);

#endif
				
				strcpy(szMediaSourceInfo, strQueryRecordList.c_str());
				WriteLog(Log_Debug, "queryRecordListByTime() �����mp4�ļ����ٷ������� ����Ҫ�жϲ�ѯ *it2 = %llu , endtime = %s ", *it2, queryStruct.endtime);
				break;
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
		//�������һ��mp4�Ĳ���ʱ�� 
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

	WriteLog(Log_Debug, "��ѯ¼����� app = %s , stream = %s , starttime  = %s ,endtime = %s ", queryStruct.app, queryStruct.stream, queryStruct.starttime, queryStruct.endtime);
	return nMediaCount;
}

//��ѯһ��¼���ļ��Ƿ����
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

/* ¼���ļ��洢 -------------------------------------------------------------------------------------*/

/* ͼƬ�ļ��洢 -------------------------------------------------------------------------------------*/
CPictureFileSource_ptr GetPictureFileSource(char* szShareURL,bool bLock )
{
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
	CPictureFileSource_ptr pReordFile = GetPictureFileSource(szShareURL,true);
	if (pReordFile != NULL)
	{
		WriteLog(Log_Debug, "CreatePictureFileSource ʧ�� , app = %s ,stream = %s �Ѿ����� ", app, stream);
		return NULL;
	}

	std::lock_guard<std::mutex> lock(ABL_CPictureFileSourceMapLock);

	CPictureFileSource_ptr pPicture = NULL;

	try
	{
		do
		{
#ifdef USE_BOOST
			pPicture = std::make_shared<CPictureFileSource>(app, stream);
#else
			pPicture = std::make_shared<CPictureFileSource>(app, stream);
#endif
	
		} while (pPicture == NULL);
	}
	catch (const std::exception &e)
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

//����һ��¼���ļ���¼��ý��Դ
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

//��ѯ¼��
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
			if ( (*it2 / 100) >= atoll(queryStruct.starttime) && (*it2 / 100) <= atoll(queryStruct.endtime) )
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

//��ѯһ��ͼƬ�ļ��Ƿ����
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
/* ͼƬ�ļ��洢 -------------------------------------------------------------------------------------*/

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

CNetRevcBase_ptr CreateBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL)
{
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
				//else if (serverHandle == srvhandle_8192)
				//	pXHClient = std::make_shared<CNetServerSendWebRTC>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
 				else if (serverHandle == srvhandle_1078)
				{//jtt1078 ���˿ڽ��복���豸 
					pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
 					if (pXHClient != NULL)
					{
 						pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Server;
						pXHClient->hParent = serverHandle;
						pXHClient->m_openRtpServerStruct.RtpPayloadDataType[0] = 0x34;//1078  
						pXHClient->m_openRtpServerStruct.jtt1078_KeepOpenPortType[0] = 0X31;//ʵ����ʽ����
						pXHClient->m_openRtpServerStruct.disableVideo[0] = 0x30;
						pXHClient->m_openRtpServerStruct.disableAudio[0] = 0x30;
						sprintf(pXHClient->m_openRtpServerStruct.jtt1078_version, "%d", ABL_MediaServerPort.jtt1078Version);
 					}
 				}
				else if (serverHandle == srvhandle_10000)
				{//���굥�˿�����
					pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient != NULL)
					{
						CNetGB28181RtpServer* gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
						pXHClient->netBaseNetType = NetBaseNetType_GB28181TcpPSInputStream;
						pXHClient->hParent = CltHandle;
						gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength];
						gb28181TCP->m_addStreamProxyStruct.RtpPayloadDataType[0] = 0x31;//PS  
						gb28181TCP->m_addStreamProxyStruct.disableVideo[0] = 0x30;//û��������Ƶ
						gb28181TCP->m_addStreamProxyStruct.disableAudio[0] = 0x30;//û��������Ƶ
					}
				}
				else
				{
					CNetRevcBase_ptr gb28181Listen = GetNetRevcBaseClientNoLock(serverHandle);
					if (gb28181Listen && gb28181Listen->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen && atoi(gb28181Listen->m_openRtpServerStruct.jtt1078_KeepOpenPortType) == 0 &&  gb28181Listen->nMediaClient == 0)
					{//����TCP ������ʽ���� 
						CNetGB28181RtpServer* gb28181TCP = NULL;
						pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, gb28181Listen->m_szShareMediaURL);
						if (pXHClient != NULL)
						{
							pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Server;//����28181 tcp ��ʽ�������� 
							gb28181Listen->nMediaClient = CltHandle; //�Ѿ��������ӽ���

							gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
							if (gb28181TCP)
							{
								strcpy(gb28181TCP->szClientIP, szIP);
								gb28181TCP->nClientPort = nPort;
								gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength]; //��ʹ��ǰ��׼�����ڴ� 
							}

							pXHClient->hParent = gb28181Listen->nClient;//��¼�����������
							pXHClient->m_gbPayload = atoi(gb28181Listen->m_openRtpServerStruct.payload);//����paylad 
							memcpy((char*)&pXHClient->m_addStreamProxyStruct, (char*)&gb28181Listen->m_addStreamProxyStruct, sizeof(gb28181Listen->m_addStreamProxyStruct));
							memcpy((char*)&pXHClient->m_openRtpServerStruct, (char*)&gb28181Listen->m_openRtpServerStruct, sizeof(gb28181Listen->m_openRtpServerStruct));
							memcpy((char*)&pXHClient->m_h265ConvertH264Struct, (char*)&gb28181Listen->m_h265ConvertH264Struct, sizeof(gb28181Listen->m_h265ConvertH264Struct));//����ָ��ת�����
						}
					}
					else if (gb28181Listen && gb28181Listen->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen && atoi(gb28181Listen->m_openRtpServerStruct.jtt1078_KeepOpenPortType) == 0 &&  gb28181Listen->nMediaClient == 0)
					{//���� tcp ������ʽ ���� 
						pXHClient = std::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, gb28181Listen->m_szShareMediaURL);
						if (pXHClient != NULL)
						{
							pXHClient->hParent = gb28181Listen->nClient;//��¼listen��ID 
							pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpTCP_Passive;//����28181 tcp ������ʽ�������� 
							gb28181Listen->nMediaClient = CltHandle; //�Ѿ��������ӽ�����ֻ����һ�����ӽ��� 
							memcpy((char*)&pXHClient->m_startSendRtpStruct, (char*)&gb28181Listen->m_startSendRtpStruct, sizeof(pXHClient->m_startSendRtpStruct)); //��listen����� m_startSendRtpStruct ������CNetGB28181RtpClient����� m_startSendRtpStruct
						}
					}
					else if (gb28181Listen && gb28181Listen->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen && atoi(gb28181Listen->m_openRtpServerStruct.jtt1078_KeepOpenPortType) >= 1)
					{//jtt1078 �����˿ڽ��룬����������������� 
						pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
						CNetRevcBase_ptr gb28181Listen = GetNetRevcBaseClientNoLock(serverHandle);
						if (pXHClient != NULL && gb28181Listen)
						{
							pXHClient->hParent = serverHandle;
							pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Server;//ָ��ʵ������������
							pXHClient->nClient = CltHandle;
							memcpy((char*)&pXHClient->m_openRtpServerStruct, (char*)&gb28181Listen->m_openRtpServerStruct, sizeof(openRtpServerStruct));//��listen�Ĺ������������������������Ķ��� 
							memset(pXHClient->m_openRtpServerStruct.app, 0x00, sizeof(pXHClient->m_openRtpServerStruct.app));//��գ���/1078/sim_chan������  
							memset(pXHClient->m_openRtpServerStruct.stream_id, 0x00, sizeof(pXHClient->m_openRtpServerStruct.stream_id));//��գ���/1078/sim_chan������  
						}
					}
					else
						return NULL;
				}
			}
			else if (netClientType == NetRevcBaseClient_addStreamProxyControl || netClientType == NetRevcBaseClient_addFFmpegProxyControl)
			{//�����������ư������С�����ffmepg 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetClientAddStreamProxy>(netClientType, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->nClient = CltHandle;
			}
			else if (netClientType == NetRevcBaseClient_addPushProxyControl)
			{//������������ 
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetClientAddPushProxy>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->nClient = CltHandle;
			}
			else if (netClientType == NetRevcBaseClient_addStreamProxy)
			{//��������
				if (memcmp(szIP, "http://", 7) == 0 && strstr(szIP, ".m3u8") != NULL)
				{//hls ��ʱ��֧�� hls ���� 
					pXHClient = std::make_shared<CNetClientRecvHttpHLS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
				else if ((memcmp(szIP, "http://", 7) == 0 || memcmp(szIP, "https://", 8) == 0) && strstr(szIP, ".flv") != NULL)
				{//flv 
					pXHClient = std::make_shared<CNetClientRecvFLV>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
				else if (memcmp(szIP, "rtsp://", 7) == 0 || memcmp(szIP, "rtsps://", 8) == 0)
				{//rtsp 
					pXHClient = std::make_shared<CNetClientRecvRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
				else if (memcmp(szIP, "rtmp://", 7) == 0 || memcmp(szIP, "rtmps://", 8) == 0)
				{//rtmp
					pXHClient = std::make_shared<CNetClientRecvRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
				else if (strstr(szIP, ".mp4") != NULL || strstr(szIP, ".mov") != NULL || strstr(szIP, ".mkv") != NULL || strstr(szIP, ".ts") != NULL || strstr(szIP, ".ps") != NULL || strstr(szIP, ".flv") != NULL || strstr(szIP, ".264") != NULL || strstr(szIP, ".265") != NULL)
				{//�����ļ�
					CltHandle = XHNetSDK_GenerateIdentifier();
					pXHClient = std::make_shared<CNetClientReadLocalMediaFile>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient;
				}
				else
					return NULL;
			}
			else if (netClientType == NetRevcBaseClient_addFFmpegProxy)
			{//ffmpeg ��������
				if (strstr(szIP, "rtsp://") != NULL || strstr(szIP, "rtsps://") != NULL || strstr(szIP, "rtmp://") != NULL || strstr(szIP, "rtmps://") != NULL || strstr(szIP, "http://") != NULL || strstr(szIP, "https://") != NULL || strstr(szIP, ".mp4") != NULL || strstr(szIP, ".mov") != NULL || strstr(szIP, ".mkv") != NULL || strstr(szIP, ".ts") != NULL || strstr(szIP, ".ps") != NULL || strstr(szIP, ".flv") != NULL || strstr(szIP, ".264") != NULL || strstr(szIP, ".265") != NULL)
				{//�����ļ�
					CltHandle = XHNetSDK_GenerateIdentifier();
					pXHClient = std::make_shared<CNetClientFFmpegRecv>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient;
				}
			}
			else if (netClientType == NetRevcBaseClient_addPushStreamProxy)
			{//��������
				if (memcmp(szIP, "rtsp://", 7) == 0 || memcmp(szIP, "rtsps://", 8) == 0)
				{//hls ��ʱ��֧�� hls ���� 
					pXHClient = std::make_shared<CNetClientSendRtsp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
				else if (memcmp(szIP, "rtmp://", 7) == 0 || memcmp(szIP, "rtmps://", 8) == 0)
				{
					pXHClient = std::make_shared<CNetClientSendRtmp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
					if (pXHClient)
						CltHandle = pXHClient->nClient; //��nClient��ֵ�� CltHandle ,��Ϊ�ؼ��� ���������ʧ�ܣ����յ��ص�֪ͨ���ڻص�֪ͨ����ɾ������ 
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerUDP)
			{//����GB28181 ��udp����
				pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerUDP;
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerTCP_Active)
			{//����GB28181 ��TCP �������ӷ�ʽ 
				pXHClient = std::make_shared<CNetGB28181RtpServer>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient != NULL)
				{
					CNetGB28181RtpServer* gb28181TCP = (CNetGB28181RtpServer*)pXHClient.get();
					gb28181TCP->netDataCache = new unsigned char[MaxNetDataCacheBufferLength]; //��ʹ��ǰ��׼�����ڴ� 
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerTCP_Active;
					pXHClient->hParent = CltHandle;
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181SendRtpUDP)
			{//����GB28181 ��udp����
				pXHClient = std::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpUDP;
			}
			else if (netClientType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
			{//����GB28181 ��tcp���� 
				pXHClient = std::make_shared<CNetGB28181RtpClient>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181SendRtpTCP_Connect;
			}
			else if (netClientType == NetBaseNetType_RecordFile_FMP4)
			{//fmp4¼��
				pXHClient = std::make_shared<CStreamRecordFMP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_RecordFile_FMP4;
			}
			else if (netClientType == NetBaseNetType_RecordFile_MP4)
			{//mp4¼��
				pXHClient = std::make_shared<CStreamRecordMP4>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = NetBaseNetType_RecordFile_MP4;
			}
			else if (netClientType == NetBaseNetType_RecordFile_TS)
			{//ts¼��
				pXHClient = std::make_shared<CStreamRecordTS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == ReadRecordFileInput_ReadFMP4File)
			{//��ȡfmp4�ļ�
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CReadRecordFileInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					pXHClient->netBaseNetType = ReadRecordFileInput_ReadFMP4File;
			}
			else if (netClientType == NetBaseNetType_SnapPicture_JPEG)
			{//ץ��ͼƬ
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetClientSnap>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
			else if (netClientType == NetBaseNetType_HttpClient_None_reader)
			{//�¼�֪ͨ1
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNoneReader = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������  
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Not_found)
			{//�¼�֪ͨ2
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNotFound = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Record_mp4)
			{//�¼�֪ͨ3
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordMp4 = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_arrive)
			{//�¼�֪ͨ4
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_not_arrive)
			{//�¼�֪ͨ5
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientNotArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_stream_disconnect)
			{//�¼�֪ͨ6
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientDisconnect = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_record_ts)
			{//�¼�֪ͨ7
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordTS = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_Record_Progress)
			{//�¼�֪ͨ8
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientRecordProgress = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_ServerStarted)
			{//�¼�֪ͨ9
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nServerStarted = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_ServerKeepalive)
			{//�¼�֪ͨ10
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nServerKeepalive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_DeleteRecordMp4)
			{//�¼�֪ͨ11
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nClientDeleteRecordMp4 = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_play)
			{//�¼�֪ͨ12
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nPlay = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_publish)
			{//�¼�֪ͨ13
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nPublish = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_iframe_arrive)
			{//�¼�֪ͨ14
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nFrameArrive = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_HttpClient_on_rtsp_replay)
			{//�¼�֪ͨ15
				pXHClient = std::make_shared<CNetClientHttp>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = ABL_MediaServerPort.nRtspReplay = pXHClient->nClient;
					pXHClient->netBaseNetType = netClientType; //������������
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RecvRtpPS_TS)
			{//���˿ڽ��չ��� 
				pXHClient = std::make_shared<CNetServerRecvRtpTS_PS>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181UDPTSStreamInput)
			{//TS ����γ�ý��Դ
				pXHClient = std::make_shared<CRtpTSStreamInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181UDPPSStreamInput)
			{//PS ����γ�ý��Դ
				pXHClient = std::make_shared<CRtpPSStreamInput>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
					CltHandle = pXHClient->nClient;
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpServerListen)
			{//����TCP�������յ�Listen 
				pXHClient = std::make_shared<CNetGB28181Listen>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpServerListen;
				}
			}
			else if (netClientType == NetBaseNetType_NetGB28181RtpSendListen)
			{//����TCP�������͵�Listen
				pXHClient = std::make_shared<CNetGB28181Listen>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_NetGB28181RtpSendListen;
				}
			}
			else if (netClientType == NetBaseNetType_RtspServerRecvPushVideo)
			{//����rtsp����udp��ʽ��Ƶ����
				pXHClient = std::make_shared<CNetRtspServerUDP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_RtspServerRecvPushVideo;
				}
			}
			else if (netClientType == NetBaseNetType_RtspServerRecvPushAudio)
			{//����rtsp����udp��ʽ��Ƶ����
				pXHClient = std::make_shared<CNetRtspServerUDP>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
				if (pXHClient)
				{
					CltHandle = pXHClient->nClient;
					pXHClient->netBaseNetType = NetBaseNetType_RtspServerRecvPushAudio;
				}
			}
			else if (netClientType == NetBaseNetType_NetServerReadMultRecordFile)
			{//������ȡ���¼���ļ�, serverHandle Ϊmp4¼���ļ�����
				CltHandle = XHNetSDK_GenerateIdentifier();
				pXHClient = std::make_shared<CNetServerReadMultRecordFile>(serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
			}
		} while (pXHClient == NULL);
	}
	catch (const std::exception &e)
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

CNetRevcBase_ptr CreateNetRevcBaseClient(int netClientType,NETHANDLE serverHandle, NETHANDLE CltHandle,char* szIP,unsigned short nPort,char* szShareMediaURL, bool bLock )
{
	if (bLock)
	{//��������������Χ���� 
		std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);
		return CreateBaseClient(netClientType, serverHandle, CltHandle, szIP,  nPort, szShareMediaURL);
	}else 
		return CreateBaseClient(netClientType, serverHandle, CltHandle, szIP, nPort, szShareMediaURL);
}

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

	CNetRevcBase_ptrMap::iterator iterator1;

	iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
 	    if((*iterator1).second->netBaseNetType == NetBaseNetType_WebRtcServerWhepPlayer)
		{//webrtc��http���ӻ��Լ������Ͽ�������ֱ������Ϊ false ,�����Ͽ����������Ϊfalse 
	/*		if((*iterator1).second->nWebRTC_Comm_State == WebRTC_Comm_State_Delete)
				(*iterator1).second->bRunFlag.exchange(false);*/
		}
  		else 
			(*iterator1).second->bRunFlag.exchange(false);
	 	
 		if ( ((*iterator1).second->netBaseNetType == NetBaseNetType_RtspClientPush || (*iterator1).second->netBaseNetType ==  NetBaseNetType_RtmpClientPush ||
			(*iterator1).second->netBaseNetType == NetBaseNetType_RtspClientRecv || (*iterator1).second->netBaseNetType == NetBaseNetType_RtmpClientRecv || (*iterator1).second->netBaseNetType == NetBaseNetType_FFmpegRecvNetworkMedia)
			&& (*iterator1).second->bProxySuccessFlag == false)
		{//rtsp\rtmp ����������rtsp \ rtmp ��������
			//���û�гɹ�������Ҫɾ������ 
			auto  pParentPtr = GetNetRevcBaseClientNoLock((*iterator1).second->hParent);
			if (pParentPtr && pParentPtr->bProxySuccessFlag == false || (*iterator1).second->m_nXHRtspURLType == XHRtspURLType_RecordPlay)
 			   pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->hParent, sizeof((*iterator1).second->hParent));
		}

		//�رչ������ 
		if ((*iterator1).second->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen)
		{
			if ((*iterator1).second->nMediaClient == 0)
			{//����û�дﵽ֪ͨ
				if (ABL_MediaServerPort.hook_enable == 1 && (*iterator1).second->bUpdateVideoFrameSpeedFlag == false)
				{
					MessageNoticeStruct msgNotice;
					msgNotice.nClient = NetBaseNetType_HttpClient_on_stream_not_arrive;
					sprintf(msgNotice.szMsg, "{\"eventName\":\"on_stream_not_arrive\",\"mediaServerId\":\"%s\",\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"key\":%llu}", ABL_MediaServerPort.mediaServerID, (*iterator1).second->m_addStreamProxyStruct.app, (*iterator1).second->m_addStreamProxyStruct.stream, (*iterator1).second->netBaseNetType, (*iterator1).second->nClient);
					pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
				}
			}
			if ((*iterator1).second->nMediaClient > 0)
				pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->nMediaClient, sizeof((*iterator1).second->nMediaClient));
		}
		else if ((*iterator1).second->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen)
		{
			WriteLog(Log_Debug, " XHNetSDK_Unlisten() , nMediaClient = %llu  ", (*iterator1).second->nClient);
		}

		//�������ĸ���ɾ����
		if ((*iterator1).second->hParent > 0)
		{
			auto  pParentPtr = GetNetRevcBaseClientNoLock((*iterator1).second->hParent);
			
			if (pParentPtr != NULL)
			{//���Ǵ������������������ľͿ���ɾ������ ����Ҳ���ǹ���ĳ����˿� ,�������������������Ĳ���ɾ�� ����Ҫ���������ﵽ�����ļ������õ����� 
 				if (!(pParentPtr->netBaseNetType == NetBaseNetType_addStreamProxyControl || pParentPtr->netBaseNetType == NetBaseNetType_addPushProxyControl) && atoi(pParentPtr->m_openRtpServerStruct.jtt1078_KeepOpenPortType) == 0 )
					pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->hParent, sizeof((*iterator1).second->hParent));
			}
		}

		if((*iterator1).second->netBaseNetType == NetBaseNetType_WebRtcServerWhepPlayer)
		{//webrtc��http���ӻ��Լ��Ͽ�������ֱ��ɾ�� 
	/*		if((*iterator1).second->nWebRTC_Comm_State == WebRTC_Comm_State_Delete)
				xh_ABLNetRevcBaseMap.erase(iterator1);*/
		}
  		else 
			xh_ABLNetRevcBaseMap.erase(iterator1);
		
  		return true;
	}
	else
	{
		return false;
	}
}

/*
 ���ܣ�
    ���˿��Ƿ��Ѿ�ʹ�� 
������
  int   nPort,      �˿�
  int   nPortType,  ����  1 openRtpServe , 2 sartSendRtp 
  bool  bLockFlag   �Ƿ���ס 
*/
bool  CheckPortAlreadyUsed(int nPort,int nPortType, bool bLockFlag)
{
 	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	bool                 bRet = false;
	CNetRevcBase_ptr     pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (nPortType == 1)
		{
			if ((pClient->netBaseNetType == NetRevcBaseClient__NetGB28181Proxy   && atoi(pClient->m_openRtpServerStruct.port) == nPort) ||
				(pClient->netBaseNetType ==  NetBaseNetType_NetGB28181RtpServerListen && atoi(pClient->m_openRtpServerStruct.port) == nPort)
				)
			{//�Ѿ�ռ���� nPort;
				bRet = true;
				break;
			}
		}
		else if (nPortType == 2)
		{
			if (( pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect) &&
				atoi(pClient->m_startSendRtpStruct.src_port) == nPort
				)
			{//�Ѿ�ռ���� nPort;
				bRet = true;
				break;
			}
		}
	}
	WriteLog(Log_Debug, "CheckPortAlreadyUsed() bRet = %d  ", bRet);
	return bRet;
}

/*
���ܣ�
   ���SSRC�Ƿ��Ѿ�ʹ��
������
	int   nSSRC,      ssrc
	bool  bLockFlag   �Ƿ���ס
*/
bool  CheckSSRCAlreadyUsed(int nSSRC, bool bLockFlag)
{
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
		{//�Ѿ�ռ���� nPort;
			bRet = true;
			break;
		}
 	}
	WriteLog(Log_Debug, "CheckSSRCAlreadyUsed() bRet = %d  ", bRet);
	return bRet;
}

/*
���ܣ�
��� dst_url �� dst_port �Ƿ��Ѿ�ʹ��
������
char  dst_url,    Ŀ��IP
int   dst_port    Ŀ��˿�
bool  bLockFlag   �Ƿ���ס
*/
bool  CheckDst_url_portAlreadyUsed(char* dst_url,int dst_port, bool bLockFlag)
{
 	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	bool                 bRet = false;
	CNetRevcBase_ptr     pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if ((pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect) &&
 			atoi(pClient->m_startSendRtpStruct.dst_port) == dst_port && strcmp(pClient->m_startSendRtpStruct.dst_url,dst_url) == 0 
			)
		{//�Ѿ�ռ���� nPort;
			bRet = true;
			break;
		}
	}
	WriteLog(Log_Debug, "CheckDst_url_portAlreadyUsed() bRet = %d  ", bRet);
	return bRet;
}

//����ĳһ���������͵Ķ�������
int  GetNetRevcBaseClientCountByNetType(NetBaseNetType netType,bool bLockFlag)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                nCount = 0 ;
	CNetRevcBase_ptr   pClient = NULL;

 	for(iterator1 = xh_ABLNetRevcBaseMap.begin();iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (pClient->netBaseNetType == netType && pClient->bSnapSuccessFlag == false )
		{//��ץ�Ķ��󣬲�����δץ�ĳɹ�
			nCount ++;
 		}
	}
	WriteLog(Log_Debug, "GetNetRevcBaseClientCountByNetType() netType = %d , nCount = %d  ", netType,nCount );
	return nCount;
}

//�����ж���װ������׼��ɾ�� 
int  FillNetRevcBaseClientFifo()
{
 	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                nCount = 0;
	CNetRevcBase_ptr   pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		//(*iterator1).second->nWebRTC_Comm_State = WebRTC_Comm_State_Delete ;//ϵͳ�˳�ʱ��Ҫ����webrtc����Ϊɾ��״̬ 
		pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
	}
 	return nCount;
}

//����ShareMediaURL��NetBaseNetType ���Ҷ��� 
CNetRevcBase_ptr  GetNetRevcBaseClientByNetTypeShareMediaURL(NetBaseNetType netType,char* ShareMediaURL, bool bLockFlag)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (pClient->netBaseNetType == netType && strcmp(ShareMediaURL,pClient->m_szShareMediaURL) == 0)
		{
	        WriteLog(Log_Debug, "GetNetRevcBaseClientByNetTypeShareMediaURL() netType = %d , nClient = %llu ", netType, pClient->nClient);
			return pClient;
		}
	}
	return  NULL ;
}

//��������url�Ƿ����
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
				WriteLog(Log_Debug, "QueryMediaSource() ������ַ�Ѿ����� url = %s ", pushURL);
				break;
			}
		}
	}
	return bFind;
}

//���������� ������M3u8���� 
int  CheckNetRevcBaseClientDisconnect()
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	int                           nDiconnectCount = 0;
	int                           nPrintCount = 0;
	char                          szLine[string_length_4096] = { 0 };
	char                          szTemp[string_length_512] = { 0 };
	int                           netBaseNetType;

	ABL_nPrintCheckNetRevcBaseClientDisconnect ++;
	if(ABL_nPrintCheckNetRevcBaseClientDisconnect % 20 == 0)//1���Ӵ�ӡһ��
	  WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() ��ǰ�������������� nSize = %llu ", xh_ABLNetRevcBaseMap.size());

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); ++iterator1)
	{
		netBaseNetType = ((*iterator1).second)->netBaseNetType;
		if (ABL_nPrintCheckNetRevcBaseClientDisconnect % 30 == 0)//1���Ӵ�ӡһ��
		{
			nPrintCount ++;
			sprintf(szTemp, "[ nClient = %llu netType = %d ] , ", ((*iterator1).second)->nClient, ((*iterator1).second)->netBaseNetType);
			strcat(szLine, szTemp);
			if (nPrintCount >= 4)
			{
				WriteLog(Log_Debug, "��ǰ������Ϣ %s ", szLine);
				memset(szLine, 0x00, sizeof(szLine));
				nPrintCount = 0;
			}
		}

		//����¼����ӣ��������hook_enable = 0 ����ɾ��
		if(ABL_MediaServerPort.hook_enable == 0 && ((((*iterator1).second)->netBaseNetType >= NetBaseNetType_HttpClient_None_reader && ((*iterator1).second)->netBaseNetType <= NetBaseNetType_HttpClient_Record_Progress ) ||
		    (((*iterator1).second)->netBaseNetType >= NetBaseNetType_HttpClient_ServerStarted && ((*iterator1).second)->netBaseNetType <= NetBaseNetType_HttpClient_on_publish ) ) )
		  pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));

 		if (
			(((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerRecvPush && ((*iterator1).second)->m_RtspNetworkType == RtspNetworkType_TCP) ||   //����rtsp��������(tcp����ʽ
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerRecvPushVideo || //rtsp ������Ƶ
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerRecvPushAudio || //rtsp ������Ƶ
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerSendPush ||   //rtsp ����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpServerRecvPush ||   //����RTMP����

			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientPush ||   //rtsp ����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientPush ||   //rtmp ����
 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv ||      //�������Rtsp����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientRecv ||      //�������Rtmp����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFlvClientRecv ||   //�������HttpFlv����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv ||    //�������HttpHLS����

			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP ||   //GB28181 ��UDP��ʽ���� 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Server || //GB28181 ��TCP��ʽ���� 

			((*iterator1).second)->netBaseNetType ==  NetBaseNetType_NetGB28181SendRtpUDP || //����UDP����
			((*iterator1).second)->netBaseNetType ==  NetBaseNetType_NetGB28181SendRtpTCP_Connect ||//����TCP���� 
 			
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFLVServerSendPush ||//���http-flv���� 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_WsFLVServerSendPush ||//���ws-flv���� 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpMP4ServerSendPush || //���MP4���� 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpServerSendPush ||  //���rtmp���� 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181UDPTSStreamInput || // ���˿� TS ������
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181UDPPSStreamInput || // ���˿� PS ������
			((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active || //����TCP��ʽ���� tcp�������ӷ�ʽ 
			((*iterator1).second)->netBaseNetType == NetBaseNetType_SnapPicture_JPEG || //ץ�Ķ���ʱ���
			((*iterator1).second)->netBaseNetType == NetBaseNetType_WebSocektRecvAudio || //websocketЭ�����pcm��Ƶ��
			((*iterator1).second)->netBaseNetType == NetBaseNetType_GB28181TcpPSInputStream || //ͨ��10000�˿�TCP��ʽ���չ���PS������
			((*iterator1).second)->netBaseNetType == NetBaseNetType_FFmpegRecvNetworkMedia || //ͨ������ffmpeg��ȡ rtmp,flv,mp4,hls ���� ����
			((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspProtectBaseState || // rtsp Э���ʼ״̬
			((*iterator1).second)->netBaseNetType == NetBaseNetType_WebRtcServerWhepPlayer //WebRTC ���� 
		)
		{//���ڼ�� HLS ������� �����������ӱ�����ͼ�� 
			if (((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv)
			{//Hls ��������
				((*iterator1).second)->RequestM3u8File();
			}

	       if (((*iterator1).second)->m_bPauseFlag == false && ((*iterator1).second)->nRecvDataTimerBySecond >= (ABL_MediaServerPort.MaxDiconnectTimeoutSecond / 2 ) )
 	       {//���ǹ���ط���ͣ��Ҳ����rtsp�ط���ͣ
			   nDiconnectCount ++;
			   ((*iterator1).second)->bRunFlag = false;
			   WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() nClient = %llu ��⵽�����쳣�Ͽ�1 ", ((*iterator1).second)->nClient );

			   //�����WebRTC���ţ���Ҫ�������� WebRTC ������� ������ɾ������WebRTC���Ŷ���
		/*	   if(((*iterator1).second)->netBaseNetType == NetBaseNetType_WebRtcServerWhepPlayer)
				 ((*iterator1).second)->nWebRTC_Comm_State = WebRTC_Comm_State_Delete ;   */
			   
			   pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));

			   if(((*iterator1).second)->m_nXHRtspURLType == XHRtspURLType_RecordPlay)
				   pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->hParent, sizeof((unsigned char*)&((*iterator1).second)->hParent));
		   }
		   //����rtcp��
		   if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv)
		   {
		       CNetClientRecvRtsp* pRtspClient = (CNetClientRecvRtsp*) (*iterator1).second.get();
			   if (pRtspClient->bSendRRReportFlag)
			   {
			       if (GetTickCount64() - pRtspClient->nCurrentTime >= 1000*3)
			       {
					   pRtspClient->SendRtcpReportData();
			       }
		      }

			  //����options ������ 
			  if(atoi(pRtspClient->m_addStreamProxyStruct.optionsHeartbeat) == 1)
			     pRtspClient->SendOptionsHeartbeat();
		   }

		   //���ڸ��¶�̬������IP
		   if (((*iterator1).second)->tUpdateIPTime - GetTickCount64() >= 1000 * 15)
		   {
			   ((*iterator1).second)->tUpdateIPTime = GetTickCount64();
			   ((*iterator1).second)->ConvertDemainToIPAddress();
		   }

		   //���ټ��¼���������
		   if (((*iterator1).second)->nRecvDataTimerBySecond >= 10 && ((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspServerSendPush && ((*iterator1).second)->m_bPauseFlag == false  && ((*iterator1).second)->nReplayClient > 0)
		   {
			   char szQuitText[128] = { 0 };
			   strcpy(szQuitText, "ABL_ANNOUNCE_QUIT:2021");
			   sprintf(((*iterator1).second)->szReponseTemp, "ANNOUNCE RTSP/1.0\r\nCSeq: %d\r\nUser-Agent: %s\r\nContent-Type: text/parameters\r\nContent-Length: %d\r\n\r\n%s", 8, MediaServerVerson,strlen(szQuitText),szQuitText);
			   WriteLog(Log_Debug, "CheckNetRevcBaseClientDisconnect() nClient = %llu ¼�������", ((*iterator1).second)->nClient);
			   XHNetSDK_Write(((*iterator1).second)->nClient,(unsigned char*)((*iterator1).second)->szReponseTemp, strlen(((*iterator1).second)->szReponseTemp),ABL_MediaServerPort.nSyncWritePacket);

			   pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
		   }

		   if (((*iterator1).second)->m_bPauseFlag == false)
			   ((*iterator1).second)->nRecvDataTimerBySecond ++;  //������ͣ����ʱ
		   else
			   ((*iterator1).second)->nRecvDataTimerBySecond = 0;//�Ѿ���ͣ�����ټ�ʱ
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen && ((*iterator1).second)->m_openRtpServerStruct.jtt1078_KeepOpenPortType[0] == 0x30 && ((*iterator1).second)->bUpdateVideoFrameSpeedFlag == false )
		{//������������� TCP 
			if ((GetTickCount64() - ((*iterator1).second)->nCreateDateTime) >= (1000 * (ABL_MediaServerPort.MaxDiconnectTimeoutSecond / 2)))
			{//�ڳ�ʱ��ʱ�䷶Χ�ڣ�������δ���� 
				WriteLog(Log_Debug, "����TCP���ճ�ʱ nClient = %llu , app = %s ,stream = %s , port = %s ", ((*iterator1).second)->nClient, ((*iterator1).second)->m_openRtpServerStruct.app, ((*iterator1).second)->m_openRtpServerStruct.stream_id, ((*iterator1).second)->m_openRtpServerStruct.port);
				pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
			}
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen && ((*iterator1).second)->nMediaClient == 0)
		{//������tcp������ʽ����
			if ((GetTickCount64() - ((*iterator1).second)->nCreateDateTime) >= (1000 * (ABL_MediaServerPort.MaxDiconnectTimeoutSecond / 2)))
			{//�ڳ�ʱ��ʱ�䷶Χ��,û�����ӽ���
				WriteLog(Log_Debug, "����gb28181 tcp ������ʽ������ʱ nClient = %llu , app = %s ,stream = %s , port = %s ", ((*iterator1).second)->nClient, ((*iterator1).second)->m_startSendRtpStruct.app, ((*iterator1).second)->m_startSendRtpStruct.stream, ((*iterator1).second)->m_startSendRtpStruct.src_port);
				pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof((unsigned char*)&((*iterator1).second)->nClient));
			}
		}
		else if (((*iterator1).second)->netBaseNetType == NetBaseNetType_addStreamProxyControl || ((*iterator1).second)->netBaseNetType == NetBaseNetType_addPushProxyControl)
		{//���ƴ�����������������,�����������Ƿ��ж���
			CNetRevcBase_ptr pClient = GetNetRevcBaseClientNoLock(((*iterator1).second)->nMediaClient);
			if (pClient == NULL)
			{//�Ѿ����ߣ���Ҫ�������� 
				if (((*iterator1).second)->bRecordProxyDisconnectTimeFlag == false)
				{
				  ((*iterator1).second)->nProxyDisconnectTime = GetTickCount64();
				  ((*iterator1).second)->bRecordProxyDisconnectTimeFlag = true;
				}

				if (GetTickCount64() - ((*iterator1).second)->nProxyDisconnectTime >= 1000 * 15)
				{
 					((*iterator1).second)->bRecordProxyDisconnectTimeFlag = false;

					((*iterator1).second)->nReConnectingCount ++; //���������ۻ� 

					if (((*iterator1).second)->nReConnectingCount > ABL_MediaServerPort.nReConnectingCount)
					{
						WriteLog(Log_Debug, "nClient = %llu , nMediaClient = %llu ,url: %s ���������Ѿ��ﵽ %llu �Σ���Ҫ�Ͽ� ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReConnectingCount);
						pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
					}
					else
					{
						sprintf(((*iterator1).second)->szResponseBody, "{\"code\":%d,\"memo\":\"Network Connnect[ %s ] Timeout .\",\"key\":%llu}", IndexApiCode_ConnectTimeout, ((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nClient);
						((*iterator1).second)->ResponseHttp(((*iterator1).second)->nClient_http, ((*iterator1).second)->szResponseBody, false);

 						//�����δ�ɹ�����ɾ�����������
						if (((*iterator1).second)->bProxySuccessFlag == true && ((*iterator1).second)->m_nXHRtspURLType != XHRtspURLType_RecordPlay)
						{
				            WriteLog(Log_Debug, "nClient = %llu , nMediaClient = %llu ��⵽�����쳣�Ͽ�2 , %s ������ִ�е� %llu ������  ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient,((*iterator1).second)->m_addStreamProxyStruct.url, ((*iterator1).second)->nReConnectingCount);
							pReConnectStreamProxyFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
						}
						else
						    pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
 					}
 				}
			}
			else
			{
				//����ǳ�ʱ�Ͽ��ģ���������ԭ������ɹ����ģ���Ҫ���޴����� 
				if (((*iterator1).second)->bProxySuccessFlag == false && pClient->bUpdateVideoFrameSpeedFlag == true )
					((*iterator1).second)->bProxySuccessFlag = true;

				//����ɹ�������������λ 
				if ( ((*iterator1).second)->nReConnectingCount != 0 && pClient->bUpdateVideoFrameSpeedFlag == true )
				{
					((*iterator1).second)->nReConnectingCount = 0;
					WriteLog(Log_Debug, "nClient = %llu , nMediaClient = %llu ,url %s ������������λΪ 0 ", ((*iterator1).second)->nClient, ((*iterator1).second)->nMediaClient, ((*iterator1).second)->m_addStreamProxyStruct.url);
				}
 			}
 		}

		//����������ִ�������������ʱ�����ӳ�ʱ�ظ�http����
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspClientRecv || ((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientRecv || ((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpFlvClientRecv || ((*iterator1).second)->netBaseNetType == NetBaseNetType_HttpHLSClientRecv ||
			((*iterator1).second)->netBaseNetType ==  NetBaseNetType_RtspClientPush || ((*iterator1).second)->netBaseNetType == NetBaseNetType_RtmpClientPush
 			)
		{
			if (!((*iterator1).second)->bResponseHttpFlag && GetTickCount64() - ((*iterator1).second)->nCreateDateTime >= 15000 )
			{//���ӳ�ʱ9�룬��δ�ظ�http����һ�ɻظ����ӳ�ʱ
				sprintf(((*iterator1).second)->szResponseBody, "{\"code\":%d,\"memo\":\"Network Connnect[ %s : %s ] Timeout .\",\"key\":%d}", IndexApiCode_ConnectTimeout, ((*iterator1).second)->m_rtspStruct.szIP, ((*iterator1).second)->m_rtspStruct.szPort, 0);
				((*iterator1).second)->ResponseHttp(((*iterator1).second)->nClient_http, ((*iterator1).second)->szResponseBody, false);

				//ɾ������������������
				CNetRevcBase_ptr pParentPtr = GetNetRevcBaseClientNoLock(((*iterator1).second)->hParent);
				if (pParentPtr)
				{
					if (pParentPtr->bProxySuccessFlag == false)
					   pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->hParent, sizeof(((*iterator1).second)->hParent));
				}
 			}
		}

		//ץ�ĳ�ʱ��� 
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_SnapPicture_JPEG && ((*iterator1).second)->bSnapSuccessFlag == false )
		{
			if(GetTickCount64() - ((*iterator1).second)->nPrintTime >= 1000 * ((*iterator1).second)->timeout_sec)
			   pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
		}

		//ץ�Ķ��󳬹�����ʱ�����
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_SnapPicture_JPEG && (GetTickCount64() - ((*iterator1).second)->nPrintTime) >= 1000 * ABL_MediaServerPort.snapObjectDuration )
		{
			WriteLog(Log_Debug, "ץ�Ķ����Ѿ������������ʱ�� %d �� ,����ɾ������ȴ����٣�nClient = %llu ", ABL_MediaServerPort.snapObjectDuration,((*iterator1).second)->nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
		}

		//���rtsp��ʼ״̬���ܳ���30�룬����ɾ��
		if (((*iterator1).second)->netBaseNetType == NetBaseNetType_RtspProtectBaseState && (GetTickCount64() - ((*iterator1).second)->nCreateDateTime) >= 1000 * 30 )
		{
			WriteLog(Log_Debug, "rtspЭ�����Ӷ����Ѿ���ʱ 30 �� ,����ɾ������ȴ����٣�nClient = %llu ", ((*iterator1).second)->nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&((*iterator1).second)->nClient, sizeof(((*iterator1).second)->nClient));
		}
   	}

	//����������
	if (ABL_MediaServerPort.hook_enable == 1 && (GetTickCount64() - ABL_MediaServerPort.nServerKeepaliveTime) >= 1000 * ABL_MediaServerPort.keepaliveDuration)
	{
		ABL_MediaServerPort.nServerKeepaliveTime = GetTickCount64();

		MessageNoticeStruct msgNotice;
		msgNotice.nClient = NetBaseNetType_HttpClient_ServerKeepalive;

#ifdef OS_System_Windows
		string strRecordPath = ABL_MediaServerPort.recordPath;
		
#ifdef USE_BOOST
		replace_all(strRecordPath, "\\", "/");
		string serverRunPath = ABL_MediaSeverRunPath;
		replace_all(serverRunPath, "\\", "/");

#else
		ABL::replace_all(strRecordPath, "\\", "/");
		string serverRunPath = ABL_MediaSeverRunPath;
		ABL::replace_all(serverRunPath, "\\", "/");
#endif
	
		SYSTEMTIME st;
		GetLocalTime(&st);
		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_server_keepalive\",\"localipAddress\":\"%s\",\"httpServerPort\":%d,\"recordPath\":\"%s\",\"mediaServerId\":\"%s\",\"serverRunPath\":\"%s\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpServerPort, strRecordPath.c_str(), ABL_MediaServerPort.mediaServerID, serverRunPath.c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
		time_t now;
		time(&now);
		struct tm *local;
		local = localtime(&now);
		sprintf(msgNotice.szMsg, "{\"eventName\":\"on_server_keepalive\",\"localipAddress\":\"%s\",\"httpServerPort\":%d,\"recordPath\":\"%s\",\"mediaServerId\":\"%s\",\"serverRunPath\":\"%s/\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpServerPort, ABL_MediaServerPort.recordPath,ABL_MediaServerPort.mediaServerID, ABL_MediaSeverRunPath, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
#endif
		pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
	}

	if (strlen(szLine))
    	WriteLog(Log_Debug, "��ǰ������Ϣ %s ", szLine);

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
	CNetRevcBase_ptr pNetRevcBase_ptr = NULL ;

	if (address)
	{
 		sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(address);
		sprintf(temp, "%s", ::inet_ntoa(addr->sin_addr));
		nPort = ::ntohs(addr->sin_port);
  	}
 
	if( (pNetRevcBase_ptr = CreateNetRevcBaseClient(NetRevcBaseClient_ServerAccept,srvhandle, clihandle, temp,nPort,"")) == NULL)
		XHNetSDK_Disconnect(clihandle);

	if (pNetRevcBase_ptr != NULL)
	{
		pNetRevcBase_ptr->bRunFlag.exchange(true);
		strcpy(pNetRevcBase_ptr->szClientIP, temp);
		pNetRevcBase_ptr->nClientPort = nPort;

		if (pNetRevcBase_ptr->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
		{//����28181 tcp ������ʽ�������� 
			pNetRevcBase_ptr->SendFirstRequst();
		}
	}
}

void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address)
{
	CNetRevcBase_ptr  pBasePtr = GetNetRevcBaseClient(clihandle);
	if (pBasePtr != NULL)
	{
		pBasePtr->InputNetData(srvhandle, clihandle, data, datasize,address);
		if (pBasePtr->netBaseNetType == NetBaseNetType_NetServerHTTP)
			HttpProcessThreadPool->InsertIntoTask(clihandle);//�еȴ�ʱ����������󶼿��Լ�����̳߳�
		else 
		    NetBaseThreadPool->InsertIntoTask(clihandle);//Window��Linux ƽ̨ʹ�� 
 	}
}

void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle)
{  
    WriteLog(Log_Debug, "Զ�̵����Ӷ��������Ͽ����磬nClient = %llu ",clihandle);
    //�Ƴ�ý�忽��
    DeleteClientMediaStreamSource(clihandle);
	
	//���̳߳س����Ƴ�
	NetBaseThreadPool->DeleteFromTask(clihandle);
	RecordReplayThreadPool->DeleteFromTask(clihandle);
	MessageSendThreadPool->DeleteFromTask(clihandle);
	HttpProcessThreadPool->DeleteFromTask(clihandle);
	
	//ɾ����������Դ����
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
 			WriteLog(Log_Debug, "clihandle = %llu ,URL: %s ,����ʧ�� result: %d ", clihandle,pClient->m_rtspStruct.szSrcRtspPullUrl,result);
			if (pClient->netBaseNetType == NetBaseNetType_RtspClientRecv || pClient->netBaseNetType ==  NetBaseNetType_RtmpClientRecv || pClient->netBaseNetType == NetBaseNetType_HttpFlvClientRecv || 
				pClient->netBaseNetType ==  NetBaseNetType_HttpHLSClientRecv || pClient->netBaseNetType ==  NetBaseNetType_RtspClientPush || pClient->netBaseNetType == NetBaseNetType_RtmpClientPush ||
				pClient->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active)
			{//rtsp ��������ʧ��
				sprintf(pClient->szResponseBody, "{\"code\":%d,\"memo\":\"Network Connect [%s : %s] Failed .\",\"key\":%llu}", IndexApiCode_ConnectFail,pClient->m_rtspStruct.szIP,pClient->m_rtspStruct.szPort, pClient->hParent);
				pClient->ResponseHttp(pClient->nClient_http, pClient->szResponseBody, false);

				//�ж��Ƿ�ɹ����������δ�ɹ���������ɾ�� ������ɹ��������޴�����
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
	{//������ӳɹ������͵�һ������
		CNetRevcBase_ptr pClient = GetNetRevcBaseClient(clihandle);
		if (pClient)
		{
			pClient->bRunFlag.exchange(true);
			WriteLog(Log_Debug, "clihandle = %llu ,URL: %s , ���ӳɹ� result: %d ", clihandle, pClient->m_rtspStruct.szSrcRtspPullUrl, result);
			pClient->bConnectSuccessFlag = true;//���ӳɹ�
			pClient->nClientPort = ntohs(nLocalPort);//���±��ض˿ں�
  			pClient->SendFirstRequst();
		}
	}
}

//���� �¼�֪ͨhttp Client ���� 
#ifdef USE_BOOST

boost::shared_ptr<CNetRevcBase> CreateHttpClientFunc(int nMsgType)
{
	boost::shared_ptr<CNetRevcBase> pMsgClient = NULL;
#else
std::shared_ptr<CNetRevcBase> CreateHttpClientFunc(int nMsgType)
{
	std::shared_ptr<CNetRevcBase> pMsgClient = NULL;
#endif


	
	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_None_reader && strlen(ABL_MediaServerPort.on_stream_none_reader) > 20 && memcmp(ABL_MediaServerPort.on_stream_none_reader,"http",4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_None_reader, 0, 0, ABL_MediaServerPort.on_stream_none_reader, 0, "");
 	
	if(ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_Not_found && strlen(ABL_MediaServerPort.on_stream_not_found) > 20 && memcmp(ABL_MediaServerPort.on_stream_not_found, "http", 4) == 0 )
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_Not_found, 0, 0, ABL_MediaServerPort.on_stream_not_found, 0, "");

 	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_Record_mp4 && strlen(ABL_MediaServerPort.on_record_mp4) > 20 && memcmp(ABL_MediaServerPort.on_record_mp4, "http", 4) == 0 )
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_Record_mp4, 0, 0, ABL_MediaServerPort.on_record_mp4, 0, "");

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_DeleteRecordMp4 && strlen(ABL_MediaServerPort.on_delete_record_mp4) > 20 && memcmp(ABL_MediaServerPort.on_delete_record_mp4, "http", 4) == 0 )
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

	if (ABL_MediaServerPort.hook_enable == 1 && nMsgType == NetBaseNetType_HttpClient_on_rtsp_replay && strlen(ABL_MediaServerPort.on_rtsp_replay) > 20 && memcmp(ABL_MediaServerPort.on_rtsp_replay, "http", 4) == 0)
		pMsgClient = CreateNetRevcBaseClient(NetBaseNetType_HttpClient_on_rtsp_replay, 0, 0, ABL_MediaServerPort.on_rtsp_replay, 0, "");

	return pMsgClient;
}

//����NetBaseNetType ���Ҷ��� 
CNetRevcBase_ptr  GetNetRevcBaseClientByNetType(NetBaseNetType netType)
{
 	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;
	CNetRevcBase_ptr   pClient = NULL;

	for (iterator1 = xh_ABLNetRevcBaseMap.begin(); iterator1 != xh_ABLNetRevcBaseMap.end(); iterator1++)
	{
		pClient = (*iterator1).second;
		if (pClient->netBaseNetType == netType )
		{
 			return pClient;
		}
	}
	return  NULL;
}

//һЩ������ 
void*  ABLMedisServerProcessThread(void* lpVoid)
{
	int nDeleteBreakTimer = 0;
	int nCheckNetRevcBaseClientDisconnectTime = 0;
	int nReConnectStreamProxyTimer = 0;
	int nCreateHttpClientTimer = 0;
	int DeleteExpireM3u8FileTimer = 0;
	ABL_bExitMediaServerRunFlag = false;
	unsigned char* pData = NULL;
	char           szDelMediaSource[string_length_1024] = { 0 };
	int            nLength;
	uint64_t       nClient;
	MessageNoticeStruct msgNotice;
 
	while (ABL_bMediaServerRunFlag)
	{
		//��������쳣�Ͽ���ִ��һЩ������ 
		if (nCheckNetRevcBaseClientDisconnectTime >= 20)
		{
			nCheckNetRevcBaseClientDisconnectTime = 0;
			CheckNetRevcBaseClientDisconnect();
		}
		
		//������Ϣ֪ͨ
		while ((pData = pMessageNoticeFifo.pop(&nLength)) != NULL)
		{
			if (nLength > 0)
			{
				memset((char*)&msgNotice, 0x00, sizeof(msgNotice));
				memcpy((char*)&msgNotice, pData, nLength);

				auto pMsgClient = GetNetRevcBaseClientByNetType((NetBaseNetType)msgNotice.nClient);
				if (pMsgClient == NULL)
 					pMsgClient = CreateHttpClientFunc(msgNotice.nClient);
  				
				if(pMsgClient != NULL )
				{
					if (pMsgClient->bConnectSuccessFlag)
					{
						pMsgClient->PushVideo((unsigned char*)msgNotice.szMsg, strlen(msgNotice.szMsg), "JSON");
						MessageSendThreadPool->InsertIntoTask(pMsgClient->nClient);
					}
					else
						pMsgClient->PushVideo((unsigned char*)msgNotice.szMsg, strlen(msgNotice.szMsg), "JSON");
				}
 			}
 			pMessageNoticeFifo.pop_front();
		}

		//������������
		if (nReConnectStreamProxyTimer >= 2)
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
							pClient->SendFirstRequst(); //ִ������
 					}
				}

				pReConnectStreamProxyFifo.pop_front();
 			}
 		}
 
		//ɾ�����ڵ�M3u8�ļ� 
		if (DeleteExpireM3u8FileTimer >= 10 * 180 )
		{
			DeleteExpireM3u8FileTimer = 0;
			DeleteExpireM3u8File();
		}

		nDeleteBreakTimer ++;
		nCheckNetRevcBaseClientDisconnectTime ++;
		nReConnectStreamProxyTimer ++;
		nCreateHttpClientTimer ++;
		DeleteExpireM3u8FileTimer ++ ;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//	Sleep(100);
	}
 
  	FillNetRevcBaseClientFifo();//�����ж���װ������׼��ɾ��

	while ((pData = pDisconnectBaseNetFifo.pop(&nLength)) != NULL)
	{
		if (nLength == sizeof(nClient))
		{
			memcpy((char*)&nClient, pData, sizeof(nClient));
			if (nClient >= 0)
			{
				DeleteClientMediaStreamSource(nClient);//�Ƴ�ý�忽��
				//���̳߳س����Ƴ�
				NetBaseThreadPool->DeleteFromTask(nClient);
				RecordReplayThreadPool->DeleteFromTask(nClient);
				MessageSendThreadPool->DeleteFromTask(nClient);
				HttpProcessThreadPool->DeleteFromTask(nClient);

				XHNetSDK_Disconnect(nClient);
				DeleteNetRevcBaseClient(nClient);//ִ��ɾ�� 
			}
		}

		pDisconnectBaseNetFifo.pop_front();

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		//Sleep(5);
	}

	//����ɾ��ý��Դ
	while ((pData = pDisconnectMediaSource.pop(&nLength)) != NULL)
	{
		if (nLength > 0 && nLength < 1024)
		{
			memset(szDelMediaSource, 0x00, sizeof(szDelMediaSource));
			memcpy(szDelMediaSource, pData, nLength);

			DeleteMediaStreamSource(szDelMediaSource);//ִ��ɾ�� 
		}

		pDisconnectMediaSource.pop_front();
	}

	ABL_bExitMediaServerRunFlag = true;
	return 0;
}

//�б�����Ķ������Ӿ���
void  SendToMapFromMutePacketList()
{
	for (int i = 0; i < nMaxAddMuteListNumber; i++)
	{
		if (ArrayAddMutePacketList[i] > 0 )
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

//����ɾ����Դ�߳�
void*  ABLMedisServerFastDeleteThread(void* lpVoid)
{
	unsigned char* pData = NULL;
 	int            nLength;
	uint64_t       nClient;
	char           szDelMediaSource[string_length_1024] = { 0 };

	while (ABL_bMediaServerRunFlag)
	{
		//����ɾ��
		while ((pData = pDisconnectBaseNetFifo.pop(&nLength)) != NULL)
		{
			if (nLength == sizeof(nClient))
			{
				memcpy((char*)&nClient, pData, sizeof(nClient));
				if (nClient >= 0)
				{
					//�Ƴ�ý�忽��
				    DeleteClientMediaStreamSource(nClient);
					
					//���̳߳س����Ƴ�
					NetBaseThreadPool->DeleteFromTask(nClient);
					RecordReplayThreadPool->DeleteFromTask(nClient);
					MessageSendThreadPool->DeleteFromTask(nClient);
					HttpProcessThreadPool->DeleteFromTask(nClient);

					//ɾ������SOCKET
					XHNetSDK_Disconnect(nClient);
					
					//ִ��ɾ�������������Դ 
					DeleteNetRevcBaseClient(nClient);
				}
			}

			pDisconnectBaseNetFifo.pop_front();
		}
  
		//����ɾ��ý��Դ
		while ((pData = pDisconnectMediaSource.pop(&nLength)) != NULL)
		{
			if (nLength > 0 && nLength < 1024 )
			{
				memset(szDelMediaSource, 0x00, sizeof(szDelMediaSource));
				memcpy(szDelMediaSource, pData, nLength);
 
				DeleteMediaStreamSource(szDelMediaSource);//ִ��ɾ�� 
 			}

			pDisconnectMediaSource.pop_front();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		//Sleep(20);
	}
	return 0;
}

//��ȡ��ǰ·��
#ifdef OS_System_Windows

void malloc_trim(int n)
{
}

bool   ABLDeleteFile(char* szFileName)
{
	return ::DeleteFile(szFileName);
}

bool GetMediaServerCurrentPath(char *szCurPath)
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
	//IP_ADAPTER_INFO�ṹ��
	PIP_ADAPTER_INFO pIpAdapterInfo = NULL;
	pIpAdapterInfo = new IP_ADAPTER_INFO;

	//�ṹ���С
	unsigned long ulSize = sizeof(IP_ADAPTER_INFO);

	//��ȡ��������Ϣ
	int nRet = GetAdaptersInfo(pIpAdapterInfo, &ulSize);

	if (ERROR_BUFFER_OVERFLOW == nRet)
	{
		//�ռ䲻�㣬ɾ��֮ǰ����Ŀռ�
		delete[]pIpAdapterInfo;

		//���·����С
		pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[ulSize];

		//��ȡ��������Ϣ
		nRet = GetAdaptersInfo(pIpAdapterInfo, &ulSize);

		//��ȡʧ��
		if (ERROR_SUCCESS != nRet)
		{
			if (pIpAdapterInfo != NULL)
			{
				delete[]pIpAdapterInfo;
			}
			return FALSE;
		}
	}

	//MAC ��ַ��Ϣ
	char szMacAddr[20];
	//��ֵָ��
	PIP_ADAPTER_INFO pIterater = pIpAdapterInfo;
	while (pIterater)
	{
		//cout << "�������ƣ�" << pIterater->AdapterName << endl;

		//cout << "����������" << pIterater->Description << endl;

		sprintf_s(szMacAddr, 20, "%02X-%02X-%02X-%02X-%02X-%02X",
			pIterater->Address[0],
			pIterater->Address[1],
			pIterater->Address[2],
			pIterater->Address[3],
			pIterater->Address[4],
			pIterater->Address[5]);

		//cout << "MAC ��ַ��" << szMacAddr << endl;
		//cout << "IP��ַ�б�" << endl << endl;

		//ָ��IP��ַ�б�
		PIP_ADDR_STRING pIpAddr = &pIterater->IpAddressList;
		while (pIpAddr)
		{
			//cout << "IP��ַ��  " << pIpAddr->IpAddress.String << endl;
			//cout << "�������룺" << pIpAddr->IpMask.String << endl;

			if (!(strcmp(pIpAddr->IpAddress.String, "127.0.0.1") == 0 || strcmp(pIpAddr->IpAddress.String, "0.0.0.0") == 0))
			{
			  strIPList += pIpAddr->IpAddress.String;
			  strIPList += ",";
 			}

			//ָ�������б�
			PIP_ADDR_STRING pGateAwayList = &pIterater->GatewayList;
			while (pGateAwayList)
			{
				//cout << "���أ�    " << pGateAwayList->IpAddress.String << endl;
				pGateAwayList = pGateAwayList->Next;
			}
			pIpAddr = pIpAddr->Next;
		}
		//cout << endl << "--------------------------------------------------" << endl;
		pIterater = pIterater->Next;
	}

	//����
	if (pIpAdapterInfo)
	{
		delete[]pIpAdapterInfo;
	}

	WriteLog(Log_Debug, "strIPList = %s ", strIPList.c_str());
 	return true;
}

//����¼��·����������¼���ļ� - windows
void FindHistoryRecordFile(char* szRecordPath)
{
	char tempFileFind[MAX_PATH];
	WIN32_FIND_DATA fd = { 0 };
	WIN32_FIND_DATA fd2 = { 0 };
	WIN32_FIND_DATA fd3 = { 0 };
	bool bFindFlag = true ;
	char szApp[256] = { 0 }, szStream[string_length_512] = { 0 };//����ֻ֧��2��·��
	char szDeleteFile[string_length_512] = { 0 };

	//�����ļ�
	sprintf(tempFileFind, "%s%s", szRecordPath, "*.*");

	HANDLE hFind = FindFirstFile(tempFileFind, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		WriteLog(Log_Debug, "FindHistoryRecordFile �����õ�¼��·�� %s ,û���ҵ��κ��ļ��� ", szRecordPath);
		return ;
	}
 
 	while (bFindFlag)
	{
		bFindFlag = FindNextFile(hFind, &fd);
		if (bFindFlag && !(strcmp(fd.cFileName,"." ) == 0 || strcmp(fd.cFileName, "..") == 0) && fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
 			//WriteLog(Log_Debug, "FindHistoryRecordFile ��·�� %s%s  ", szRecordPath, fd.cFileName);

			sprintf(tempFileFind, "%s%s\\%s", szRecordPath, fd.cFileName, "*.*");
			HANDLE hFind2 = FindFirstFile(tempFileFind, &fd2);
			bool bFindFlag2 = true;
			while (bFindFlag2)
			{
				bFindFlag2 = FindNextFile(hFind2, &fd2);
				{
					if (bFindFlag2 && !(strcmp(fd2.cFileName, ".") == 0 || strcmp(fd2.cFileName, "..") == 0) && fd2.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
					{
						//WriteLog(Log_Debug, "FindHistoryRecordFile ��·�� %s%s\\%s  ", szRecordPath, fd.cFileName, fd2.cFileName);

						sprintf(tempFileFind, "%s%s\\%s\\%s", szRecordPath, fd.cFileName, fd2.cFileName, "*.*");
						HANDLE hFind3 = FindFirstFile(tempFileFind, &fd3);
 						bool bFindFlag3 = true;

						CRecordFileSource_ptr pRecord = CreateRecordFileSource(fd.cFileName, fd2.cFileName);
						if (hFind3 && pRecord && strstr(fd3.cFileName,".mp4") != NULL )
							pRecord->AddRecordFile(fd3.cFileName);
						
						//ɾ���� .m3u8 �ļ�
						if (hFind3 && strstr(fd3.cFileName, ".m3u8") != NULL)
						{
							sprintf(szDeleteFile, "%s%s\\%s\\%s", szRecordPath, fd.cFileName, fd2.cFileName, fd3.cFileName);
							ABLDeleteFile(szDeleteFile);
						}

						while (bFindFlag3 && pRecord)
						{
 							bFindFlag3 = FindNextFile(hFind3, &fd3);
 							if (bFindFlag3 && !(strcmp(fd3.cFileName, ".") == 0 || strcmp(fd3.cFileName, "..") == 0) /* && fd3.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE*/)
							{
								if (pRecord && strstr(fd3.cFileName, ".mp4") != NULL)
								{
									pRecord->AddRecordFile(fd3.cFileName);
								}
								//ɾ���� .m3u8 �ļ�
								if (bFindFlag3 && strstr(fd3.cFileName, ".m3u8") != NULL)
								{
									sprintf(szDeleteFile, "%s%s\\%s\\%s", szRecordPath, fd.cFileName, fd2.cFileName, fd3.cFileName);
									ABLDeleteFile(szDeleteFile);
								}
								//WriteLog(Log_Debug, "FindHistoryRecordFile ���ļ� %s%s\\%s\\%s  ", szRecordPath, fd.cFileName, fd2.cFileName, fd3.cFileName);
							}
 						}
						FindClose(hFind3);

						if(pRecord)
						  pRecord->Sort();
 					}
				}
			}
			FindClose(hFind2);
		}
	}
	FindClose(hFind);
}

//����ͼƬ·����������ͼƬ�ļ� - windows
void FindHistoryPictureFile(char* szPicturePath)
{
	char tempFileFind[MAX_PATH];
	WIN32_FIND_DATA fd = { 0 };
	WIN32_FIND_DATA fd2 = { 0 };
	WIN32_FIND_DATA fd3 = { 0 };
	bool bFindFlag = true;
	char szApp[256] = { 0 }, szStream[string_length_512] = { 0 };//����ֻ֧��2��·��
												   //�����ļ�
	sprintf(tempFileFind, "%s%s", szPicturePath, "*.*");

	HANDLE hFind = FindFirstFile(tempFileFind, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		WriteLog(Log_Debug, "FindHistoryPictureFile �����õ�¼��·�� %s ,û���ҵ��κ��ļ��� ", szPicturePath);
		return;
	}

	while (bFindFlag)
	{
		bFindFlag = FindNextFile(hFind, &fd);
		if (bFindFlag && !(strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) && fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			//WriteLog(Log_Debug, "FindHistoryPictureFile ��·�� %s%s  ", szPicturePath, fd.cFileName);

			sprintf(tempFileFind, "%s%s\\%s", szPicturePath, fd.cFileName, "*.*");
			HANDLE hFind2 = FindFirstFile(tempFileFind, &fd2);
			bool bFindFlag2 = true;
			while (bFindFlag2)
			{
				bFindFlag2 = FindNextFile(hFind2, &fd2);
				{
					if (bFindFlag2 && !(strcmp(fd2.cFileName, ".") == 0 || strcmp(fd2.cFileName, "..") == 0) && fd2.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
					{
						//WriteLog(Log_Debug, "FindHistoryPictureFile ��·�� %s%s\\%s  ", szPicturePath, fd.cFileName, fd2.cFileName);

						sprintf(tempFileFind, "%s%s\\%s\\%s", szPicturePath, fd.cFileName, fd2.cFileName, "*.*");
						HANDLE hFind3 = FindFirstFile(tempFileFind, &fd3);
						bool bFindFlag3 = true;

						CPictureFileSource_ptr pPicture = CreatePictureFileSource(fd.cFileName, fd2.cFileName);

						while (bFindFlag3 && pPicture)
						{
							bFindFlag3 = FindNextFile(hFind3, &fd3);
							if (bFindFlag3 && !(strcmp(fd3.cFileName, ".") == 0 || strcmp(fd3.cFileName, "..") == 0) /*&& fd3.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE */)
							{
								if (pPicture)
								{
									pPicture->AddPictureFile(fd3.cFileName);
								}
								//WriteLog(Log_Debug, "FindHistoryPictureFile ���ļ� %s%s\\%s\\%s  ", szPicturePath, fd.cFileName, fd2.cFileName, fd3.cFileName);
							}
						}
						FindClose(hFind3);

						if(pPicture)
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

//ɾ���ļ�
bool  ABLDeleteFile(char* szFileName)
{
	int nRet = unlink(szFileName);
	return (nRet == 0) ? true : false;
}

//����¼��·����������¼���ļ� - linux 
void FindHistoryRecordFile(char* szRecordPath)
{
	struct dirent * filename;    // return value for readdir()
	DIR * dir;                   // return value for opendir()
	dir = opendir(szRecordPath);
	char  szTempPath[512] = { 0 };
	char  szDeleteFile[1024] = { 0 };

 	while ((filename = readdir(dir)) != NULL)
	{
		// get rid of "." and ".."
		if (strcmp(filename->d_name, ".") == 0 ||
			strcmp(filename->d_name, "..") == 0)
			continue;
			
		if (strlen(filename->d_name) > 0 )
		{
			//WriteLog(Log_Debug, "FindHistoryRecordFile ��·�� %s ", filename->d_name);

			struct dirent * filename2;     
			DIR *           dir2;                   
			memset(szTempPath, 0x00, sizeof(szTempPath));
			sprintf(szTempPath,"%s%s", szRecordPath, filename->d_name);
			dir2 = opendir(szTempPath);
   
			while ((filename2 = readdir(dir2)) != NULL)
			{
				if (!(strcmp(filename2->d_name, ".") == 0 || strcmp(filename2->d_name, "..") == 0))
				{
					//WriteLog(Log_Debug, "FindHistoryRecordFile ����2��·�� %s ", filename2->d_name);

					struct dirent * filename3;     
					DIR *           dir3;                   
					memset(szTempPath, 0x00, sizeof(szTempPath));
					sprintf(szTempPath,"%s%s/%s", szRecordPath, filename->d_name,filename2->d_name);
					dir3 = opendir(szTempPath);
					
					CRecordFileSource_ptr pRecord = CreateRecordFileSource(filename->d_name,filename2->d_name);
					
					while ((filename3 = readdir(dir3)) != NULL && pRecord)
					{
						if (!(strcmp(filename3->d_name, ".") == 0 || strcmp(filename3->d_name, "..") == 0))
						{
							//WriteLog(Log_Debug, "FindHistoryRecordFile ,¼���ļ����� %s ", filename3->d_name);
							if (pRecord && strstr(filename3->d_name,".mp4") != NULL )
							{
								pRecord->AddRecordFile(filename3->d_name);
							}
							else if (strstr(filename3->d_name, ".m3u8") != NULL)
							{//ɾ�������� m3u8 �ļ� 
								sprintf(szDeleteFile, "%s%s/%s/%s", szRecordPath, filename->d_name, filename2->d_name, filename3->d_name);
								ABLDeleteFile(szDeleteFile);
							}
						}
					}
					closedir(dir3);

					if(pRecord)
					pRecord->Sort();
				}
			}//while ((filename2 = readdir(dir2)) != NULL)
			closedir(dir2);
			
		}
	}//while ((filename = readdir(dir)) != NULL)
	closedir(dir);
}

//����ͼƬ·����������ͼƬ�ļ� - linux 
void FindHistoryPictureFile(char* szPicturePath)
{
	struct dirent * filename;    // return value for readdir()
	DIR * dir;                   // return value for opendir()
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
			//WriteLog(Log_Debug, "FindHistoryPictureFile ��·�� %s ", filename->d_name);

			struct dirent * filename2;
			DIR *           dir2;
			memset(szTempPath, 0x00, sizeof(szTempPath));
			sprintf(szTempPath, "%s%s", szPicturePath, filename->d_name);
			dir2 = opendir(szTempPath);

			while ((filename2 = readdir(dir2)) != NULL)
			{
				if (!(strcmp(filename2->d_name, ".") == 0 || strcmp(filename2->d_name, "..") == 0))
				{
					//WriteLog(Log_Debug, "FindHistoryPictureFile ����2��·�� %s ", filename2->d_name);

					struct dirent * filename3;
					DIR *           dir3;
					memset(szTempPath, 0x00, sizeof(szTempPath));
					sprintf(szTempPath, "%s%s/%s", szPicturePath, filename->d_name, filename2->d_name);
					dir3 = opendir(szTempPath);

					CPictureFileSource_ptr pPicture = CreatePictureFileSource(filename->d_name, filename2->d_name);

					while ((filename3 = readdir(dir3)) != NULL && pPicture)
					{
						if (!(strcmp(filename3->d_name, ".") == 0 || strcmp(filename3->d_name, "..") == 0))
						{
							//WriteLog(Log_Debug, "FindHistoryPictureFile ,¼���ļ����� %s ", filename3->d_name);
							if (pPicture)
							{
								pPicture->AddPictureFile(filename3->d_name);
							}
						}
					}
					closedir(dir3);

					if(pPicture)
					  pPicture->Sort();
				}
			}//while ((filename2 = readdir(dir2)) != NULL)
			closedir(dir2);
		}
	}//while ((filename = readdir(dir)) != NULL)
	closedir(dir);
}

#endif

#ifdef OS_System_Windows
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	pcm16_alaw_tableinit();
	pcm16_ulaw_tableinit();
 
	//��ȡcpu��������� 
	ABL_nCurrentSystemCpuCount = std::thread::hardware_concurrency(); 
	if (ABL_nCurrentSystemCpuCount <= 4)
		ABL_nCurrentSystemCpuCount = 4;
	else if (ABL_nCurrentSystemCpuCount > 256)
		ABL_nCurrentSystemCpuCount = 256;

ABL_Restart:
	unsigned char nGet;
	int nBindHttp, nBindRtsp, nBindRtmp,nBindWsFlv,nBindHttpFlv, nBindHls,nBindMp4,nBindRecvAudio,nBingPS10000,nBind1078, nBind8192;

	memset(ABL_MediaSeverRunPath, 0x00, sizeof(ABL_MediaSeverRunPath));
	memset(ABL_wwwMediaPath, 0x00, sizeof(ABL_wwwMediaPath));
	memset(ABL_szLocalIP, 0x00, sizeof(ABL_szLocalIP));

	nMaxAddMuteListNumber = 0;
	for (int i = 0; i < 8192; i++)
		ArrayAddMutePacketList[i] = 0;

#ifdef OS_System_Windows

	//���������̨���ڣ������ٿ�ס 
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
		WriteLog(Log_Error, "û���ҵ������ļ� ��%s ", szConfigFileName);
		//Sleep(3000);
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		return -1;
	}

	//��ȡ�û����õ�IP��ַ 
	strcpy(ABL_szLocalIP, ABL_ConfigFile.GetValue("ABLMediaServer", "localipAddress", ""));

	//�Զ���ȡIP��ַ
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
	WriteLog(Log_Debug, "����IP��ַ ABL_szLocalIP : %s ", ABL_szLocalIP);
	WriteLog(Log_Debug, "����cpu����������� nCurrentSystemCpuCount %d ", ABL_nCurrentSystemCpuCount);
	
	strcpy(ABL_MediaServerPort.secret, ABL_ConfigFile.GetValue("ABLMediaServer", "secret", "035c73f7-bb6b-4889-a715-d9eb2d1925cc111"));
	ABL_MediaServerPort.nHttpServerPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "httpServerPort", "8081"));
	ABL_MediaServerPort.nRtspPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "rtspPort", "554"));
	ABL_MediaServerPort.nRtmpPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "rtmpPort", "1935"));
	ABL_MediaServerPort.nHttpFlvPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "httpFlvPort", "8088"));
	ABL_MediaServerPort.nWSFlvPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "wsFlvPort", "6088"));
	ABL_MediaServerPort.nHttpMp4Port = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "httpMp4Port", "8089"));
	ABL_MediaServerPort.ps_tsRecvPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "ps_tsRecvPort", "10000"));
 	ABL_MediaServerPort.nHlsPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "hlsPort", "9081"));
	ABL_MediaServerPort.WsRecvPcmPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "WsRecvPcmPort", "9298"));
	ABL_MediaServerPort.nHlsEnable = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "hls_enable", "5"));
	ABL_MediaServerPort.nHLSCutType = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "hlsCutType", "1"));
	ABL_MediaServerPort.nH265CutType = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "h265CutType", "1"));
	ABL_MediaServerPort.hlsCutTime = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "hlsCutTime", "1"));
	ABL_MediaServerPort.nMaxTsFileCount = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "maxTsFileCount", "10"));
	strcpy(ABL_MediaServerPort.wwwPath, ABL_ConfigFile.GetValue("ABLMediaServer", "wwwPath", ""));

	ABL_MediaServerPort.nRecvThreadCount = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "RecvThreadCount", "64"));
	ABL_MediaServerPort.nSendThreadCount = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "SendThreadCount", "64"));
	ABL_MediaServerPort.nRecordReplayThread = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "RecordReplayThread", "32"));
	ABL_MediaServerPort.nGBRtpTCPHeadType = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "GB28181RtpTCPHeadType", "0"));
	ABL_MediaServerPort.nEnableAudio = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "enable_audio", "0"));
	ABL_MediaServerPort.nIOContentNumber = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "IOContentNumber", "16"));
	ABL_MediaServerPort.nThreadCountOfIOContent = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "ThreadCountOfIOContent", "16"));
	ABL_MediaServerPort.nReConnectingCount = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "ReConnectingCount", "48000"));

	strcpy(ABL_MediaServerPort.recordPath, ABL_ConfigFile.GetValue("ABLMediaServer", "recordPath", ""));
	ABL_MediaServerPort.pushEnable_mp4 = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "pushEnable_mp4", "0"));
	ABL_MediaServerPort.fileSecond = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "fileSecond", "180"));
	ABL_MediaServerPort.videoFileFormat = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "videoFileFormat", "1"));
	ABL_MediaServerPort.fileKeepMaxTime = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "fileKeepMaxTime", "12"));
	ABL_MediaServerPort.recordFileCutType = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "recordFileCutType", "1"));
	ABL_MediaServerPort.enable_GetFileDuration = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "enable_GetFileDuration", "0"));
  	ABL_MediaServerPort.fileRepeat = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "fileRepeat", "0"));
	ABL_MediaServerPort.httpDownloadSpeed = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "httpDownloadSpeed", "6"));

	ABL_MediaServerPort.maxTimeNoOneWatch = atof(ABL_ConfigFile.GetValue("ABLMediaServer", "maxTimeNoOneWatch", "2"));
	ABL_MediaServerPort.nG711ConvertAAC = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "G711ConvertAAC", "1"));

	strcpy(ABL_MediaServerPort.picturePath, ABL_ConfigFile.GetValue("ABLMediaServer", "picturePath", ""));
	ABL_MediaServerPort.pictureMaxCount = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "pictureMaxCount", "30"));
	ABL_MediaServerPort.captureReplayType = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "captureReplayType", "1"));
	ABL_MediaServerPort.deleteSnapPicture = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "deleteSnapPicture", "0"));
	ABL_MediaServerPort.iframeArriveNoticCount = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "iframeArriveNoticCount", "30"));
 	ABL_MediaServerPort.maxSameTimeSnap = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "maxSameTimeSnap", "16"));
	ABL_MediaServerPort.snapObjectDestroy = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "snapObjectDestroy", "1"));
	ABL_MediaServerPort.snapObjectDuration = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "snapObjectDuration", "120"));
	ABL_MediaServerPort.snapOutPictureWidth = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "snapOutPictureWidth", "0"));
	ABL_MediaServerPort.snapOutPictureHeight = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "snapOutPictureHeight", "0"));
	
	ABL_MediaServerPort.H265ConvertH264_enable = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "H265ConvertH264_enable", "1"));
	ABL_MediaServerPort.H265DecodeCpuGpuType = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "H265DecodeCpuGpuType", "0"));
	ABL_MediaServerPort.convertOutWidth = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "convertOutWidth", "7210"));
	ABL_MediaServerPort.convertOutHeight = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "convertOutHeight", "1480"));
	ABL_MediaServerPort.convertMaxObject = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "convertMaxObject", "214"));
	ABL_MediaServerPort.convertOutBitrate = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "convertOutBitrate", "123"));
	ABL_MediaServerPort.H264DecodeEncode_enable = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "H264DecodeEncode_enable", "0"));
	ABL_MediaServerPort.filterVideo_enable = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "filterVideo_enable", "0"));
	strcpy(ABL_MediaServerPort.filterVideoText, ABL_ConfigFile.GetValue("ABLMediaServer", "filterVideo_text", ""));
	ABL_MediaServerPort.nFilterFontSize = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontSize", "12"));
	strcpy(ABL_MediaServerPort.nFilterFontColor, ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontColor", "red"));
	ABL_MediaServerPort.nFilterFontLeft = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontLeft", "5"));
	ABL_MediaServerPort.nFilterFontTop = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontTop", "5"));
	ABL_MediaServerPort.nFilterFontAlpha = atof(ABL_ConfigFile.GetValue("ABLMediaServer", "FilterFontAlpha", "0.5"));
	ABL_MediaServerPort.MaxDiconnectTimeoutSecond = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "MaxDiconnectTimeoutSecond", "18"));
	ABL_MediaServerPort.ForceSendingIFrame = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "ForceSendingIFrame", "0"));
	ABL_MediaServerPort.gb28181LibraryUse = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "gb28181LibraryUse", "1"));
	ABL_MediaServerPort.httqRequstClose = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "httqRequstClose", "0"));
	ABL_MediaServerPort.flvPlayAddMute = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "flvPlayAddMute", "1"));
	
	//��ȡ�¼�֪ͨ����
	ABL_MediaServerPort.hook_enable = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "hook_enable", "0"));
	ABL_MediaServerPort.noneReaderDuration = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "noneReaderDuration", "32"));
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
	strcpy(ABL_MediaServerPort.on_rtsp_replay, ABL_ConfigFile.GetValue("ABLMediaServer", "on_rtsp_replay", ""));
	ABL_MediaServerPort.keepaliveDuration = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "keepaliveDuration", "20"));
	ABL_MediaServerPort.nWebRtcPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "webrtcPort", "8000"));
	ABL_MediaServerPort.GB28181RtpMinPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "GB28181RtpMinPort", "35000"));
	ABL_MediaServerPort.GB28181RtpMaxPort = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "GB28181RtpMaxPort", "40000"));
	ABL_MediaServerPort.n1078Port = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "1078Port", "1078"));
	ABL_MediaServerPort.jtt1078Version = atoi(ABL_ConfigFile.GetValue("ABLMediaServer", "jtt1078Version", "2016"));

	if (ABL_MediaServerPort.httpDownloadSpeed > 10)
		ABL_MediaServerPort.httpDownloadSpeed = 10;
	else if (ABL_MediaServerPort.httpDownloadSpeed <= 0)
		ABL_MediaServerPort.httpDownloadSpeed = 1;
 
	if(strlen(ABL_MediaServerPort.recordPath) > 0 && GetFileAttributes(ABL_MediaServerPort.recordPath) == -1)
		WriteLog(Log_Debug, "���õ�¼��·�� %s �ǲ����ڵ�·������λΪ����ĵ�ǰ·�� ", ABL_MediaServerPort.recordPath);

	if (strlen(ABL_MediaServerPort.recordPath) == 0 || GetFileAttributes(ABL_MediaServerPort.recordPath) == -1 )
	   strcpy(ABL_MediaServerPort.recordPath, ABL_MediaSeverRunPath);
	else
	{//�û����õ�·��,��ֹ�û�û�д�����·��
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.recordPath;
		while (true)
		{
			nPos = strPath.find("\\", nPos+3);
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

	if (strlen(ABL_MediaServerPort.picturePath) > 0 && GetFileAttributes(ABL_MediaServerPort.picturePath) == -1)
		WriteLog(Log_Debug, "���õ�ץ��ͼƬ·�� %s �ǲ����ڵ�·������λΪ����ĵ�ǰ·�� ", ABL_MediaServerPort.picturePath);

	//����ͼƬץ��·��
	if (strlen(ABL_MediaServerPort.picturePath) == 0 || GetFileAttributes(ABL_MediaServerPort.picturePath) == -1)
		strcpy(ABL_MediaServerPort.picturePath, ABL_MediaSeverRunPath);
	else
	{//�û����õ�·��,��ֹ�û�û�д�����·��
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

	if (strlen(ABL_MediaServerPort.wwwPath) > 0 && GetFileAttributes(ABL_MediaServerPort.wwwPath) == -1)
		WriteLog(Log_Debug, "���õ���Ƭ�ļ�·�� %s �ǲ����ڵ�·������λΪ����ĵ�ǰ·�� ", ABL_MediaServerPort.wwwPath);

	//������Ƭ·��
	if (strlen(ABL_MediaServerPort.wwwPath) == 0 || GetFileAttributes(ABL_MediaServerPort.wwwPath) == -1)
		strcpy(ABL_MediaServerPort.wwwPath, ABL_MediaSeverRunPath);
	else
	{//�û����õ�·��,��ֹ�û�û�д�����·��
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

	//������·�� record 
	if (ABL_MediaServerPort.recordPath[strlen(ABL_MediaServerPort.recordPath) - 1] != '\\')
		strcat(ABL_MediaServerPort.recordPath, "\\");
	strcat(ABL_MediaServerPort.recordPath, "record\\");
	::CreateDirectory(ABL_MediaServerPort.recordPath, NULL);

	//������·�� picture 
	if (ABL_MediaServerPort.picturePath[strlen(ABL_MediaServerPort.picturePath) - 1] != '\\')
		strcat(ABL_MediaServerPort.picturePath, "\\");
	strcat(ABL_MediaServerPort.picturePath, "picture\\");
	::CreateDirectory(ABL_MediaServerPort.picturePath, NULL);

	sprintf(ABL_MediaServerPort.debugPath, "%s%s\\", ABL_MediaSeverRunPath, "debugFile");
	::CreateDirectory(ABL_MediaServerPort.debugPath, NULL);

	WriteLog(Log_Debug, "�����ɹ�¼��·����%s ,����ͼƬ·��: %s  ", ABL_MediaServerPort.recordPath, ABL_MediaServerPort.picturePath);
	//����ʷ¼���ļ�װ�� list  
	FindHistoryRecordFile(ABL_MediaServerPort.recordPath);
	//����ʷͼƬװ��list 
	FindHistoryPictureFile(ABL_MediaServerPort.picturePath);

	rtp_packet_setsize(65535);

	if (ABL_MediaServerPort.H265ConvertH264_enable == 1 && ABL_MediaServerPort.H265DecodeCpuGpuType == 1 && ABL_bInitCudaSDKFlag == false)
	{///Ӣΰ��
		if (true/*���ܸ����Կ��������жϣ���Щ������˫�Կ� strstr(pD3DName.Description, "GeForce") != NULL || strstr(pD3DName.Description, "NVIDIA") != NULL*/)
		{//Ӣΰ���Կ�Ӳ����
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

			if(ABL_bCudaFlag == false || ABL_nCudaCount <= 0)
				ABL_MediaServerPort.H265DecodeCpuGpuType = 0; //�ָ�cpu���
			else
			{//cuda ��Դ�Ѿ������� 
				ABL_bInitCudaSDKFlag = true; 
				WriteLog(Log_Debug, "����Ӣΰ���Կ� ABL_bCudaFlag = %d, Ӣΰ���Կ����� : %d  ", ABL_bCudaFlag, ABL_nCudaCount);
			}
		}
		else
			ABL_MediaServerPort.H265DecodeCpuGpuType = 0; //�ָ�cpu���
	}
	else if (ABL_MediaServerPort.H265ConvertH264_enable == 1 && ABL_MediaServerPort.H265DecodeCpuGpuType == 2 )
	{//amd 
		ABL_MediaServerPort.H265DecodeCpuGpuType = 0; //�ָ�cpu���
	}

	if(ABL_MediaServerPort.H265ConvertH264_enable == 1)
	   WriteLog(Log_Debug, "ABL_MediaServerPort.H265DecodeCpuGpuType = %d ", ABL_MediaServerPort.H265DecodeCpuGpuType);
#else
	InitLogFile();
    if(argc >= 3)
      WriteLog(Log_Debug, "argc = %d, argv[0] = %s ,argv[1] = %s ,argv[2] = %s ", argc,argv[0],argv[1],argv[2]);
 
	strcpy(ABL_MediaSeverRunPath, get_current_dir_name());
	if(argc >=3 && strcmp(argv[1],"-c") == 0)
	{//�������ļ�����
	   strcpy(szConfigFileName,argv[2]) ;	
	}else
	  sprintf(szConfigFileName, "%s/%s", ABL_MediaSeverRunPath, "ABLMediaServer.ini");
	WriteLog(Log_Debug, "ABLMediaServer.ini : %s ", szConfigFileName);
 	if (access(szConfigFileName, F_OK) != 0)
	{
		WriteLog(Log_Debug, "��ǰ·�� %s û�������ļ� ABLMediaServer.ini�����顣", ABL_MediaSeverRunPath);
		return -1;
	}
	if (ABL_ConfigFile.LoadFile(szConfigFileName) != SI_OK)
	{
		WriteLog(Log_Debug, "��ȡ�����ļ� %s ʧ�� ��", szConfigFileName);
		return -1;
	}
	
	ABL_SetPathAuthority(ABL_MediaSeverRunPath);
	
	//��ȡ�û����õ�IP��ַ 
	strcpy(ABL_szLocalIP, ABL_ConfigFile.GetValue("ABLMediaServer", "localipAddress",""));
	WriteLog(Log_Debug, "��ȡ�������ļ���IP : %s ", ABL_szLocalIP);
	WriteLog(Log_Debug, "����cpu����������� nCurrentSystemCpuCount %d ", ABL_nCurrentSystemCpuCount);

	//��ȡIP��ַ 
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
	WriteLog(Log_Debug, "����IP��ַ ABL_szLocalIP : %s ", ABL_szLocalIP);
	
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
	ABL_MediaServerPort.recordFileCutType = ABL_ConfigFile.GetLongValue("ABLMediaServer", "recordFileCutType",1);
 	ABL_MediaServerPort.fileRepeat = ABL_ConfigFile.GetLongValue("ABLMediaServer", "fileRepeat",0);
	ABL_MediaServerPort.enable_GetFileDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "enable_GetFileDuration", 0);
 	ABL_MediaServerPort.httpDownloadSpeed = ABL_ConfigFile.GetLongValue("ABLMediaServer", "httpDownloadSpeed",0);

	ABL_MediaServerPort.maxTimeNoOneWatch = ABL_ConfigFile.GetDoubleValue("ABLMediaServer", "maxTimeNoOneWatch",0);
	ABL_MediaServerPort.nG711ConvertAAC = ABL_ConfigFile.GetLongValue("ABLMediaServer", "G711ConvertAAC",1); 

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
 
	//��ȡ�¼�֪ͨ����
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
	strcpy(ABL_MediaServerPort.on_rtsp_replay, ABL_ConfigFile.GetValue("ABLMediaServer", "on_rtsp_replay", ""));

	ABL_MediaServerPort.MaxDiconnectTimeoutSecond = ABL_ConfigFile.GetLongValue("ABLMediaServer", "MaxDiconnectTimeoutSecond",0);
	ABL_MediaServerPort.ForceSendingIFrame = ABL_ConfigFile.GetLongValue("ABLMediaServer", "ForceSendingIFrame",0);
	ABL_MediaServerPort.gb28181LibraryUse = ABL_ConfigFile.GetLongValue("ABLMediaServer", "gb28181LibraryUse",0);
	ABL_MediaServerPort.keepaliveDuration = ABL_ConfigFile.GetLongValue("ABLMediaServer", "keepaliveDuration",0);
	ABL_MediaServerPort.flvPlayAddMute = ABL_ConfigFile.GetLongValue("ABLMediaServer", "flvPlayAddMute",0);
	ABL_MediaServerPort.nWebRtcPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "webrtcPort",8289);
	ABL_MediaServerPort.GB28181RtpMinPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "GB28181RtpMinPort", 35000);
	ABL_MediaServerPort.GB28181RtpMaxPort = ABL_ConfigFile.GetLongValue("ABLMediaServer", "GB28181RtpMaxPort", 40000);
	ABL_MediaServerPort.n1078Port = ABL_ConfigFile.GetLongValue("ABLMediaServer", "1078Port", 1078);
	ABL_MediaServerPort.jtt1078Version = ABL_ConfigFile.GetLongValue("ABLMediaServer", "jtt1078Version", 2016);
  
	if (ABL_MediaServerPort.httpDownloadSpeed > 10)
		ABL_MediaServerPort.httpDownloadSpeed = 10;
	else if (ABL_MediaServerPort.httpDownloadSpeed <= 0)
		ABL_MediaServerPort.httpDownloadSpeed = 1;

	if (strlen(ABL_MediaServerPort.recordPath) > 0 && isPathExist(ABL_MediaServerPort.recordPath) == false )
		WriteLog(Log_Debug, "���õ�¼��·�� %s �ǲ����ڵ�·������λΪ����ĵ�ǰ·�� ", ABL_MediaServerPort.recordPath);
 
	if (strlen(ABL_MediaServerPort.recordPath) == 0 || isPathExist(ABL_MediaServerPort.recordPath) == false )
		strcpy(ABL_MediaServerPort.recordPath, ABL_MediaSeverRunPath);
	else
	{//�û����õ�·��,��ֹ�û�û�д�����·��
		int nPos = 0;
		char szTempPath[512] = { 0 };
		string strPath = ABL_MediaServerPort.recordPath;
		while (true)
		{
			nPos = strPath.find("/", nPos+1);
			if (nPos > 0)
			{
				memcpy(szTempPath, ABL_MediaServerPort.recordPath, nPos);
				umask(0);
				mkdir(szTempPath, 777);
				ABL_SetPathAuthority(szTempPath);
				
	            WriteLog(Log_Debug, "������·����%s ", szTempPath);
			}
			else
			{
				umask(0);
 				mkdir(ABL_MediaServerPort.recordPath, 777);
				ABL_SetPathAuthority(ABL_MediaServerPort.recordPath);
	            WriteLog(Log_Debug, "������·����%s ", ABL_MediaServerPort.recordPath);
				break;
			}
		}
	}

 	if (strlen(ABL_MediaServerPort.picturePath) > 0 && isPathExist(ABL_MediaServerPort.picturePath) == false)
		WriteLog(Log_Debug, "���õ�ץ��ͼƬ·�� %s �ǲ����ڵ�·������λΪ����ĵ�ǰ·�� ", ABL_MediaServerPort.picturePath);
 
	if (strlen(ABL_MediaServerPort.picturePath) == 0 || isPathExist(ABL_MediaServerPort.picturePath) == false)
		strcpy(ABL_MediaServerPort.picturePath, ABL_MediaSeverRunPath);
	else
	{//�û����õ�·��,��ֹ�û�û�д�����·��
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
				ABL_SetPathAuthority(szTempPath);

				WriteLog(Log_Debug, "������·����%s ", szTempPath);
			}
			else
			{
				umask(0);
				mkdir(ABL_MediaServerPort.picturePath, 777);
				ABL_SetPathAuthority(ABL_MediaServerPort.picturePath);
				
				WriteLog(Log_Debug, "������·����%s ", ABL_MediaServerPort.picturePath);
				break;
			}
		}
	}
	if (strlen(ABL_MediaServerPort.wwwPath) > 0 && isPathExist(ABL_MediaServerPort.wwwPath) == false)
		WriteLog(Log_Debug, "���õ���Ƭ�ļ�·�� %s �ǲ����ڵ�·������λΪ����ĵ�ǰ·�� ", ABL_MediaServerPort.wwwPath);

	if (strlen(ABL_MediaServerPort.wwwPath) == 0 || isPathExist(ABL_MediaServerPort.wwwPath) == false)
		strcpy(ABL_MediaServerPort.wwwPath, ABL_MediaSeverRunPath);
	else
	{//�û����õ�·��,��ֹ�û�û�д�����·��
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
				ABL_SetPathAuthority(szTempPath);

				WriteLog(Log_Debug, "������·����%s ", szTempPath);
			}
			else
			{
				umask(0);
				mkdir(ABL_MediaServerPort.wwwPath, 777);
				ABL_SetPathAuthority(ABL_MediaServerPort.wwwPath);
				
				WriteLog(Log_Debug, "������·����%s ", ABL_MediaServerPort.wwwPath);
				break;
			}
		}
	}
	//������·�� record 
	if (ABL_MediaServerPort.recordPath[strlen(ABL_MediaServerPort.recordPath) - 1] != '/')
		strcat(ABL_MediaServerPort.recordPath, "/");
	strcat(ABL_MediaServerPort.recordPath, "record/");
	umask(0);
	mkdir(ABL_MediaServerPort.recordPath, 777);
	ABL_SetPathAuthority(ABL_MediaServerPort.recordPath);

    //���������ļ�·��
	sprintf(ABL_MediaServerPort.debugPath, "%s/debugFile/", ABL_MediaSeverRunPath);
	umask(0);
	mkdir(ABL_MediaServerPort.debugPath, 777);
	ABL_SetPathAuthority(ABL_MediaServerPort.debugPath);

	//������·�� picture 
	if (ABL_MediaServerPort.picturePath[strlen(ABL_MediaServerPort.picturePath) - 1] != '/')
		strcat(ABL_MediaServerPort.picturePath, "/");
	strcat(ABL_MediaServerPort.picturePath, "picture/");
	umask(0);
	mkdir(ABL_MediaServerPort.picturePath, 777);
	ABL_SetPathAuthority(ABL_MediaServerPort.picturePath);
	WriteLog(Log_Debug, "�����ɹ�¼��·����%s ,����ͼƬ·���ɹ���%s ", ABL_MediaServerPort.recordPath, ABL_MediaServerPort.picturePath);

	struct rlimit rlim, rlim_new;
	if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim_new) != 0) {
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			setrlimit(RLIMIT_CORE, &rlim_new);
		}
	 WriteLog(Log_Debug,"����core�ļ���СΪ: %llu " , rlim_new.rlim_cur);
	}

	if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_NOFILE, &rlim_new) != 0) {
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			setrlimit(RLIMIT_NOFILE, &rlim_new);
		}
	  WriteLog(Log_Debug, "�������Socket�׽������������: %llu " , rlim_new.rlim_cur);
	}

	//����ʷ¼���ļ�װ�� list  
	FindHistoryRecordFile(ABL_MediaServerPort.recordPath);
	//����ʷͼƬװ��list 
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
			ABL_MediaServerPort.H265DecodeCpuGpuType = 0 ;//�ָ�Ϊ����� 
	   }else 
		  ABL_MediaServerPort.H265DecodeCpuGpuType = 0 ;//�ָ�Ϊ�����

	   WriteLog(Log_Debug, " H265DecodeCpuGpuType = %d ",ABL_MediaServerPort.H265DecodeCpuGpuType );
	}
#endif
	WriteLog(Log_Debug, "....��������ý������� ABLMediaServer Start ....");
	
	WriteLog(Log_Debug, "�������ļ��ж�ȡ�� \r\n���в�����http = %d, rtsp = %d , rtmp = %d ,http-flv = %d ,ws-flv = %d ,http-mp4 = %d, nHlsPort = %d , nHlsEnable = %d nHLSCutType = %d \r\n ��������߳����� RecvThreadCount = %d ���緢���߳����� SendThreadCount = %d ",
		                     ABL_MediaServerPort.nHttpServerPort,ABL_MediaServerPort.nRtspPort, ABL_MediaServerPort.nRtmpPort, ABL_MediaServerPort.nHttpFlvPort, ABL_MediaServerPort.nWSFlvPort, ABL_MediaServerPort.nHttpMp4Port, ABL_MediaServerPort.nHlsPort, ABL_MediaServerPort.nHlsEnable , ABL_MediaServerPort.nHLSCutType,
		                     ABL_MediaServerPort.nRecvThreadCount, ABL_MediaServerPort.nSendThreadCount);

	if (ABL_MediaServerPort.hlsCutTime <= 0)
		ABL_MediaServerPort.hlsCutTime = 1 ;
	else if (ABL_MediaServerPort.hlsCutTime > 120)
		ABL_MediaServerPort.hlsCutTime = 120;
	
	//�������糬ʱʱ��
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
	pDisconnectMediaSource.InitFifo(1024 * 1024 * 2);

	//����www��·�� 
#ifdef OS_System_Windows
	if (ABL_MediaServerPort.wwwPath[strlen(ABL_MediaServerPort.wwwPath) - 1] != '\\')
		strcat(ABL_MediaServerPort.wwwPath, "\\");
	sprintf(ABL_wwwMediaPath, "%swww", ABL_MediaServerPort.wwwPath);
	::CreateDirectory(ABL_wwwMediaPath,NULL);
#else
	if (ABL_MediaServerPort.wwwPath[strlen(ABL_MediaServerPort.wwwPath) - 1] != '/')
		strcat(ABL_MediaServerPort.wwwPath, "/");
	sprintf(ABL_wwwMediaPath, "%swww", ABL_MediaServerPort.wwwPath);
	umask(0);
	mkdir(ABL_wwwMediaPath, 777);
    ABL_SetPathAuthority(ABL_wwwMediaPath);	
#endif
	WriteLog(Log_Debug, "www ·��Ϊ %s ", ABL_wwwMediaPath);
	
	//��ֹ�û�����д
	if ((ABL_MediaServerPort.snapOutPictureWidth == 0 && ABL_MediaServerPort.snapOutPictureHeight != 0) || (ABL_MediaServerPort.snapOutPictureWidth != 0 && ABL_MediaServerPort.snapOutPictureHeight == 0))
		ABL_MediaServerPort.snapOutPictureWidth = ABL_MediaServerPort.snapOutPictureHeight = 0;
	if (ABL_MediaServerPort.snapOutPictureWidth > 1920)
		ABL_MediaServerPort.snapOutPictureWidth = 1920;
	if (ABL_MediaServerPort.snapOutPictureHeight > 1080)
		ABL_MediaServerPort.snapOutPictureHeight = 1080;

	//ȷ��������ա������߳�����
	if (ABL_nCurrentSystemCpuCount > 0 && ABL_nCurrentSystemCpuCount <= 4)
		ABL_MediaServerPort.nRecvThreadCount = ABL_nCurrentSystemCpuCount * 4 ;
	else if (ABL_nCurrentSystemCpuCount > 4 && ABL_nCurrentSystemCpuCount <= 8)
		ABL_MediaServerPort.nRecvThreadCount = ABL_nCurrentSystemCpuCount * 3;
	else if (ABL_nCurrentSystemCpuCount > 8 && ABL_nCurrentSystemCpuCount <= 24)
		ABL_MediaServerPort.nRecvThreadCount = ABL_nCurrentSystemCpuCount * 2;
	else
		ABL_MediaServerPort.nRecvThreadCount = ABL_nCurrentSystemCpuCount ;

	ABL_nGB28181Port = ABL_MediaServerPort.GB28181RtpMinPort;

	//�����������ݽ���
	NetBaseThreadPool = new CNetBaseThreadPool(ABL_MediaServerPort.nRecvThreadCount);

	//¼��ط��̳߳�
	RecordReplayThreadPool = new CNetBaseThreadPool(ABL_MediaServerPort.nRecordReplayThread);

	//��Ϣ�����̳߳�
	MessageSendThreadPool = new CNetBaseThreadPool(6);

	//¼���ѯ�̳߳�
	HttpProcessThreadPool = new CNetBaseThreadPool(ABL_nCurrentSystemCpuCount);
 
	int nRet = -1 ;
	if (!ABL_bInitXHNetSDKFlag) //��ֻ֤��ʼ��1��
	{
 		nRet = XHNetSDK_Init(ABL_MediaServerPort.nRecvThreadCount, 1);
	  
	    ABL_bInitXHNetSDKFlag = true;
		WriteLog(Log_Debug, "Network Init = %d \r\n", nRet);
	}
	  
	nBindHttp = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHttpServerPort, &srvhandle_8080, onaccept, onread, onclose, true, ABL_MediaServerPort.nHttpServerPort % 2 == 1 ? true : false);
	nBindRtsp = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nRtspPort, &srvhandle_554, onaccept, onread, onclose,true, ABL_MediaServerPort.nRtspPort % 2 == 1 ? true : false);
	nBindRtmp = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nRtmpPort, &srvhandle_1935, onaccept, onread, onclose,true, ABL_MediaServerPort.nRtmpPort % 2 == 1 ? true : false);
	nBindWsFlv = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nWSFlvPort, &srvhandle_6088, onaccept, onread, onclose, true, ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? true : false );
	nBindHttpFlv = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHttpFlvPort, &srvhandle_8088, onaccept, onread, onclose, true, ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? true : false );
	nBindHls = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHlsPort, &srvhandle_9088, onaccept, onread, onclose, true, ABL_MediaServerPort.nHlsPort % 2 == 1 ? true : false);
	nBindMp4 = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nHttpMp4Port, &srvhandle_8089, onaccept, onread, onclose, true, ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? true : false );
	nBindRecvAudio = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.WsRecvPcmPort, &srvhandle_9298, onaccept, onread, onclose, true, ABL_MediaServerPort.WsRecvPcmPort % 2 == 1 ? true : false);
	nBingPS10000 = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.ps_tsRecvPort, &srvhandle_10000, onaccept, onread, onclose, true);
	nBind1078 = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.n1078Port, &srvhandle_1078, onaccept, onread, onclose, true);
	nBind8192 = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_MediaServerPort.nWebRtcPort, &srvhandle_8192, onaccept, onread, onclose, true, ABL_MediaServerPort.nWebRtcPort % 2 == 1 ? true : false);
 	
	WriteLog(Log_Debug, (nBindHttp == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success) ":"�󶨶˿� [ %s ] %d ʧ��(fail)����Ҫ�޸ĸö˿ں�  ", ABL_MediaServerPort.nHttpServerPort % 2 == 1 ? "https" : "http", ABL_MediaServerPort.nHttpServerPort);
	WriteLog(Log_Debug, (nBindRtsp == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success)  " : "�󶨶˿� [ %s ] %d ʧ��(fail) ����Ҫ�޸ĸö˿ں�", ABL_MediaServerPort.nRtspPort % 2 == 1 ? "rtsps" : "rtsp", ABL_MediaServerPort.nRtspPort);
	WriteLog(Log_Debug, (nBindRtmp == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success)  " : "�󶨶˿� [ %s ] %d ʧ��(fail)����Ҫ�޸ĸö˿ں� ", ABL_MediaServerPort.nRtmpPort % 2 == 1 ? "rtmps" : "rtmp", ABL_MediaServerPort.nRtmpPort);
	WriteLog(Log_Debug, (nBindWsFlv == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success)  " : "�󶨶˿� [ %s ] %d ʧ��(fail)����Ҫ�޸ĸö˿ں� ", ABL_MediaServerPort.nWSFlvPort % 2 == 1 ? "wss-flv" : "ws-flv", ABL_MediaServerPort.nWSFlvPort);
	WriteLog(Log_Debug, (nBindHttpFlv == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success)  " : "�󶨶˿� [ %s ] %d ʧ��(fail)����Ҫ�޸ĸö˿ں� ", ABL_MediaServerPort.nHttpFlvPort % 2 == 1 ? "https-flv" : "http-flv", ABL_MediaServerPort.nHttpFlvPort);
	WriteLog(Log_Debug, (nBindHls == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success)  " : "�󶨶˿� [ %s ] %d ʧ��(fail) ����Ҫ�޸ĸö˿ں�", ABL_MediaServerPort.nHlsPort % 2 == 1 ? "https-hls" : "http-hls", ABL_MediaServerPort.nHlsPort);
	WriteLog(Log_Debug, (nBindMp4 == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success)  " : "�󶨶˿� [ %s ] %d ʧ��(fail)����Ҫ�޸ĸö˿ں� ", ABL_MediaServerPort.nHttpMp4Port % 2 == 1 ? "https-mp4" : "http-mp4", ABL_MediaServerPort.nHttpMp4Port);
	WriteLog(Log_Debug, (nBindRecvAudio == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success)  " : "�󶨶˿� [ %s ] %d ʧ��(fail) ����Ҫ�޸ĸö˿ں�", ABL_MediaServerPort.WsRecvPcmPort % 2 == 1 ? "wss-recvpcm" : "ws-recvpcm", ABL_MediaServerPort.WsRecvPcmPort);
	WriteLog(Log_Debug, (nBingPS10000 == 0) ? "�󶨶˿� [ tcp ] %d  �ɹ�(success)  " : "�󶨶˿� %d(tcp) ʧ��(fail)����Ҫ�޸ĸö˿ں� ", ABL_MediaServerPort.ps_tsRecvPort);
	WriteLog(Log_Debug, (nBind1078 == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success) " : "�󶨶˿� [ %s ] %d ʧ��(fail)����Ҫ�޸ĸö˿ں�  ", ABL_MediaServerPort.n1078Port % 2 == 1 ? "rtps_1078" : "rtp_1078", ABL_MediaServerPort.n1078Port);
	WriteLog(Log_Debug, (nBind8192 == 0) ? "�󶨶˿� [ %s ] %d �ɹ�(success) " : "�󶨶˿� [ %s ] %d ʧ��(fail)����Ҫ�޸ĸö˿ں�  ", nBind8192 % 2 == 1 ? "http-webrtc" : "https-webrtc", ABL_MediaServerPort.nWebRtcPort);

	alaw_pcm16_tableinit();
	ulaw_pcm16_tableinit();

	ABL_MediaServerPort.nServerStartTime = GetCurrentSecond();
	ABL_MediaServerPort.nServerKeepaliveTime = GetTickCount64();
 
#if  0 //���� hls �ͻ���  http://190.168.24.112:8082/live/Camera_00001.m3u8  \   http://190.15.240.36:9088/Media/Camera_00001.m3u8 \ http://190.15.240.36:9088/Media/Camera_00001/hls.m3u8
	//CreateNetRevcBaseClient(0, 0, "http://190.168.24.112:8082/live/Camera_00001.m3u8", 0);
	CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy,0, 0, "http://190.15.240.36:9088/Media/Camera_00001.m3u8", 0,"/Media/Camera_00002");
#endif
#if   0 //���� rtmp �ͻ���  rtmp://10.0.0.239:1936/Media/Camera_00001  \   rtmp://10.0.0.239:1936/Media/Camera_00001
	CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy,0, 0, "rtmp://190.15.240.36:1935/Media/Camera_00001", 0, "/Media/Camera_00002");
#endif
#if   0 //���� flv  http://190.15.240.36:8088/Media/Camera_00001.flv
	CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy,0, 0, "http://190.15.240.36:8088/Media/Camera_00001.flv", 0, "/Media/Camera_00002");
#endif
#if   0 //���� rtmp �ͻ���  rtmp://10.0.0.239:1936/Media/Camera_00001  \   rtmp://10.0.0.239:1936/Media/Camera_00001
	CNetRevcBase_ptr rtmpClient = CreateNetRevcBaseClient(NetRevcBaseClient_addPushStreamProxy,0, 0, "rtmp://190.15.240.36:1935/Media/Camera_00001", 0,"/Media/Camera_00001");
	nTestRtmpPushID = rtmpClient->nClient;
#endif
#if  0 //���� rtmp �ͻ���  rtmp://10.0.0.239:1936/Media/Camera_00001  \   rtmp://10.0.0.239:1936/Media/Camera_00001
	CNetRevcBase_ptr rtmpClient = CreateNetRevcBaseClient(NetRevcBaseClient_addStreamProxy,0, 0, "rtsp://admin:abldyjh2020@192.168.1.120:554", 0,"/Media/Camera_00001");
	nTestRtmpPushID = rtmpClient->nClient;
#endif
#if   0 //���� rtsp �ͻ���  
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
#if   0//������Ϣ֪ͨ  http://10.0.0.238:7088/index/hook/on_stream_none_reader
	 CreateNetRevcBaseClient(NetBaseNetType_HttpClient_None_reader, 0, 0, "http://10.0.0.238:7088/index/hook/on_stream_none_reader", 0, "");
#endif

	 //�������˿ڹ������ 
	 CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RecvRtpPS_TS, 0, 0, "", 0, "");

	//����ҵ�����߳�
#ifdef  OS_System_Windows
	unsigned long dwThread,dwThread2;
	::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ABLMedisServerProcessThread, (LPVOID)NULL, 0, &dwThread);
	::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ABLMedisServerFastDeleteThread, (LPVOID)NULL, 0, &dwThread2);
#else
	pthread_t  hMedisServerProcessThread;
	pthread_create(&hMedisServerProcessThread, NULL, ABLMedisServerProcessThread, (void*)NULL);
	pthread_t  hMedisServerProcessThread2;
	pthread_create(&hMedisServerProcessThread2, NULL, ABLMedisServerFastDeleteThread, (void*)NULL);
#endif

	while (ABL_bMediaServerRunFlag)
	{
		//����������֪ͨ 
		if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.bNoticeStartEvent == false )
		{
			if ((GetCurrentSecond() - ABL_MediaServerPort.nServerStartTime) > 6 && (GetCurrentSecond() - ABL_MediaServerPort.nServerStartTime) <= 15)
			{
				ABL_MediaServerPort.bNoticeStartEvent = true;
				MessageNoticeStruct msgNotice;
				msgNotice.nClient = NetBaseNetType_HttpClient_ServerStarted;

#ifdef OS_System_Windows
				string strRecordPath = ABL_MediaServerPort.recordPath;
#ifdef USE_BOOST

				replace_all(strRecordPath, "\\", "/");
				string serverRunPath = ABL_MediaSeverRunPath;
				replace_all(serverRunPath, "\\", "/");
#else
				ABL::replace_all(strRecordPath, "\\", "/");
				string serverRunPath = ABL_MediaSeverRunPath;
				ABL::replace_all(serverRunPath, "\\", "/");
#endif
		
				SYSTEMTIME st;
				GetLocalTime(&st);
				sprintf(msgNotice.szMsg, "{\"eventName\":\"on_server_started\",\"localipAddress\":\"%s\",\"httpServerPort\":%d,\"recordPath\":\"%s\",\"mediaServerId\":\"%s\",\"serverRunPath\":\"%s\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpServerPort,strRecordPath.c_str(), ABL_MediaServerPort.mediaServerID, serverRunPath.c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
				time_t now;
				time(&now);
				struct tm *local;
				local = localtime(&now);
				sprintf(msgNotice.szMsg, "{\"eventName\":\"on_server_started\",\"localipAddress\":\"%s\",\"httpServerPort\":%d,\"recordPath\":\"%s\",\"mediaServerId\":\"%s\",\"serverRunPath\":\"%s/\",\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpServerPort,ABL_MediaServerPort.recordPath, ABL_MediaServerPort.mediaServerID, ABL_MediaSeverRunPath, local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
#endif
				pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
			}
		}
		//Sleep(1000);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
 	}
 
	ABL_bMediaServerRunFlag = false;
	while (!ABL_bExitMediaServerRunFlag)
	//	Sleep(100);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
	XHNetSDK_Unlisten(srvhandle_8080);
	XHNetSDK_Unlisten(srvhandle_554);
	XHNetSDK_Unlisten(srvhandle_1935);
	XHNetSDK_Unlisten(srvhandle_8088);
	XHNetSDK_Unlisten(srvhandle_6088);
	XHNetSDK_Unlisten(srvhandle_8089);
	XHNetSDK_Unlisten(srvhandle_9088);
	XHNetSDK_Unlisten(srvhandle_9298);
	XHNetSDK_Unlisten(srvhandle_10000);
	XHNetSDK_Unlisten(srvhandle_1078);
	XHNetSDK_Unlisten(srvhandle_8192);
	
	delete NetBaseThreadPool;
	NetBaseThreadPool = NULL;

	delete RecordReplayThreadPool;
	RecordReplayThreadPool = NULL;

	delete MessageSendThreadPool;
	MessageSendThreadPool = NULL;

	delete HttpProcessThreadPool;
	HttpProcessThreadPool = NULL;

	pDisconnectBaseNetFifo.FreeFifo();
	pReConnectStreamProxyFifo.FreeFifo();
	pMessageNoticeFifo.FreeFifo();
	pNetBaseObjectFifo.FreeFifo();
	pDisconnectMediaSource.FreeFifo();

	xh_ABLRecordFileSourceMap.clear();
	xh_ABLPictureFileSourceMap.clear();

#ifdef OS_System_Windows
	CloseHandle(hRunAsOne);
	StopLogFile();
#else
	ExitLogFile();
#endif

	WriteLog(Log_Debug, "--------------------ABLMediaServer End .... --------------------");

	malloc_trim(0);
	
	ABL_bMediaServerRunFlag = true;
	if (ABL_bRestartServerFlag)
	{
		ABL_MediaServerPort.nServerStartTime = GetCurrentSecond();
		ABL_MediaServerPort.nServerKeepaliveTime = GetTickCount64();
		ABL_MediaServerPort.bNoticeStartEvent = false;
		ABL_bRestartServerFlag = false ;
		memset((char*)&ABL_MediaServerPort, 0x00, sizeof(ABL_MediaServerPort));//�������ļ�ȫ����������¶�ȡ 
	    goto ABL_Restart;
	}
	XHNetSDK_Deinit();
	
	//cuda Ӳ�����룬������Դ�ͷ� 
#ifdef OS_System_Windows
	if (ABL_bInitCudaSDKFlag)
		cudaCodec_UnInit();
#else
	if(ABL_bInitCudaSDKFlag && pCudaDecodeHandle != NULL && pCudaEncodeHandle != NULL)
	{
	 cudaCodec_UnInit();
     cudaEncode_UnInit();
	 
	 dlclose(pCudaDecodeHandle);
	 dlclose(pCudaEncodeHandle);
	}
	
	//����srtp 
	//srtp_shutdown();
#endif

	return 0;
}

