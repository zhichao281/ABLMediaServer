/*
���ܣ�
    ʵ��http����������Ӧ�ͻ��˵ĸ��ֲ������� 
	 
����    2021-07-25
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerHTTP.h"
#include "rapidjson/prettywriter.h"
#ifdef USE_BOOST

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSourceNoLock(char* szURL);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSourceByClientID(uint64_t nClient);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern bool                                  QueryMediaSource(char* pushURL);
extern int                                   GetPushRtspClientToJson(char* szMediaSourceInfo);

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern MediaServerPort                       ABL_MediaServerPort;
extern int                                   GetAllMediaStreamSource(char* szMediaSourceInfo, getMediaListStruct mediaListStruct);
extern int                                   GetAllOutList(char* szMediaSourceInfo, char* szOutType);
extern int                                   CloseMediaStreamSource(closeStreamsStruct closeStruct);
extern boost::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
extern int                                   queryRecordListByTime(char* szMediaSourceInfo, queryRecordListStruct queryStruct, char* szOutRecordPlayURL);
extern uint64_t                              GetCurrentSecond();
extern uint64_t                              GetCurrentSecondByTime(char* szDateTime);
extern int                                   queryPictureListByTime(char* szMediaSourceInfo, queryPictureListStruct queryStruct);
extern boost::shared_ptr<CPictureFileSource> GetPictureFileSource(char* szShareURL, bool bLock);
extern int  GetNetRevcBaseClientCountByNetType(NetBaseNetType netType, bool bLockFlag);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientByNetTypeShareMediaURL(NetBaseNetType netType, char* ShareMediaURL, bool bLockFlag);
extern bool                                  CheckPortAlreadyUsed(int nPort, int nPortType, bool bLockFlag);
extern bool                                  CheckSSRCAlreadyUsed(int nSSRC, bool bLockFlag);
extern bool                                  CheckAppStreamExisting(char* szAppStreamURL);
extern bool                                  CheckDst_url_portAlreadyUsed(char* dst_url, int dst_port, bool bLockFlag);
extern volatile bool                         ABL_bMediaServerRunFlag;
extern volatile bool                         ABL_bRestartServerFlag;
extern char                                  ABL_MediaSeverRunPath[256];
extern char                                  ABL_wwwMediaPath[256];
extern int                                   GetALLListServerPort(char* szMediaSourceInfo, ListServerPortStruct  listServerPortStruct);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle, NETHANDLE clihandle);
extern void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle, NETHANDLE clihandle, void* address);
extern void LIBNET_CALLMETHOD   onread(NETHANDLE srvhandle, NETHANDLE clihandle, uint8_t* data, uint32_t datasize, void* address);
extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle, uint8_t result, uint16_t nLocalPort);
extern unsigned short                         ABL_nGB28181Port;
extern char                                   ABL_szLocalIP[128];
extern char                                   szConfigFileName[512];
extern uint32_t                               ABL_nSSRC;

extern  CSimpleIniA                           ABL_ConfigFile;
#else

extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSourceNoLock(char* szURL);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSourceByClientID(uint64_t nClient);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL, bool bLock = true);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
extern bool                                  QueryMediaSource(char* pushURL);
extern int                                   GetPushRtspClientToJson(char* szMediaSourceInfo);

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern MediaServerPort                       ABL_MediaServerPort;
extern int                                   GetAllMediaStreamSource(char* szMediaSourceInfo, getMediaListStruct mediaListStruct);
extern int                                   GetAllOutList(char* szMediaSourceInfo, char* szOutType);
extern int                                   CloseMediaStreamSource(closeStreamsStruct closeStruct);
extern std::shared_ptr<CRecordFileSource>  GetRecordFileSource(char* szShareURL);
extern int                                   queryRecordListByTime(char* szMediaSourceInfo, queryRecordListStruct queryStruct, char* szOutRecordPlayURL);
extern uint64_t                              GetCurrentSecond();
extern uint64_t                              GetCurrentSecondByTime(char* szDateTime);
extern int                                   queryPictureListByTime(char* szMediaSourceInfo, queryPictureListStruct queryStruct);
extern std::shared_ptr<CPictureFileSource> GetPictureFileSource(char* szShareURL, bool bLock);
extern int  GetNetRevcBaseClientCountByNetType(NetBaseNetType netType, bool bLockFlag);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClientByNetTypeShareMediaURL(NetBaseNetType netType, char* ShareMediaURL, bool bLockFlag);
extern bool                                  CheckPortAlreadyUsed(int nPort, int nPortType, bool bLockFlag);
extern bool                                  CheckSSRCAlreadyUsed(int nSSRC, bool bLockFlag);
extern bool                                  CheckAppStreamExisting(char* szAppStreamURL);
extern bool                                  CheckDst_url_portAlreadyUsed(char* dst_url, int dst_port, bool bLockFlag);
extern volatile bool                         ABL_bMediaServerRunFlag;
extern volatile bool                         ABL_bRestartServerFlag;
extern char                                  ABL_MediaSeverRunPath[256];
extern char                                  ABL_wwwMediaPath[256];
extern int                                   GetALLListServerPort(char* szMediaSourceInfo, ListServerPortStruct  listServerPortStruct);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle, NETHANDLE clihandle);
extern void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle, NETHANDLE clihandle, void* address);
extern void LIBNET_CALLMETHOD   onread(NETHANDLE srvhandle, NETHANDLE clihandle, uint8_t* data, uint32_t datasize, void* address);
extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle, uint8_t result, uint16_t nLocalPort);
extern unsigned short                         ABL_nGB28181Port;
extern char                                   ABL_szLocalIP[128];
extern char                                   szConfigFileName[512];
extern uint32_t                               ABL_nSSRC;

extern  CSimpleIniA                           ABL_ConfigFile;
#endif
#ifndef  OS_System_Windows
extern void  ABL_SetPathAuthority(char* szPath);
#endif 

CNetServerHTTP::CNetServerHTTP(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	memset(request_uuid, 0x00, sizeof(request_uuid));
	memset(szConnection, 0x00, sizeof(szConnection));
	strcpy(szConnection, "Close");
	netBaseNetType = NetBaseNetType_NetServerHTTP;
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	nNetStart = nNetEnd = netDataCacheLength = 0;
	strcpy(m_szShareMediaURL, szShareMediaURL);

	WriteLog(Log_Debug, "CNetServerHTTP ���� = %X  nClient = %llu ",this, nClient);
}

CNetServerHTTP::~CNetServerHTTP()
{
	bRunFlag.exchange(false);
	std::lock_guard<std::mutex> lock(NetServerHTTPLock);

	DeleteAllHttpKeyValue();
	WriteLog(Log_Debug, "CNetServerHTTP ���� = %X  nClient = %llu \r\n", this, nClient);
	malloc_trim(0);
}

int CNetServerHTTP::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
 	return 0;
}

int CNetServerHTTP::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	return 0;
}
int CNetServerHTTP::SendVideo()
{
	return 0;
}

int CNetServerHTTP::SendAudio()
{
	return 0;
}

int CNetServerHTTP::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	nRecvDataTimerBySecond = 0;
	std::lock_guard<std::mutex> lock(NetServerHTTPLock);
	if (!bRunFlag.load())
		return -1;

	if (MaxNetServerHttpBuffer - nNetEnd >= nDataLength)
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

			//�ѿ����buffer��� 
			memset(netDataCache + nNetEnd, 0x00, MaxNetServerHttpBuffer - nNetEnd); 

			if (MaxNetServerHttpBuffer - nNetEnd < nDataLength)
			{
				bRunFlag.exchange(false);
				nNetStart = nNetEnd = netDataCacheLength = 0;
				memset(netDataCache, 0x00, MaxNetServerHttpBuffer);
				WriteLog(Log_Debug, "CNetServerHTTP = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return 0;
			}
		}
		else
		{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
			nNetStart = nNetEnd = netDataCacheLength = 0;
			memset(netDataCache, 0x00, MaxNetServerHttpBuffer);
 		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}

	return 0;
}

//���httpͷ�Ƿ��������
int   CNetServerHTTP::CheckHttpHeadEnd()
{
	int       nHttpHeadEndPos = -1;
	unsigned char szHttpHeadEnd[4] = {0x0d,0x0a,0x0d,0x0a};
	for (int i = nNetStart; i < netDataCacheLength; i++)
	{
		if (memcmp(netDataCache + i, szHttpHeadEnd, 4) == 0)
		{
			nHttpHeadEndPos = i;
			break;
		}
	}
	return nHttpHeadEndPos;
}

int CNetServerHTTP::ProcessNetData()
{
	std::lock_guard<std::mutex> lock(NetServerHTTPLock);
	if (!bRunFlag.load())
		return -1;

	if (netDataCacheLength > 4096 )
	{
		WriteLog(Log_Debug, "CNetServerHTTP = %X , nClient = %llu ,netDataCacheLength = %d, ���͹�����url���ݳ��ȷǷ� ,����ɾ�� ", this, nClient, netDataCacheLength);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1;
	}

	int nHttpHeadEndPos = CheckHttpHeadEnd();

	if ( !(memcmp(netDataCache, "GET ", 4) == 0 || memcmp(netDataCache, "POST ", 5) == 0))
	{
		WriteLog(Log_Debug, "CNetServerHTTP = %X , nClient = %llu , ���յ����ݷǷ� ", this, nClient);
        sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Not find GET or POST method in body content . \",\"key\":%d}", IndexApiCode_HttpProtocolError, 0);
		ResponseSuccess(szResponseBody);
		bRunFlag.exchange(false);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return -1 ;
	}

	if (nHttpHeadEndPos < 0 )
		return -1;

	//�������һ������ʱ��
	bResponseHttpFlag = false;
	nCreateDateTime = GetTickCount64();

	memset(szHttpHead, 0x00, sizeof(szHttpHead));
	memcpy(szHttpHead, netDataCache + nNetStart, nHttpHeadEndPos - nNetStart + 4 );
	httpParse.ParseSipString(szHttpHead);

	nContent_Length = 0;
	memset(szContentLength, 0x00, sizeof(szContentLength));
	if (httpParse.GetFieldValue("Content-Length", szContentLength))
	{//POST 
		nContent_Length = atoi(szContentLength);

		if (nContent_Length <= 0)
		{//���Ϊpost ��ʽ��������дbody����
 			WriteLog(Log_Debug, "CNetServerHTTP =%X Э�����1 nClient = %llu ", this, nClient);
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"body content Not Found \",\"key\":%d}", IndexApiCode_HttpProtocolError, 0);
			ResponseSuccess(szResponseBody);

			//ȫ�����
			nNetStart = nNetEnd = netDataCacheLength = 0;
			memset(netDataCache, 0x00, MaxNetServerHttpBuffer);
			return -1;
 		}
 
		if (netDataCacheLength - (nHttpHeadEndPos + 4) < nContent_Length  )
 			return -2;//������δ�������� 

		memset(szHttpPath, 0x00, sizeof(szHttpPath));
		if (httpParse.GetFieldValue("POST", szHttpPath) == false)
		{
			WriteLog(Log_Debug, "CNetServerHTTP =%X Э�����2 nClient = %llu ", this, nClient);
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Http Protocol Error \",\"key\":%d}", IndexApiCode_HttpProtocolError, 0);
			ResponseSuccess(szResponseBody);
			
			//ȫ�����
			nNetStart = nNetEnd = netDataCacheLength = 0;
			memset(netDataCache, 0x00, MaxNetServerHttpBuffer);
			return -3;
		}

		//ȥ��HTTP 1.1 
		string strHttpPath = szHttpPath;
		int    nPos;
		nPos = strHttpPath.find(" HTTP", 0);
		if (nPos > 0)
			szHttpPath[nPos] = 0x00;

		//ȥ�����ź���Ĳ���
		strHttpPath = szHttpPath;
		nPos = strHttpPath.find("?", 0);
		if (nPos > 0)
			szHttpPath[nPos] = 0x00;

		if (nContent_Length > 1024 * 64)
		{
			WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu , nContent_Length = %d ���ȳ��� 64K  ", this, nClient, nContent_Length);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return -4;
		}

		memset(szHttpBody, 0x00, sizeof(szHttpBody));
		memcpy(szHttpBody, netDataCache + nHttpHeadEndPos + 4, nContent_Length);

		ResponseHttpRequest("POST", szHttpPath, szHttpBody);
  	}
	else
	{
		memset(szHttpPath, 0x00, sizeof(szHttpPath));
		memset(szHttpBody, 0x00, sizeof(szHttpBody));
		
		if (httpParse.GetFieldValue("GET", szHttpPath) == false)
		{
			WriteLog(Log_Debug, "CNetServerHTTP =%X Э�����3 nClient = %llu ", this, nClient);
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Http Protocol Error \",\"key\":%d}", IndexApiCode_HttpProtocolError, 0);
			ResponseSuccess(szResponseBody);

 			//ȫ����� 
			nNetStart = nNetEnd = netDataCacheLength = 0;
			memset(netDataCache, 0x00, MaxNetServerHttpBuffer);
			return -4;
		}
		//ȥ��HTTP 1.1 
		string strHttpPath = szHttpPath;
		int    nPos;
		nPos = strHttpPath.find(" HTTP", 0);
		if (nPos > 0)
			szHttpPath[nPos] = 0x00;

		//ȥ������,��?����buffer����Ϊ����
		strHttpPath = szHttpPath;
		nPos = strHttpPath.find("?", 0);
		if (nPos > 0)
		{
			memcpy(szHttpBody, szHttpPath + nPos + 1, strlen(szHttpPath) - nPos - 1);
			szHttpPath[nPos] = 0x00;
		}
 
		ResponseHttpRequest("GET", szHttpPath, szHttpBody);
	}

	//��ʣ���buff��ǰ�ƶ� 
    netDataCacheLength = netDataCacheLength - (nHttpHeadEndPos + 4 + nContent_Length);
	if (netDataCacheLength > 0)
	{
		memmove(netDataCache, netDataCache + (nHttpHeadEndPos + 4 + nContent_Length), netDataCacheLength);
		nNetStart = 0;
		nNetEnd = netDataCacheLength;

		//��ʣ��Ŀռ���� 
		memset(netDataCache + nNetEnd , 0x00, MaxNetServerHttpBuffer - nNetEnd );
	}
	else
	{
		nNetStart = nNetEnd = netDataCacheLength = 0;

		//ȫ����� 
		memset(netDataCache, 0x00, MaxNetServerHttpBuffer);
	}
 
	return 0;
}

//ɾ�����в���
void CNetServerHTTP::DeleteAllHttpKeyValue()
{
	RequestKeyValue* pDel;
	RequestKeyValueMap::iterator it;

	for (it = requestKeyValueMap.begin(); it != requestKeyValueMap.end();)
	{
		pDel = (*it).second;
		delete pDel;
		requestKeyValueMap.erase(it++);
	}
}

bool  CNetServerHTTP::GetKeyValue(char* key, char* value)
{
	RequestKeyValue* pFind;
	RequestKeyValueMap::iterator it;

	it = requestKeyValueMap.find(key);
	if (it != requestKeyValueMap.end())
	{
		pFind = (*it).second;
		strcpy(value, pFind->value);
		return true;
	}
	else
		return false;
}

//��get ��ʽ�Ĳ��������и�
bool CNetServerHTTP::SplitterTextParam(char* szTextParam)
{//id=1000&name=luoshenzhen&address=�㶫����&age=45
	string strTextParam = szTextParam;
	int    nPos1=0, nPos2=0;
	int    nFind1,nFind2;
	int    nKeyCount = 0;
	char   szValue[string_length_2048] = { 0 };
	char   szKey[string_length_2048] = { 0 };
	char   szBlockString[string_length_8192] = { 0 };

	DeleteAllHttpKeyValue();
	string strValue;
	string strKey;
	string strBlockString;
	bool   breakFlag = false ;  

	while (true)
	{
		nFind1 = strTextParam.find("&", nPos1);
		memset(szBlockString, 0x00, sizeof(szBlockString));
		if (nFind1 > 0)
		{
			if (nFind1 - nPos1 > 0 &&  nFind1 - nPos1 < string_length_8192 )
			{
				memcpy(szBlockString , szTextParam + nPos1, nFind1 - nPos1);
				strBlockString = szBlockString;
 			}
			nPos1 = nFind1+1;
		}
		else
		{
			if ((strlen(szTextParam) - nPos1) > 0 && (strlen(szTextParam) - nPos1) < string_length_8192)
			{
				memcpy(szBlockString , szTextParam+ nPos1, strlen(szTextParam) - nPos1);
				strBlockString = szBlockString;
			}
			breakFlag = true ;
		}

		if (strlen(szBlockString) > 0)
		{//�ҳ��ֶ�
	
#ifdef USE_BOOST
			replace_all(strBlockString, "%20", "");//ȥ���ո�
			replace_all(strBlockString, "%26", "&");
#else
			ABL::replace_all(strBlockString, "%20", "");//ȥ���ո�
			ABL::replace_all(strBlockString, "%26", "&");			
#endif
			memset(szBlockString, 0x00, sizeof(szBlockString));
			strcpy(szBlockString, strBlockString.c_str());

			RequestKeyValue* keyValue = NULL;
			while(keyValue == NULL)
				keyValue = new RequestKeyValue();
			nFind2 = strBlockString.find("=", 0);
			if (nFind2 > 0)
			{
				memcpy(keyValue->key, szBlockString, nFind2);
				memcpy(keyValue->value, szBlockString + nFind2 + 1, strBlockString.size() - nFind2 - 1);
			}
			else
			{
				if(strlen(szBlockString) < 512 )
			   	  strcpy(keyValue->key, szBlockString);
 			}
			requestKeyValueMap.insert(RequestKeyValueMap::value_type(keyValue->key, keyValue));
			nKeyCount ++;
			WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu ��ȡHTTP�������  %s = %s", this, nClient, keyValue->key, keyValue->value);
		}
		if (breakFlag)
			break;
	} 

	GetKeyValue("request_uuid", request_uuid);
	return nKeyCount > 0 ? true : false;
}

//��json���ݽ����и�
bool CNetServerHTTP::SplitterJsonParam(char* szJsonParam)
{
	int    nKeyCount = 0;
	if (szJsonParam == NULL || strlen(szJsonParam) == 0)
		return false;

	char  szValue[string_length_2048] = { 0 };
	rapidjson::Type nType;

	DeleteAllHttpKeyValue();

	string strJson = szJsonParam;
	
#ifdef USE_BOOST
	boost::trim(strJson);


#else
	ABL::trim(strJson);
#endif
	strcpy(szJsonParam, strJson.c_str());

	if (szJsonParam[0] != '{' || szJsonParam[strlen(szJsonParam) - 1] != '}')
		return false;

	string strJsonTest = szJsonParam;
	Document docTest;
	docTest.Parse<0>(strJsonTest.c_str());
	 
	if (!docTest.HasParseError())
	{
		for (rapidjson::Value::ConstMemberIterator itr = docTest.MemberBegin(); itr != docTest.MemberEnd(); itr++)
		{
			Value jKey;
			Value jValue;
			Document::AllocatorType allocator;
			jKey.CopyFrom(itr->name, allocator);
			jValue.CopyFrom(itr->value, allocator);
			if (jKey.IsString())
			{
				string name = jKey.GetString();
			

#ifdef USE_BOOST
				boost::trim(name);
#else
				ABL::trim(name);
#endif

				nType = jValue.GetType();
 				if (nType == kStringType)
				{
				  string value = jValue.GetString();
 	#ifdef USE_BOOST
					boost::trim(value);
#else
					ABL::trim(value);
#endif
				  strcpy(szValue, value.c_str());
				}
				else if (nType == kNumberType)
				{
					//Mod by ZXT
					if(jValue.IsDouble())
						sprintf(szValue, "%.2f", jValue.GetFloat());
					else
						sprintf(szValue, "%llu", jValue.GetInt64());
				}

				RequestKeyValue* keyValue = NULL;
				while(keyValue == NULL )
					keyValue = new RequestKeyValue();
				strcpy(keyValue->key, name.c_str());
				strcpy(keyValue->value, szValue);
				requestKeyValueMap.insert(RequestKeyValueMap::value_type(keyValue->key, keyValue));

				WriteLog(Log_Debug, "CNetServerHTTP =%X  nClient = %llu ��ȡHTTP�������  %s = %s", this, nClient, keyValue->key, keyValue->value);
				nKeyCount ++;
			}
		}
	}
	GetKeyValue("request_uuid", request_uuid);
	return nKeyCount > 0 ? true : false;
}

//�ظ��ɹ���Ϣ
bool  CNetServerHTTP::ResponseSuccess(char* szSuccessInfo)
{
	std::lock_guard<std::mutex> lock(httpResponseLock);

	//���� resquet_uuid 
	InsertUUIDtoJson(szSuccessInfo, request_uuid);

	int nLength = strlen(szSuccessInfo);
	if(strlen(szConnection) == 0)
	  sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nAccess-Control-Allow-Methods: POST, GET, OPTIONS, DELETE\r\nAccess-Control-Max-Age: 3600\r\nAccess-Control-Allow-Headers: Content-Type,XFILENAME,XFILECATEGORY,XFILESIZE,x-requested-with,Authorization\r\nAccess-Control-Allow-Credentials: true\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, nLength);
	else
	 sprintf(szResponseHttpHead, "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: application/json;charset=utf-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: %s\r\nAccess-Control-Allow-Methods: POST, GET, OPTIONS, DELETE\r\nAccess-Control-Max-Age: 3600\r\nAccess-Control-Allow-Headers: Content-Type,XFILENAME,XFILECATEGORY,XFILESIZE,x-requested-with,Authorization\r\nAccess-Control-Allow-Credentials: true\r\nContent-Length: %d\r\n\r\n", MediaServerVerson, szConnection, nLength);
	XHNetSDK_Write(nClient, (unsigned char*)szResponseHttpHead, strlen(szResponseHttpHead), 1);

	int nPos = 0;
	int nWriteRet;
	int nSendErrorCount = 0;
	while (nLength > 0 && szSuccessInfo != NULL)
	{
		if (nLength > Send_ResponseHttp_MaxPacketCount)
		{
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)szSuccessInfo + nPos, Send_ResponseHttp_MaxPacketCount, 1);
			nLength -= Send_ResponseHttp_MaxPacketCount;
			nPos += Send_ResponseHttp_MaxPacketCount;
 		}
		else
		{
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)szSuccessInfo + nPos, nLength, 1);
			nPos += nLength;
			nLength = 0;
		}

		if (nWriteRet != 0)
		{//���ͳ���
			nSendErrorCount++;
			if (nSendErrorCount >= 5)
			{
				WriteLog(Log_Debug, "CNetServerHTTP = %X nClient = %llu ���ʹ������� %d �� ��׼��ɾ�� ", this, nClient, nSendErrorCount);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				break;
			}
		}
		else
			nSendErrorCount = 0;
	}

	if(nPos < 2048 )
	  WriteLog(Log_Debug, szSuccessInfo);
  
	return true;
}

//��Ӧhttp����
bool CNetServerHTTP::ResponseHttpRequest(char* szModem, char* httpURL, char* requestParam)
{
	if (!bRunFlag.load())
		return false;
	WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu  , szModem = %s ��httpURL = %s", this, nClient, szModem, httpURL);

	//url ����Ϸ��Լ��ж�,������һЩ���߰�������󷢹���  .php .asp .html .htm .jars .zip .rar .exe .tar .tar.gz .7z .dll .bat
	if (! ((strcmp(httpURL, "/index/api/addStreamProxy") == 0) || (strcmp(httpURL, "/index/api/delStreamProxy") == 0) || (strcmp(httpURL, "/index/api/delMediaStream") == 0) ||
		(strcmp(httpURL, "/index/api/addPushProxy") == 0) || (strcmp(httpURL, "/index/api/delPushProxy") == 0) || (strcmp(httpURL, "/index/api/openRtpServer") == 0) ||
		(strcmp(httpURL, "/index/api/closeRtpServer") == 0) || (strcmp(httpURL, "/index/api/startSendRtp") == 0) || (strcmp(httpURL, "/index/api/stopSendRtp") == 0) ||
		(strcmp(httpURL, "/index/api/getMediaList") == 0) || (strcmp(httpURL, "/index/api/getOutList") == 0) || (strcmp(httpURL, "/index/api/delOutList") == 0) ||
		(strcmp(httpURL, "/index/api/getServerConfig") == 0) || (strcmp(httpURL, "/index/api/close_streams") == 0) || (strcmp(httpURL, "/index/api/startRecord") == 0) ||
		(strcmp(httpURL, "/index/api/stopRecord") == 0) || (strcmp(httpURL, "/index/api/queryRecordList") == 0) || (strcmp(httpURL, "/index/api/getSnap") == 0) || (strstr(httpURL, ".jpg") != 0)  || //jpg ������Ҫ
		(strcmp(httpURL, "/index/hook/on_stream_none_reader") == 0 || strcmp(httpURL, "/index/hook/on_stream_not_found") == 0 || strcmp(httpURL, "/index/hook/on_record_mp4") == 0) || //�����¼��ظ�
		(strcmp(httpURL, "/index/api/queryPictureList") == 0) || (strcmp(httpURL, "/index/api/controlStreamProxy") == 0) || (strcmp(httpURL, "/index/api/setTransFilter") == 0) ||
		(strcmp(httpURL, "/index/api/setConfigParamValue") == 0) || (strcmp(httpURL, "/index/api/shutdownServer") == 0) || (strcmp(httpURL, "/index/api/restartServer") == 0) || 
		(strcmp(httpURL, "/stats/pushers") == 0 ) || (strcmp(httpURL, "/index/api/getTranscodingCount") == 0) || (strcmp(httpURL, "/index/api/listServerPort") == 0) || (strcmp(httpURL, "/index/api/setServerConfig") == 0) || 
		(strcmp(httpURL, "/index/api/pauseRtpServer") == 0) || (strcmp(httpURL, "/index/api/resumeRtpServer") == 0) || (strcmp(httpURL, "/index/api/addFFmpegProxy") == 0) || (strcmp(httpURL, "/index/api/delFFmpegProxy") == 0) ||
		(strcmp(httpURL, "/index/api/controlRecordPlay") == 0) || (strcmp(httpURL, "/index/api/sendJtt1078Talk") == 0))  )
	{ 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Http Request ( %s ) Not Supported \",\"key\":%d}", IndexApiCode_ErrorRequest, httpURL, 0);
		ResponseSuccess(szResponseBody);
		WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http ��������Ƿ� ��httpURL = %s", this, nClient, httpURL);
		bRunFlag.exchange(false);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return false ;
	}

	//��������Ƿ�Ϸ�  
	string strRequestParam = requestParam;
	int    nPos1, nPos2;
	nPos1 = strRequestParam.find("secret", 0);
	nPos2 = strRequestParam.find(ABL_MediaServerPort.secret, 0);
	if( !(nPos1 >= 0 && nPos2 >= nPos1 )  )
	{
		if (!(strstr(httpURL, ".jpg") != NULL || strstr(httpURL, ".mp4") != NULL ))
		{
			WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http ��������Ƿ�1 ��requestParam = %s", this, nClient, requestParam);
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret Error .\"}", IndexApiCode_secretError);
			ResponseSuccess(szResponseBody);
			bRunFlag.exchange(false);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
			return false;
 		}
	}
	
	//����� request_uuid 
	memset(request_uuid, 0x00, sizeof(request_uuid));

	if (strcmp(szModem, "GET") == 0)
	{
		nPos1 = strRequestParam.find("secret=", 0);
 		if (nPos1 < 0)
		{
			if (!(strstr(httpURL, ".jpg") != NULL || strstr(httpURL, ".mp4") != NULL))
			{
				WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http ��������Ƿ�2 ��requestParam = %s", this, nClient, requestParam);
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret Error .\"}", IndexApiCode_secretError);
				ResponseSuccess(szResponseBody);
				bRunFlag.exchange(false);
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
				return false;
			}
		}
		SplitterTextParam(requestParam);
	}
	else if (strcmp(szModem, "POST") == 0)
	{
		memset(szContentType, 0x00, sizeof(szContentType));
 		httpParse.GetFieldValue("Content-Type", szContentType);
		if (strcmp(szContentType, "application/json") == 0 || strcmp(szContentType, "application/json; charset=utf-8") == 0)
		{//json ��ʽ
			if (!SplitterJsonParam(requestParam))
			{
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"json error\",\"key\":%d}", IndexApiCode_HttpJsonError,  0);
				ResponseSuccess(szResponseBody); 
				WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http json %s ���� ��httpURL = %s", this, nClient, requestParam, httpURL);
				return false;
			}
		}
		else if (strcmp(szContentType, "application/x-www-form-urlencoded") == 0 || strcmp(szContentType, "application/x-www-form-urlencoded; charset=utf-8") == 0)
		{//
 			if (!SplitterTextParam(requestParam))
			{
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"json error\",\"key\":%d}", IndexApiCode_HttpJsonError, 0);
				ResponseSuccess(szResponseBody);
				WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http x-www-form-urlencoded %s ���� ��httpURL = %s", this, nClient, requestParam, httpURL);
				return false;
			}
		}
		else
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"IndexApiCode_ContentTypeNotSupported , ContentType: %s\"}", IndexApiCode_ContentTypeNotSupported, szContentType);
			ResponseSuccess(szResponseBody);
			WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu IndexApiCode_ContentTypeNotSupported , ContentType: %s ��httpURL = %s", this, nClient, szContentType, httpURL);
			return false;
		}
 	}
	else
	{
		WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http ��������Ƿ� ��httpURL = %s", this, nClient, httpURL);
		bRunFlag.exchange(false);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return false;
	}

	//����Connection ��ֵ�����Ƿ�ر� 
	memset(szConnection, 0x00, sizeof(szConnection));
	httpParse.GetFieldValue("Connection", szConnection);

	if (strcmp(httpURL, "/index/api/addStreamProxy") == 0)
	{//���������������(rtsp,rtmp������mp4�ļ���
		index_api_addStreamProxy(NetRevcBaseClient_addStreamProxyControl);
 	}else if (strcmp(httpURL, "/index/api/addFFmpegProxy") == 0)
	{//����ffmepg����ʵ�ִ���������rtmp,flv ,hls,http-mp4��
		index_api_addStreamProxy(NetRevcBaseClient_addFFmpegProxyControl);
	}
	else if (strcmp(httpURL, "/index/api/delStreamProxy") == 0 || strcmp(httpURL, "/index/api/delFFmpegProxy") == 0)
	{//ɾ����������
		index_api_delRequest();
	}
	else if (strcmp(httpURL, "/index/api/delMediaStream") == 0)
	{//ɾ��ý��Դ
		index_api_delRequest();
	}
	else if (strcmp(httpURL, "/index/api/addPushProxy") == 0)
	{//������������
		index_api_addPushProxy();
	}
	else if (strcmp(httpURL, "/index/api/delPushProxy") == 0)
	{//ɾ����������
		index_api_delRequest();
	}
	else if (strcmp(httpURL, "/index/api/openRtpServer") == 0)
	{//����GB28181��������
		index_api_openRtpServer();
	}
	else if (strcmp(httpURL, "/index/api/closeRtpServer") == 0)
	{//ɾ��GB28181��������
		index_api_delRequest();
	}
	else if (strcmp(httpURL, "/index/api/startSendRtp") == 0)
	{//����GB28181��������
		index_api_startSendRtp();
	}
	else if (strcmp(httpURL, "/index/api/stopSendRtp") == 0)
	{//ɾ��GB28181��������
		index_api_delRequest();
	}
	else if (strcmp(httpURL, "/index/api/getMediaList") == 0)
	{//�����б�
		index_api_getMediaList();
	}
	else if (strcmp(httpURL, "/index/api/getOutList") == 0)
	{//�������ⷢ���б�
		index_api_getOutList();
	}
	else if (strcmp(httpURL, "/index/api/delOutList") == 0)
	{//ɾ�����ⷢ�����б�
		index_api_delRequest();
	}
	else if (strcmp(httpURL, "/index/api/getServerConfig") == 0)
	{//��ȡϵͳ���ò���  
		index_api_getServerConfig();
	}
	else if (strcmp(httpURL, "/index/api/close_streams") == 0)
	{//����app��stream ��ɾ��ý��Դ   
		index_api_close_streams();
	}
	else if (strcmp(httpURL, "/index/api/startRecord") == 0 )
	{//��ʼ¼��
		index_api_startStopRecord(true);
	}
	else if (strcmp(httpURL, "/index/api/stopRecord") == 0)
	{//ֹͣ¼��
		index_api_startStopRecord(false);
	}
	else if (strcmp(httpURL, "/index/api/queryRecordList") == 0)
	{//��ѯ¼���б�
		index_api_queryRecordList();
	}
	else if (strcmp(httpURL, "/index/api/sendJtt1078Talk") == 0)
	{//����1078�����Խ�
		index_api_sendJtt1078Talk();
	}
	else if (strcmp(httpURL, "/index/hook/on_stream_none_reader") == 0 || strcmp(httpURL, "/index/hook/on_stream_not_found") == 0 || strcmp(httpURL, "/index/hook/on_record_mp4") == 0)
	{//��ʾ�յ�֪ͨ�¼�
		WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http �յ�֪ͨ�¼� \r\nhttpURL = %s\r\n%s\r\n", this, nClient, httpURL, requestParam);
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Http Request ( %s ) Success \",\"key\":%llu}", IndexApiCode_OK, httpURL, nClient);
		ResponseSuccess(szResponseBody);
		return true;
	}
	else if (strcmp(httpURL, "/index/api/getSnap") == 0)
	{//����ץ�� index_api_queryPictureList()
		index_api_getSnap();
	}
	else if (strcmp(httpURL, "/index/api/queryPictureList") == 0)
	{//��ѯͼƬ
		index_api_queryPictureList();
	}
	else if (strstr(httpURL, ".jpg") != 0)
	{//����httpͼƬ
		index_api_downloadImage(httpURL);
	}
	else if (strcmp(httpURL, "/index/api/controlStreamProxy") == 0)
	{//������������ 
		index_api_controlStreamProxy();
	}
	else if (strcmp(httpURL, "/index/api/setTransFilter") == 0)
	{//ˮӡת����� 
		index_api_setTransFilter();
	}
	else if (strcmp(httpURL, "/index/api/setConfigParamValue") == 0)
	{//�޸����ò���
		index_api_setConfigParamValue();
	}
	else if (strcmp(httpURL, "/index/api/shutdownServer") == 0)
	{//�˳�������
		index_api_shutdownServer();
	}
	else if (strcmp(httpURL, "/index/api/restartServer") == 0)
	{//����������
		index_api_restartServer();
	}
	else if (strcmp(httpURL, "/index/api/getTranscodingCount") == 0)
	{//��ȡ��ǰת������
		index_api_getTranscodingCount();
	}
	else if (strcmp(httpURL, "/index/api/listServerPort") == 0)
	{//��ȡ������ռ�ö˿�
		index_api_listServerPort();
	}
	else if (strcmp(httpURL, "/index/api/setServerConfig") == 0)
	{//�������÷���������
		index_api_setServerConfig();
	}
	else if (strcmp(httpURL, "/index/api/pauseRtpServer") == 0)
	{//���������ͣ
		index_api_pauseRtpServer();
	}
	else if (strcmp(httpURL, "/index/api/resumeRtpServer") == 0)
	{//������ռ���
		index_api_resumeRtpServer();
	}
	else if (strcmp(httpURL, "/index/api/controlRecordPlay") == 0)
	{//¼��طſ��� 
		index_api_controlRecordPlay();
	}
	else
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Http Request ( %s ) error\",\"key\":%d}", IndexApiCode_ErrorRequest, httpURL, 0);
		ResponseSuccess(szResponseBody);
		WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu http ��������Ƿ� ��httpURL = %s", this, nClient, httpURL);
		bRunFlag.exchange(false);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient,sizeof(nClient));
		return false;
 	}

	if (strcmp(httpURL, "/index/api/getSnap") != 0)
	{
		if (strcmp(szConnection, "close") == 0 || strcmp(szConnection, "Close") == 0 || ABL_MediaServerPort.httqRequstClose == 1)
		{
			Sleep(500);
 		    pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
        }
	}

	return true;
}

//ִ����������
bool  CNetServerHTTP::index_api_addStreamProxy(NetRevcBaseClientType nType)
{
	char szShareMediaURL[string_length_2048] = { 0 };
	
	memset(szRtspURLTemp,0x00,sizeof(szRtspURLTemp));
	memset((char*)&m_addStreamProxyStruct, 0x00, sizeof(m_addStreamProxyStruct));
	strcpy(m_addStreamProxyStruct.enable_mp4, "0");
	strcpy(m_addStreamProxyStruct.disableVideo, "0");
	strcpy(m_addStreamProxyStruct.disableAudio, "0");
	strcpy(m_addStreamProxyStruct.optionsHeartbeat, "0");
	sprintf(m_addStreamProxyStruct.fileKeepMaxTime, "%d", ABL_MediaServerPort.fileKeepMaxTime);
	sprintf(m_addStreamProxyStruct.videoFileFormat,"%d",ABL_MediaServerPort.videoFileFormat);
	sprintf(m_addStreamProxyStruct.G711ConvertAAC, "%d", ABL_MediaServerPort.nG711ConvertAAC);

	GetKeyValue("secret", m_addStreamProxyStruct.secret);
	GetKeyValue("vhost", m_addStreamProxyStruct.vhost);
	GetKeyValue("app", m_addStreamProxyStruct.app);
	GetKeyValue("stream", m_addStreamProxyStruct.stream);
	GetKeyValue("enable_mp4", m_addStreamProxyStruct.enable_mp4);
	GetKeyValue("enable_hls", m_addStreamProxyStruct.enable_hls);
	if(nType == NetRevcBaseClient_addStreamProxyControl)
	  GetKeyValue("isRtspRecordURL", m_addStreamProxyStruct.isRtspRecordURL);
	if (strlen(m_addStreamProxyStruct.isRtspRecordURL) == 0)
		strcpy(m_addStreamProxyStruct.isRtspRecordURL, "0");
	GetKeyValue("convertOutWidth", m_addStreamProxyStruct.convertOutWidth);
	GetKeyValue("convertOutHeight", m_addStreamProxyStruct.convertOutHeight);
	GetKeyValue("H264DecodeEncode_enable", m_addStreamProxyStruct.H264DecodeEncode_enable);
	GetKeyValue("disableVideo", m_addStreamProxyStruct.disableVideo);
	GetKeyValue("disableAudio", m_addStreamProxyStruct.disableAudio);
	GetKeyValue("optionsHeartbeat", m_addStreamProxyStruct.optionsHeartbeat);
	GetKeyValue("fileKeepMaxTime", m_addStreamProxyStruct.fileKeepMaxTime);
	GetKeyValue("videoFileFormat", m_addStreamProxyStruct.videoFileFormat);
	GetKeyValue("G711ConvertAAC", m_addStreamProxyStruct.G711ConvertAAC);

	//GetKeyValue("url", m_addStreamProxyStruct.url);
	GetKeyValue("url",szRtspURLTemp);
	DecodeUrl(szRtspURLTemp, m_addStreamProxyStruct.url, sizeof(szRtspURLTemp)) ;

	//��ֹ��������Ϊ¼��ط�
	if (strstr(m_addStreamProxyStruct.url, RecordFileReplaySplitter) != NULL)
		strcpy(m_addStreamProxyStruct.isRtspRecordURL, "1");

	//���һЩ�������
	if (strlen(m_addStreamProxyStruct.secret) == 0 || strlen(m_addStreamProxyStruct.app) == 0 || strlen(m_addStreamProxyStruct.stream) == 0 || strlen(m_addStreamProxyStruct.url) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret,app,stream,url required \"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_addStreamProxyStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\",\"key\":%d}", IndexApiCode_secretError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//app ,stream �������ַ������治���� / 
	if (strstr(m_addStreamProxyStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
 	}
	if (strstr(m_addStreamProxyStruct.stream, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���app stream �����Ƿ��� RecordFileReplaySplitter 
	if (strstr(m_addStreamProxyStruct.app, RecordFileReplaySplitter) != NULL || strstr(m_addStreamProxyStruct.stream, RecordFileReplaySplitter) != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app , stream parameter error,Forbidden %s \",\"key\":%d}", IndexApiCode_ParamError, RecordFileReplaySplitter, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���enable_mp4 ��ֵ
	if (strlen(m_addStreamProxyStruct.enable_mp4) > 0 && !(strcmp(m_addStreamProxyStruct.enable_mp4,"1") == 0 || strcmp(m_addStreamProxyStruct.enable_mp4, "0") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"enable_mp4 parameter error , enable_mp4 must (1 , 0 )\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (!( memcmp(m_addStreamProxyStruct.url,"rtsp://",7) == 0 || memcmp(m_addStreamProxyStruct.url, "rtmp://", 7) == 0 || memcmp(m_addStreamProxyStruct.url, "rtsps://",8) == 0 || memcmp(m_addStreamProxyStruct.url, "rtmps://", 8) == 0 || memcmp(m_addStreamProxyStruct.url, "http://", 7) == 0 || memcmp(m_addStreamProxyStruct.url, "https://", 8) == 0 || m_addStreamProxyStruct.url[1] == ':' || m_addStreamProxyStruct.url[0] == '/') )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"url %s parameter error\",\"key\":%d}", IndexApiCode_ParamError, m_addStreamProxyStruct.url, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���ת��Ŀ����Ƿ�Ϸ�
	if (strlen(m_addStreamProxyStruct.convertOutWidth) > 0 && strlen(m_addStreamProxyStruct.convertOutHeight) > 0)
	{
		int nWidth  = atoi(m_addStreamProxyStruct.convertOutWidth);
		int nHeight = atoi(m_addStreamProxyStruct.convertOutHeight);
		if (!((nWidth == -1 && nHeight == -1) || (nWidth == 1920 && nHeight == 1080) || (nWidth == 1280 && nHeight == 720) || (nWidth == 960 && nHeight == 640) ||
			(nWidth == 800 && nHeight == 480) || (nWidth == 704 && nHeight == 576) || (nWidth == 960 && nHeight == 540) || (nWidth == 640 && nHeight == 360 )))
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"convertOutWidth : %d ,convertOutHeight: %d error. reference value (-1 x -1 , 1920 x 1080, 1280 x 720 ,960 x 640 ,960 x 540, 800 x 480 ,704 x 576 ,640 x 360)\"}", IndexApiCode_ParamError, nWidth,nHeight);
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	//���H264DecodeEncode_enable ��ֵ
	if (strlen(m_addStreamProxyStruct.H264DecodeEncode_enable) > 0 && !(strcmp(m_addStreamProxyStruct.H264DecodeEncode_enable, "1") == 0 || strcmp(m_addStreamProxyStruct.H264DecodeEncode_enable, "0") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"H264DecodeEncode_enable parameter error , H264DecodeEncode_enable must is (0 , 1 )\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	
	//��� disableVideo disableAudio 
	if ( strcmp(m_addStreamProxyStruct.disableVideo,"1") == 0  && strcmp(m_addStreamProxyStruct.disableAudio, "1") == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"disableVideo , disableAudio Cannot be both 1 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strlen(m_addStreamProxyStruct.app) > 0 && strlen(m_addStreamProxyStruct.stream) > 0 && strlen(m_addStreamProxyStruct.url) > 0)
	{
		//��� app stream �Ƿ����
		sprintf(szShareMediaURL, "/%s/%s", m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);

		//����Ƿ�ռ��
		if (CheckAppStreamExisting(szShareMediaURL) == true)
		{//app,stream ����ʹ�ã�����������δ���� 
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s is Using.  \",\"key\":%d}", IndexApiCode_AppStreamHaveUsing, szShareMediaURL, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}

		auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
		if (tmpMediaSource != NULL)
		{
			DeleteMediaStreamSource(szShareMediaURL);
			WriteLog(Log_Debug, "ý��Դ %s �Ѿ��쳣������ִ��ɾ�� ", szShareMediaURL);
		}

		auto pClient = CreateNetRevcBaseClient(nType, 0, 0, m_addStreamProxyStruct.url, 0, szShareMediaURL);
		if (pClient != NULL)
		{
			strcpy(pClient->app, m_addStreamProxyStruct.app);
			strcpy(pClient->stream, m_addStreamProxyStruct.stream);
			memcpy((char*)&pClient->m_addStreamProxyStruct, (char*)&m_addStreamProxyStruct, sizeof(addStreamProxyStruct));
			if (strlen(m_addStreamProxyStruct.convertOutWidth) > 0 && strlen(m_addStreamProxyStruct.convertOutHeight) > 0)
			{//�ѿ��߸�ֵ��ת��ṹ
				pClient->m_h265ConvertH264Struct.convertOutWidth = atoi(m_addStreamProxyStruct.convertOutWidth);
				pClient->m_h265ConvertH264Struct.convertOutHeight = atoi(m_addStreamProxyStruct.convertOutHeight);
			}
			if (strlen(m_addStreamProxyStruct.H264DecodeEncode_enable) > 0)
				pClient->m_h265ConvertH264Struct.H264DecodeEncode_enable = atoi(m_addStreamProxyStruct.H264DecodeEncode_enable);

			WriteLog(Log_Debug, "\"rtsp://%s:%d/%s/%s\",", ABL_MediaServerPort.ABL_szLocalIP,ABL_MediaServerPort.nRtspPort,m_addStreamProxyStruct.app,m_addStreamProxyStruct.stream);
            pClient->nClient_http = nClient ; //��ֵ��http�������� 
			WriteLog(Log_Debug, "�������� nClient_http = %d ",nClient );
			pClient->SendFirstRequst();//ִ�е�һ������
		}
	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter error\",\"key\":0}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	return true;
}

//ɾ������HTTP���� 
bool  CNetServerHTTP::index_api_delRequest()
{
	char szShareMediaURL[string_length_512] = { 0 };

	memset((char*)&m_delRequestStruct, 0x00, sizeof(m_delRequestStruct));
	GetKeyValue("secret", m_delRequestStruct.secret);
	GetKeyValue("key", m_delRequestStruct.key);

	if (strcmp(m_delRequestStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���Key ��ֵ
	if (strlen(m_delRequestStruct.key) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"key parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	auto pClient = GetNetRevcBaseClient(atoi(m_delRequestStruct.key));
	if (pClient == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"key %s Not Found .\"}", IndexApiCode_KeyNotFound, m_delRequestStruct.key, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}
	else
	{
		auto pFindMediaSource = GetMediaStreamSourceByClientID(atoi(m_delRequestStruct.key));
		if(pFindMediaSource)//����Ϊ�û������Ͽ� 
		   pFindMediaSource->initiative = true ;
		
		WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu ɾ��Key %s �ɹ� ", this, nClient, m_delRequestStruct.key);
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"success\"}", IndexApiCode_OK);

		nDelKey = atoi(m_delRequestStruct.key);

		pDisconnectBaseNetFifo.push((unsigned char*)&nDelKey, sizeof(nDelKey));
 
		ResponseSuccess(szResponseBody);
		return true;
	}
}

//���Ӵ���rtsp\rtmp ���� 
bool CNetServerHTTP::index_api_addPushProxy()
{
	char szShareMediaURL[string_length_512] = { 0 };

	memset((char*)&m_addPushProxyStruct, 0x00, sizeof(m_addPushProxyStruct));
	strcpy(m_addPushProxyStruct.disableVideo, "0");
	strcpy(m_addPushProxyStruct.disableAudio, "0");
	GetKeyValue("secret", m_addPushProxyStruct.secret);
	GetKeyValue("vhost", m_addPushProxyStruct.vhost);
	GetKeyValue("app", m_addPushProxyStruct.app);
	GetKeyValue("stream", m_addPushProxyStruct.stream);
	GetKeyValue("url", m_addPushProxyStruct.url);
	GetKeyValue("disableVideo", m_addPushProxyStruct.disableVideo);
	GetKeyValue("disableAudio", m_addPushProxyStruct.disableAudio);

	//���һЩ�������
	if (strlen(m_addPushProxyStruct.secret) == 0 || strlen(m_addPushProxyStruct.app) == 0 || strlen(m_addPushProxyStruct.stream) == 0 || strlen(m_addPushProxyStruct.url) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret,app,stream,url required \"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_addPushProxyStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\",\"key\":%d}", IndexApiCode_secretError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//app ,stream �������ַ������治���� / 
	if (strstr(m_addPushProxyStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strstr(m_addPushProxyStruct.stream, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//ֻ֧��2��
	string strURL = m_addPushProxyStruct.url;
	int    nSubPathCount = 0, nPos = 10;
	while (true)
	{
		nPos = strURL.find("/", nPos);
		if (nPos > 0)
		{
			nSubPathCount++;
			nPos += 1;
 		}
		else
			break;
	}

	//��� disableVideo disableAudio 
	if (strcmp(m_addPushProxyStruct.disableVideo, "1") == 0 && strcmp(m_addPushProxyStruct.disableAudio, "1") == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"disableVideo , disableAudio Cannot be both 1 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (nSubPathCount != 2 || !(memcmp(m_addPushProxyStruct.url, "rtsp://", 7) == 0 || memcmp(m_addPushProxyStruct.url, "rtmp://", 7) == 0 || memcmp(m_addStreamProxyStruct.url, "http://", 7) == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"url %s parameter error\",\"key\":%d}", IndexApiCode_ParamError, m_addPushProxyStruct.url, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strlen(m_addPushProxyStruct.app) > 0 && strlen(m_addPushProxyStruct.stream) > 0 && strlen(m_addPushProxyStruct.url) > 0)
	{
		//��� app stream �Ƿ����
		sprintf(szShareMediaURL, "/%s/%s", m_addPushProxyStruct.app, m_addPushProxyStruct.stream);
		auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
		if (tmpMediaSource == NULL)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s Not Found \",\"key\":%d}", IndexApiCode_ParamError, szShareMediaURL, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}

		//�ж��Ƿ��Ѿ�������
		if (QueryMediaSource(m_addPushProxyStruct.url))
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"url: %s Already exists \",\"key\":%d}", IndexApiCode_ParamError, m_addPushProxyStruct.url, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}

		auto pClient = CreateNetRevcBaseClient(NetRevcBaseClient_addPushProxyControl, 0, 0, m_addPushProxyStruct.url, 0, szShareMediaURL);
		if (pClient != NULL)
		{
			memcpy((char*)&pClient->m_addPushProxyStruct, (char*)&m_addPushProxyStruct, sizeof(m_addPushProxyStruct));

			pClient->nClient_http = nClient; //��ֵ��http�������� 
			WriteLog(Log_Debug, "�������� nClient_http = %d ", nClient);

			pClient->SendFirstRequst();//ִ�е�һ������
 		}
	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter error\",\"key\":0}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	return true;
}

//����GB28181,��������
bool  CNetServerHTTP::index_api_openRtpServer()
{
	char szShareMediaURL[string_length_512] = { 0 };
	memset((char*)&m_openRtpServerStruct, 0x00, sizeof(m_openRtpServerStruct));

	strcpy(m_openRtpServerStruct.enable_mp4, "0");
	strcpy(m_openRtpServerStruct.RtpPayloadDataType, "1");
	strcpy(m_openRtpServerStruct.disableVideo, "0");
	strcpy(m_openRtpServerStruct.disableAudio, "0");
	strcpy(m_openRtpServerStruct.send_disableVideo, "0");
	strcpy(m_openRtpServerStruct.send_disableAudio, "0");
	strcpy(m_openRtpServerStruct.detectSendAppStream, "1");
	sprintf(m_openRtpServerStruct.fileKeepMaxTime, "%d", ABL_MediaServerPort.fileKeepMaxTime);
	sprintf(m_addStreamProxyStruct.videoFileFormat, "%d", ABL_MediaServerPort.videoFileFormat);
	sprintf(m_openRtpServerStruct.videoFileFormat, "%d", ABL_MediaServerPort.videoFileFormat);
	strcpy(m_openRtpServerStruct.jtt1078_KeepOpenPortType, "0");
	sprintf(m_openRtpServerStruct.G711ConvertAAC, "%d", ABL_MediaServerPort.nG711ConvertAAC);
 
	GetKeyValue("secret", m_openRtpServerStruct.secret);
	GetKeyValue("vhost", m_openRtpServerStruct.vhost);
	GetKeyValue("app", m_openRtpServerStruct.app);
	GetKeyValue("stream_id", m_openRtpServerStruct.stream_id);
	GetKeyValue("port", m_openRtpServerStruct.port);
	GetKeyValue("port2", m_openRtpServerStruct.port2);
	GetKeyValue("enable_tcp", m_openRtpServerStruct.enable_tcp);
	GetKeyValue("payload", m_openRtpServerStruct.payload);
	GetKeyValue("enable_mp4", m_openRtpServerStruct.enable_mp4);
	GetKeyValue("enable_hls", m_openRtpServerStruct.enable_hls);
	GetKeyValue("convertOutWidth", m_openRtpServerStruct.convertOutWidth);
	GetKeyValue("convertOutHeight", m_openRtpServerStruct.convertOutHeight);
	GetKeyValue("H264DecodeEncode_enable", m_openRtpServerStruct.H264DecodeEncode_enable);
	GetKeyValue("RtpPayloadDataType", m_openRtpServerStruct.RtpPayloadDataType);
	GetKeyValue("disableVideo", m_openRtpServerStruct.disableVideo);
	GetKeyValue("disableAudio", m_openRtpServerStruct.disableAudio);
	GetKeyValue("send_app", m_openRtpServerStruct.send_app);
	GetKeyValue("send_stream_id", m_openRtpServerStruct.send_stream_id);
	GetKeyValue("send_disableVideo", m_openRtpServerStruct.send_disableVideo);
	GetKeyValue("send_disableAudio", m_openRtpServerStruct.send_disableAudio);
	GetKeyValue("dst_url", m_openRtpServerStruct.dst_url);
	GetKeyValue("dst_port", m_openRtpServerStruct.dst_port);
	GetKeyValue("jtt1078_version", m_openRtpServerStruct.jtt1078_version);
	GetKeyValue("detectSendAppStream", m_openRtpServerStruct.detectSendAppStream);
	GetKeyValue("fileKeepMaxTime", m_openRtpServerStruct.fileKeepMaxTime);
	GetKeyValue("videoFileFormat", m_addStreamProxyStruct.videoFileFormat);
	GetKeyValue("videoFileFormat", m_openRtpServerStruct.videoFileFormat);
	GetKeyValue("jtt1078_KeepOpenPortType", m_openRtpServerStruct.jtt1078_KeepOpenPortType);
	GetKeyValue("G711ConvertAAC", m_openRtpServerStruct.G711ConvertAAC);
	GetKeyValue("G711ConvertAAC", m_addStreamProxyStruct.G711ConvertAAC);

	if (strlen(m_openRtpServerStruct.secret) == 0 || strlen(m_openRtpServerStruct.app) == 0 || strlen(m_openRtpServerStruct.stream_id) == 0 || strlen(m_openRtpServerStruct.port) == 0 ||
		strlen(m_openRtpServerStruct.enable_tcp) == 0 || strlen(m_openRtpServerStruct.payload) == 0 )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"(secret , app , stream_id , port , enable_tcp , payload ) parameter need .\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_openRtpServerStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\",\"key\":%d}", IndexApiCode_secretError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//app ,stream �������ַ������治���� / 
	if (strstr(m_openRtpServerStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strstr(m_openRtpServerStruct.stream_id, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream_id parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���stream_id �����Ƿ��� RecordFileReplaySplitter 
	if (strstr(m_openRtpServerStruct.stream_id, RecordFileReplaySplitter) != NULL || strstr(m_openRtpServerStruct.app, RecordFileReplaySplitter) != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app , stream_id parameter error,Forbidden %s \",\"key\":%d}", IndexApiCode_ParamError, RecordFileReplaySplitter, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	int nTcp_Switch = atoi(m_openRtpServerStruct.enable_tcp);
	if ( !(nTcp_Switch >= 0 && nTcp_Switch <= 2))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"enable_tcp parameter error ,enable_tcp must is  (0 , 1, 2) \",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strlen(m_openRtpServerStruct.payload) <= 0 || atoi(m_openRtpServerStruct.payload) < 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"payload parameter error \",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���enable_mp4 ��ֵ
	if (strlen(m_openRtpServerStruct.enable_mp4) > 0 && !(strcmp(m_openRtpServerStruct.enable_mp4, "1") == 0 || strcmp(m_openRtpServerStruct.enable_mp4, "0") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"enable_mp4 parameter error , enable_mp4 must (1 , 0 )\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���H264DecodeEncode_enable ��ֵ
	if (strlen(m_openRtpServerStruct.H264DecodeEncode_enable) > 0 && !(strcmp(m_openRtpServerStruct.H264DecodeEncode_enable, "1") == 0 || strcmp(m_openRtpServerStruct.H264DecodeEncode_enable, "0") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"H264DecodeEncode_enable parameter error , H264DecodeEncode_enable must is (0 , 1 )\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���ת��Ŀ����Ƿ�Ϸ�
	if (strlen(m_openRtpServerStruct.convertOutWidth) > 0 && strlen(m_openRtpServerStruct.convertOutHeight) > 0)
	{
		int nWidth = atoi(m_openRtpServerStruct.convertOutWidth);
		int nHeight = atoi(m_openRtpServerStruct.convertOutHeight);
		if (!((nWidth == -1 && nHeight == -1) || (nWidth == 1920 && nHeight == 1080) || (nWidth == 1280 && nHeight == 720) || (nWidth == 960 && nHeight == 640) || (nWidth == 960 && nHeight == 540) || (nWidth == 800 && nHeight == 480) || (nWidth == 704 && nHeight == 576) || (nWidth == 640 && nHeight == 360)))
 		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"convertOutWidth : %d ,convertOutHeight: %d error. reference value ( -1 x -1 ,1920 x 1080 , 1280 x 720 ,960 x 640 ,960 x 540 ,800 x 480 ,704 x 576 ,640 x 360)\"}", IndexApiCode_ParamError, nWidth, nHeight);
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	//���  enable_tcp == 2 TCP����ʱ ��dst_ur ,dst_port ����Ϊ�� 
	if (strcmp(m_openRtpServerStruct.enable_tcp, "2") == 0 )
	{
		if (strlen(m_openRtpServerStruct.dst_url) == 0 || strlen(m_openRtpServerStruct.dst_port) == 0)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"When enable_tcp is 2 , dst_url , dst_port Cannot be empty \"}", IndexApiCode_ParamError);
			ResponseSuccess(szResponseBody);
			return false;
 		}
	}

	//��� disableVideo disableAudio 
	if (strcmp(m_openRtpServerStruct.disableVideo, "1") == 0 && strcmp(m_openRtpServerStruct.disableAudio, "1") == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"disableVideo , disableAudio Cannot be both 1 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//��� send_disableVideo send_disableAudio 
	if (strcmp(m_openRtpServerStruct.send_disableVideo, "1") == 0 && strcmp(m_openRtpServerStruct.send_disableAudio, "1") == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"send_disableVideo , send_disableAudio Cannot be both 1 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//��ⷢ��app,stream ����ͬʱ����
	if ((strlen(m_openRtpServerStruct.send_app) > 0 && strlen(m_openRtpServerStruct.send_stream_id) == 0 ) || strlen(m_openRtpServerStruct.send_app) == 0 && strlen(m_openRtpServerStruct.send_stream_id) > 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"send_app , send_stream_id Must be set both \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//��ⷢ��app,stream 
	if (strlen(m_openRtpServerStruct.send_app) > 0 && strlen(m_openRtpServerStruct.send_stream_id) > 0)
	{
		if (m_openRtpServerStruct.detectSendAppStream[0] == 0x31)
		{
			sprintf(szShareMediaURL, "/%s/%s", m_openRtpServerStruct.send_app, m_openRtpServerStruct.send_stream_id);
			auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
			if (tmpMediaSource == NULL)
			{
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s Not Found \",\"key\":%d}", IndexApiCode_ParamError, szShareMediaURL, 0);
				ResponseSuccess(szResponseBody);
				return false;
			}
		}
	}

	//��� RtpPayloadDataType ��ȡֵ��Χ 
 	if (!(atoi(m_openRtpServerStruct.RtpPayloadDataType) >= 1 && atoi(m_openRtpServerStruct.RtpPayloadDataType) <= 4) )
	{
 		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Param RtpPayloadDataType  must at ( 1  2  3  4 ) \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
 	}

	//��� RtpPayloadDataType == 4 ��Ҫָ���汾��
	if (atoi(m_openRtpServerStruct.RtpPayloadDataType) == 4 && !(strcmp(m_openRtpServerStruct.jtt1078_version,"2013") == 0 || strcmp(m_openRtpServerStruct.jtt1078_version, "2016") == 0 || strcmp(m_openRtpServerStruct.jtt1078_version, "2019") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"It is necessary to set jtt1078_version equals 2013, 2016, 2019 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (atoi(m_openRtpServerStruct.port) > 0)
	{//���󶨵Ķ˿��������ļ����Ƿ�ʹ��
 		if (CheckPortIfUse(atoi(m_openRtpServerStruct.port)))
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"port = %d The bound port is already in use in the configuration file\"}", IndexApiCode_ParamError,atoi(m_openRtpServerStruct.port));
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	//������ó����˿ڲ���
	if (atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) >= 1 )
	{
		if (!(atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) >= 1 && atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) <= 4))
		{//jtt1078 �����˿����ͱ����� 1 �� 4 ֮��
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"jtt1078 The normally open port type must be between 1 and 4 directly \"}", IndexApiCode_ParamError);
			ResponseSuccess(szResponseBody);
			return false;
		}

		if (atoi(m_openRtpServerStruct.RtpPayloadDataType) != 4)
		{//ֻ��jtt1078��֧�ֳ����˿�����
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Only jtt1078 supports normally open port settings \"}", IndexApiCode_ParamError);
			ResponseSuccess(szResponseBody);
			return false;
		}
		if (atoi(m_openRtpServerStruct.enable_tcp) != 1)
		{//ֻ��tcp������֧�ֳ����˿�����
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Only TCP passive supports normally open port settings \"}", IndexApiCode_ParamError);
			ResponseSuccess(szResponseBody);
			return false;
		}
		if (atoi(m_openRtpServerStruct.port) == 0)
		{//jtt1078�����˿ڱ���ɫ���ñ��ض˿�
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Jtt1078 normally open port must be set to a local port \"}", IndexApiCode_ParamError);
			ResponseSuccess(szResponseBody);
			return false;
		}
 	}

	if (strlen(m_openRtpServerStruct.app) > 0 && strlen(m_openRtpServerStruct.stream_id) > 0 )
	{
		//��� app stream �Ƿ����
		sprintf(szShareMediaURL, "/%s/%s", m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);

		//���˿��Ƿ�ʹ��
		if (atoi(m_openRtpServerStruct.port) != 0 && CheckPortAlreadyUsed(atoi(m_openRtpServerStruct.port),1, true) == true)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"port error: %s Already used .\",\"key\":%d}", IndexApiCode_PortAlreadyUsed, m_openRtpServerStruct.port, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}

		//��� /app/stream �Ƿ��Ѿ�ʹ�� 
		if (CheckAppStreamExisting(szShareMediaURL) == true && atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) == 0 )
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter error: /%s/%s Already used .\",\"key\":%d}", IndexApiCode_AppStreamAlreadyUsed, m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}
 
		auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
		if (tmpMediaSource != NULL && atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) == 0)
		{
			DeleteMediaStreamSource(szShareMediaURL);
			WriteLog(Log_Debug, "ý��Դ %s �Ѿ��쳣������ִ��ɾ�� ", szShareMediaURL);
 		}

		int nRet = bindRtpServerPort();
 	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter error\",\"key\":0}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	return true;
}

//�󶨹�����ն˿�
int   CNetServerHTTP::bindRtpServerPort()
{
	nTcp_Switch = atoi(m_openRtpServerStruct.enable_tcp);
	int nRet = 2, nRet2 = 2,nRet3 = 2 ,nRet4 = 2 ;
	nMediaClient = nMediaClient2 = nMediaClient3 = nMediaClient4 = 0;
	char szTemp[string_length_2048] = { 0 };
	char szTemp2[string_length_2048] = { 0 };

	//�������jtt1078�ĳ����˿ڣ���Ҫ���� app��stream_id Ϊ�գ�����ᱻ�ж�Ϊ /app/stream_id ���ڽ��� 
	if (atoi(m_openRtpServerStruct.enable_tcp) == 1 && atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) >= 1)
	{
		memset(m_openRtpServerStruct.app, 0x00, sizeof(m_openRtpServerStruct.app));
		memset(m_openRtpServerStruct.stream_id, 0x00, sizeof(m_openRtpServerStruct.stream_id));
	}

	sprintf(szTemp, "/%s/%s", m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);
	sprintf(m_szShareMediaURL, "/%s/%s", m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);

	//�����������ṹ����app,stream ,����ý��Դ������ʱ��Ҫ
	strcpy(m_addStreamProxyStruct.app, m_openRtpServerStruct.app);
	strcpy(m_addStreamProxyStruct.stream, m_openRtpServerStruct.stream_id);
	sprintf(m_addStreamProxyStruct.url, "rtp://127.0.0.1:%s/%s/%s", m_openRtpServerStruct.port, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
	strcpy(m_addStreamProxyStruct.enable_mp4, m_openRtpServerStruct.enable_mp4);//�Ƿ�¼��
	strcpy(m_addStreamProxyStruct.enable_hls, m_openRtpServerStruct.enable_hls);//�Ƿ���hls
	strcpy(m_addStreamProxyStruct.convertOutWidth, m_openRtpServerStruct.convertOutWidth);//ת���
	strcpy(m_addStreamProxyStruct.convertOutHeight, m_openRtpServerStruct.convertOutHeight);//ת���
	strcpy(m_addStreamProxyStruct.H264DecodeEncode_enable, m_openRtpServerStruct.H264DecodeEncode_enable);//ת���
	strcpy(m_addStreamProxyStruct.RtpPayloadDataType, m_openRtpServerStruct.RtpPayloadDataType);//Pt ��������
	strcpy(m_addStreamProxyStruct.disableVideo, m_openRtpServerStruct.disableVideo);//�Ƿ������Ƶ
	strcpy(m_addStreamProxyStruct.disableAudio, m_openRtpServerStruct.disableAudio);//�Ƿ������Ƶ
	strcpy(m_addStreamProxyStruct.dst_url, m_openRtpServerStruct.dst_url);//tcp��������IP 
	strcpy(m_addStreamProxyStruct.dst_port, m_openRtpServerStruct.dst_port);//tcp�������Ӷ˿�
	strcpy(m_addStreamProxyStruct.fileKeepMaxTime, m_openRtpServerStruct.fileKeepMaxTime);//¼����󱣴�ʱ�� ����ȷ��ÿһ·ý��Դ���ȴ������ļ���ȡĬ��ֵ��addStreamProxy ��openRtpServer ���������޸� 
	strcpy(m_addStreamProxyStruct.videoFileFormat, m_openRtpServerStruct.videoFileFormat);//¼���ļ���ʽ  1  fmp4(ps), 2  mp4 , 3  ts  ��addStreamProxy ��openRtpServer ���������޸� 
	strcpy(m_addStreamProxyStruct.G711ConvertAAC, m_openRtpServerStruct.G711ConvertAAC);//g711a,g711u �Ƿ�ת��Ϊ AAC  
  
	if (nTcp_Switch == 0)
	{//udp��ʽ
		if (atoi(m_openRtpServerStruct.port) == 0)
		{
			while(true)
			{
				if (atoi(m_openRtpServerStruct.RtpPayloadDataType) == 2)
				{//����ES�� ����Ҫ�������˿� 
 					sprintf(m_openRtpServerStruct.port, "%d", ABL_nGB28181Port ); //������ʵ�˿� 

					nRet = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port, NULL, &nMediaClient, onread, 1);//rtp
					nRet2 = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port + 1, NULL, &nMediaClient2, onread, 1);//rtcp
  					ABL_nGB28181Port += 2;

 					sprintf(m_openRtpServerStruct.port2, "%d", ABL_nGB28181Port ); //������ʵ�˿� 

					nRet3 = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port, NULL, &nMediaClient3, onread, 1);//rtp
					nRet4 = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port + 1, NULL, &nMediaClient4, onread, 1);//rtcp
  					ABL_nGB28181Port += 2;
					
					if (nRet == 0 && nRet2 == 0 && nRet3 == 0 && nRet4 == 0)
						break;
					else
					{//�ر�����һ��
						if (nRet == 0)
							XHNetSDK_DestoryUdp(nMediaClient);
						if (nRet2 == 0)
							XHNetSDK_DestoryUdp(nMediaClient2);
						if (nRet3 == 0)
							XHNetSDK_DestoryUdp(nMediaClient3);
						if (nRet4 == 0)
							XHNetSDK_DestoryUdp(nMediaClient4);
					}					
				}else 
				{
					//�Զ������˿�
					sprintf(m_openRtpServerStruct.port, "%d", ABL_nGB28181Port ); //������ʵ�˿� 

					nRet = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port, NULL, &nMediaClient, onread, 1);//rtp
					nRet2 = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port + 1, NULL, &nMediaClient2, onread, 1);//rtcp
					ABL_nGB28181Port += 2;
					if (nRet == 0 && nRet2 == 0)
						break;
					else
					{//�ر�����һ��
						if (nRet == 0)
							XHNetSDK_DestoryUdp(nMediaClient);
						if (nRet2 == 0)
							XHNetSDK_DestoryUdp(nMediaClient2);
					}
				}
			} 
		}
		else
		{
			nRet = XHNetSDK_BuildUdp(NULL, atoi(m_openRtpServerStruct.port), NULL, &nMediaClient, onread, 1);
			nRet2 = XHNetSDK_BuildUdp(NULL, atoi(m_openRtpServerStruct.port) + 1, NULL, &nMediaClient2, onread, 1);
			if (atoi(m_openRtpServerStruct.RtpPayloadDataType) == 2 && atoi(m_openRtpServerStruct.port2) > 0 && atoi(m_openRtpServerStruct.port2) <= 65535 )
			{//ES ��2 �˿�
				nRet3 = XHNetSDK_BuildUdp(NULL, atoi(m_openRtpServerStruct.port2), NULL, &nMediaClient3, onread, 1);
				nRet4 = XHNetSDK_BuildUdp(NULL, atoi(m_openRtpServerStruct.port2) + 1, NULL, &nMediaClient4, onread, 1);
			}
		}
 
		if (nRet == 0 && nRet2 == 0)
		{//rtp ,rtcp ���󶨳ɹ�
			auto pClient = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RtpServerUDP, 0, nMediaClient, "", atoi(m_openRtpServerStruct.port), szTemp);
 			if (pClient != NULL)
			{
				strcpy(pClient->app, m_openRtpServerStruct.app);
				strcpy(pClient->stream, m_openRtpServerStruct.stream_id);
				pClient->hParent = nMediaClient;//udp��ʽû�и������ �����԰ѱ���ID��Ϊ���������������������Ͽ�ʱʹ��
				pClient->nClientPort = atoi(m_openRtpServerStruct.port);
				pClient->m_gbPayload = atoi(m_openRtpServerStruct.payload);//����Ϊ��ȷ��paylad
				pClient->nClientRtcp = nMediaClient2;//rtcp ����
				memcpy((char*)&pClient->m_openRtpServerStruct, (char*)&m_openRtpServerStruct, sizeof(m_openRtpServerStruct));
				memcpy((unsigned char*)&pClient->m_addStreamProxyStruct, (unsigned char*)&m_addStreamProxyStruct, sizeof(m_addStreamProxyStruct));
				if (strlen(m_addStreamProxyStruct.convertOutWidth) > 0 && strlen(m_addStreamProxyStruct.convertOutHeight) > 0)
				{//�ѿ��߸�ֵ��ת��ṹ
					pClient->m_h265ConvertH264Struct.convertOutWidth = atoi(m_addStreamProxyStruct.convertOutWidth);
					pClient->m_h265ConvertH264Struct.convertOutHeight = atoi(m_addStreamProxyStruct.convertOutHeight);
				}
				if (strlen(m_addStreamProxyStruct.H264DecodeEncode_enable) > 0)
					pClient->m_h265ConvertH264Struct.H264DecodeEncode_enable = atoi(m_addStreamProxyStruct.H264DecodeEncode_enable);

				if (atoi(m_openRtpServerStruct.RtpPayloadDataType) == 2 && nMediaClient3 > 0  )
				{//����ES����2�˿�
					auto pClient2 = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RtpServerUDP, 0, nMediaClient3, "", atoi(m_openRtpServerStruct.port2), szTemp);
					if (pClient2 != NULL)
					{
						strcpy(pClient2->app, m_openRtpServerStruct.app);
						strcpy(pClient2->stream, m_openRtpServerStruct.stream_id);
						pClient2->hParent = nMediaClient3;//udp��ʽû�и������ �����԰ѱ���ID��Ϊ���������������������Ͽ�ʱʹ��
						pClient2->nClientPort = atoi(m_openRtpServerStruct.port);
						pClient2->m_gbPayload = atoi(m_openRtpServerStruct.payload);//����Ϊ��ȷ��paylad
						pClient2->nClientRtcp = nMediaClient4;//rtcp ����
						memcpy((char*)&pClient2->m_openRtpServerStruct, (char*)&m_openRtpServerStruct, sizeof(m_openRtpServerStruct));
						memcpy((unsigned char*)&pClient2->m_addStreamProxyStruct, (unsigned char*)&m_addStreamProxyStruct, sizeof(m_addStreamProxyStruct));
						if (strlen(m_addStreamProxyStruct.convertOutWidth) > 0 && strlen(m_addStreamProxyStruct.convertOutHeight) > 0)
						{//�ѿ��߸�ֵ��ת��ṹ
							pClient2->m_h265ConvertH264Struct.convertOutWidth = atoi(m_addStreamProxyStruct.convertOutWidth);
							pClient2->m_h265ConvertH264Struct.convertOutHeight = atoi(m_addStreamProxyStruct.convertOutHeight);
						}
						if (strlen(m_addStreamProxyStruct.H264DecodeEncode_enable) > 0)
							pClient2->m_h265ConvertH264Struct.H264DecodeEncode_enable = atoi(m_addStreamProxyStruct.H264DecodeEncode_enable);

						sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"port\":\"%s\",\"port2\":\"%s\",\"key\":%llu}", m_openRtpServerStruct.port, m_openRtpServerStruct.port2, nMediaClient);
					}
				}else 
				   sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"port\":\"%s\",\"key\":%llu}", m_openRtpServerStruct.port, nMediaClient);
			}
		}
		else
		{//������һ���˿ڰ�ʧ�� 
			XHNetSDK_DestoryUdp(nMediaClient);//�ر� rtp 
			XHNetSDK_DestoryUdp(nMediaClient2);//�ر� rtcp 
			nRet = 2;//��ʶΪ���ɹ� 
		}
	}
	else if (nTcp_Switch == 1)
	{//�����TCP
		WriteLog(Log_Debug, "׼����tcp �˿� ");

		if (atoi(m_openRtpServerStruct.port) == 0)
		{
			do
			{
				if (atoi(m_openRtpServerStruct.RtpPayloadDataType) == 2)
				{//ES ��������ͨ2���˿�
					sprintf(m_openRtpServerStruct.port, "%d", ABL_nGB28181Port); //������ʵ�˿� 
					nRet = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_nGB28181Port, &nMediaClient, onaccept, onread, onclose, true);
 					ABL_nGB28181Port += 2;

					sprintf(m_openRtpServerStruct.port2, "%d", ABL_nGB28181Port); //������ʵ�˿� 
					nRet2 = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_nGB28181Port, &nMediaClient2, onaccept, onread, onclose, true);
					ABL_nGB28181Port += 2;

					if (!(nRet == 0 && nRet2 == 0))
					{
						XHNetSDK_Unlisten(nMediaClient);
						XHNetSDK_Unlisten(nMediaClient2);
						continue;
					}
				}
				else
				{
  					sprintf(m_openRtpServerStruct.port, "%d", ABL_nGB28181Port); //������ʵ�˿� 

					nRet = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_nGB28181Port, &nMediaClient, onaccept, onread, onclose, true);
					ABL_nGB28181Port += 2;
				}
			} while (nRet != 0);
		}
		else
		{
			nRet = XHNetSDK_Listen((int8_t*)("0.0.0.0"), atoi(m_openRtpServerStruct.port), &nMediaClient, onaccept, onread, onclose, true);
			if(atoi(m_openRtpServerStruct.RtpPayloadDataType) == 2 && atoi(m_openRtpServerStruct.port2) > 0 && atoi(m_openRtpServerStruct.port2) <= 65535 )
				 XHNetSDK_Listen((int8_t*)("0.0.0.0"), atoi(m_openRtpServerStruct.port2), &nMediaClient2, onaccept, onread, onclose, true);
 		}

		WriteLog(Log_Debug, "׼����tcp �˿� %s �ɹ� , nRet = %d", m_openRtpServerStruct.port, nRet);

		if (nRet == 0)
		{
			WriteLog(Log_Debug, "��ʼ����������� ");

			auto pClient = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RtpServerListen, 0, nMediaClient, "", atoi(m_openRtpServerStruct.port), szTemp);
			if (pClient != NULL)
			{
				strcpy(pClient->app, m_openRtpServerStruct.app);
				strcpy(pClient->stream, m_openRtpServerStruct.stream_id);
				pClient->nClientPort = atoi(m_openRtpServerStruct.port);
 		      	memcpy((char*)&pClient->m_addStreamProxyStruct,(char*)&m_addStreamProxyStruct,sizeof(m_addStreamProxyStruct));
				memcpy((char*)&pClient->m_openRtpServerStruct,(char*)&m_openRtpServerStruct,sizeof(m_openRtpServerStruct));
				if (strlen(m_addStreamProxyStruct.convertOutWidth) > 0 && strlen(m_addStreamProxyStruct.convertOutHeight) > 0)
				{//�ѿ��߸�ֵ��ת��ṹ
					pClient->m_h265ConvertH264Struct.convertOutWidth = atoi(m_addStreamProxyStruct.convertOutWidth);
					pClient->m_h265ConvertH264Struct.convertOutHeight = atoi(m_addStreamProxyStruct.convertOutHeight);
				}
				if (strlen(m_addStreamProxyStruct.H264DecodeEncode_enable) > 0)
					pClient->m_h265ConvertH264Struct.H264DecodeEncode_enable = atoi(m_addStreamProxyStruct.H264DecodeEncode_enable);

				if (atoi(m_openRtpServerStruct.RtpPayloadDataType) == 2 && nMediaClient2 > 0 )
				{//����ES����2�˿�
					auto pClient2 = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RtpServerListen, 0, nMediaClient2, "", atoi(m_openRtpServerStruct.port2), szTemp);
					if (pClient2 != NULL)
					{
						strcpy(pClient2->app, m_openRtpServerStruct.app);
						strcpy(pClient2->stream, m_openRtpServerStruct.stream_id);
						pClient2->nClientPort = atoi(m_openRtpServerStruct.port2);
						memcpy((char*)&pClient2->m_addStreamProxyStruct, (char*)&m_addStreamProxyStruct, sizeof(m_addStreamProxyStruct));
						memcpy((char*)&pClient2->m_openRtpServerStruct, (char*)&m_openRtpServerStruct, sizeof(m_openRtpServerStruct));
						strcpy(pClient2->m_openRtpServerStruct.port2, m_openRtpServerStruct.port);
						if (strlen(m_addStreamProxyStruct.convertOutWidth) > 0 && strlen(m_addStreamProxyStruct.convertOutHeight) > 0)
						{ 
							pClient2->m_h265ConvertH264Struct.convertOutWidth = atoi(m_addStreamProxyStruct.convertOutWidth);
							pClient2->m_h265ConvertH264Struct.convertOutHeight = atoi(m_addStreamProxyStruct.convertOutHeight);
						}
						if (strlen(m_addStreamProxyStruct.H264DecodeEncode_enable) > 0)
							pClient2->m_h265ConvertH264Struct.H264DecodeEncode_enable = atoi(m_addStreamProxyStruct.H264DecodeEncode_enable);
					}
					sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"port\":\"%s\",\"port2\":\"%s\",\"key\":%llu}", m_openRtpServerStruct.port, m_openRtpServerStruct.port2, nMediaClient);
				}else 
				  sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"port\":\"%s\",\"key\":%llu}", m_openRtpServerStruct.port, nMediaClient);

				if(atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType) != 0 )
					WriteLog(Log_Debug, "�ɹ��� jtt1078 �����˿� %d ,��������Ϊ jtt1078_KeepOpenPortType = %d ", atoi(m_openRtpServerStruct.port),atoi(m_openRtpServerStruct.jtt1078_KeepOpenPortType));

			}else 
				WriteLog(Log_Debug, "��ʼ�����������ʧ�� ");
 		}
	}
	else if (nTcp_Switch == 2)
	{//tcp �������� 
		sprintf(m_szShareMediaURL, "/%s/%s", m_openRtpServerStruct.app, m_openRtpServerStruct.stream_id);
		if (atoi(m_openRtpServerStruct.port) == 0)
		{
			sprintf(m_openRtpServerStruct.port, "%d", ABL_nGB28181Port);
			nRet = XHNetSDK_Connect((int8_t*)m_openRtpServerStruct.dst_url, atoi(m_openRtpServerStruct.dst_port), (int8_t*)(NULL), ABL_nGB28181Port, (uint64_t*)&nMediaClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);
		    ABL_nGB28181Port += 2;
		}
		else
			nRet = XHNetSDK_Connect((int8_t*)m_openRtpServerStruct.dst_url, atoi(m_openRtpServerStruct.dst_port), (int8_t*)(NULL), atoi(m_openRtpServerStruct.port), (uint64_t*)&nMediaClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);

#ifdef USE_BOOST
		boost::shared_ptr<CNetRevcBase> pClient = NULL;
#else
		std::shared_ptr<CNetRevcBase> pClient = NULL;
#endif
		if (nRet == 0)
			pClient = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RtpServerTCP_Active, 0, nMediaClient, "", atoi(m_openRtpServerStruct.port), m_szShareMediaURL);

		if (pClient != NULL)
		{
			strcpy(pClient->app, m_openRtpServerStruct.app);
			strcpy(pClient->stream, m_openRtpServerStruct.stream_id);
			pClient->nClient_http = nClient;
			pClient->nReturnPort = atoi(m_openRtpServerStruct.port);
			strcpy(pClient->m_addStreamProxyStruct.app, m_openRtpServerStruct.app);
			strcpy(pClient->m_addStreamProxyStruct.stream, m_openRtpServerStruct.stream_id);

			//��¼���ӹ����IP���˿� 
			strcpy(pClient->m_rtspStruct.szIP, m_openRtpServerStruct.dst_url);
			strcpy(pClient->m_rtspStruct.szPort, m_openRtpServerStruct.dst_port);
			strcpy(pClient->szClientIP, m_openRtpServerStruct.dst_url);
			pClient->nClientPort = atoi(m_openRtpServerStruct.dst_port);

			memcpy((char*)&pClient->m_addStreamProxyStruct, (char*)&m_addStreamProxyStruct, sizeof(m_addStreamProxyStruct));
			memcpy((char*)&pClient->m_openRtpServerStruct, (char*)&m_openRtpServerStruct, sizeof(m_openRtpServerStruct));
			if (strlen(m_addStreamProxyStruct.convertOutWidth) > 0 && strlen(m_addStreamProxyStruct.convertOutHeight) > 0)
			{//�ѿ��߸�ֵ��ת��ṹ
				pClient->m_h265ConvertH264Struct.convertOutWidth = atoi(m_addStreamProxyStruct.convertOutWidth);
				pClient->m_h265ConvertH264Struct.convertOutHeight = atoi(m_addStreamProxyStruct.convertOutHeight);
			}
			if (strlen(m_addStreamProxyStruct.H264DecodeEncode_enable) > 0)
				pClient->m_h265ConvertH264Struct.H264DecodeEncode_enable = atoi(m_addStreamProxyStruct.H264DecodeEncode_enable);

		}
		sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\",\"port\":\"%s\",\"key\":%llu}", m_openRtpServerStruct.port, nMediaClient);
	}
 
	 //�ظ�Http ����
	if (nRet != 0)
 	   sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"bind port %s failed .\",\"port\":\"%s\",\"key\":%llu}", IndexApiCode_BindPortError, m_openRtpServerStruct.port, m_openRtpServerStruct.port, 0);
	
	// 0 udp ��ʽ��1 tcp ������ʽֱ�ӷ�����Ӧ��2 tcp ������ʽ��Ҫ���ӳɹ���ʧ�ܺ��ٷ���
	if(nTcp_Switch == 0 || nTcp_Switch == 1 ) 
	  ResponseSuccess(szResponseBody);

 	if (ABL_nGB28181Port >= ABL_MediaServerPort.GB28181RtpMaxPort)
		ABL_nGB28181Port = ABL_MediaServerPort.GB28181RtpMinPort;  //�˿����·�ת

	//�󶨶˿�ʧ�ܣ���Ҫɾ��
	if (nRet != 0)
		return -1;
	else
		return 0;
}
//����GB28181,����rtp
bool  CNetServerHTTP::index_api_startSendRtp()
{
	char szShareMediaURL[string_length_512] = { 0 };
	unsigned short nReturnPort ;
	int  nRet = 0,nRet2 = 0 ;
	int is_udp = 0;
	NETHANDLE nRtcpClient;

	memset((char*)&m_startSendRtpStruct, 0x00, sizeof(m_startSendRtpStruct));
	strcpy(m_startSendRtpStruct.RtpPayloadDataType, "1");//Ĭ��PS���
	strcpy(m_startSendRtpStruct.disableAudio, "0");
	strcpy(m_startSendRtpStruct.disableVideo, "0");
	strcpy(m_startSendRtpStruct.recv_disableAudio, "0");
	strcpy(m_startSendRtpStruct.recv_disableVideo, "0");
	strcpy(m_startSendRtpStruct.enable_hls, "0");
	strcpy(m_startSendRtpStruct.enable_mp4, "0");
	sprintf(m_startSendRtpStruct.fileKeepMaxTime, "%d", ABL_MediaServerPort.fileKeepMaxTime);

	GetKeyValue("secret", m_startSendRtpStruct.secret);
	GetKeyValue("vhost", m_startSendRtpStruct.vhost);
	GetKeyValue("app", m_startSendRtpStruct.app);
	GetKeyValue("stream", m_startSendRtpStruct.stream);
	GetKeyValue("ssrc", m_startSendRtpStruct.ssrc);
	GetKeyValue("src_port", m_startSendRtpStruct.src_port);
	GetKeyValue("dst_url", m_startSendRtpStruct.dst_url);
	GetKeyValue("dst_port", m_startSendRtpStruct.dst_port);
	GetKeyValue("is_udp", m_startSendRtpStruct.is_udp);
	GetKeyValue("payload", m_startSendRtpStruct.payload);
 	GetKeyValue("RtpPayloadDataType", m_startSendRtpStruct.RtpPayloadDataType);
	GetKeyValue("disableAudio", m_startSendRtpStruct.disableAudio);
	GetKeyValue("disableVideo", m_startSendRtpStruct.disableVideo);
	GetKeyValue("recv_app", m_startSendRtpStruct.recv_app);
	GetKeyValue("recv_stream", m_startSendRtpStruct.recv_stream);
	GetKeyValue("recv_disableAudio", m_startSendRtpStruct.recv_disableAudio);
	GetKeyValue("recv_disableVideo", m_startSendRtpStruct.recv_disableVideo);
	GetKeyValue("jtt1078_version", m_startSendRtpStruct.jtt1078_version);
	GetKeyValue("enable_hls", m_startSendRtpStruct.enable_hls);
	GetKeyValue("enable_mp4", m_startSendRtpStruct.enable_mp4);
	GetKeyValue("fileKeepMaxTime", m_startSendRtpStruct.fileKeepMaxTime);

	is_udp = atoi(m_startSendRtpStruct.is_udp);
	if (strlen(m_startSendRtpStruct.secret) == 0 || strlen(m_startSendRtpStruct.app) == 0 || strlen(m_startSendRtpStruct.stream) == 0 || strlen(m_startSendRtpStruct.ssrc) == 0 ||
		strlen(m_startSendRtpStruct.src_port) == 0 || strlen(m_startSendRtpStruct.dst_url) == 0 || strlen(m_startSendRtpStruct.dst_port) == 0 || strlen(m_startSendRtpStruct.is_udp) == 0 ||
		strlen(m_startSendRtpStruct.payload) == 0)
	{
		if (is_udp == 2)
		{
 		}
		else
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"(secret , app , stream , ssrc , src_port , dst_url ,dst_port, is_udp, payload ) parameter need .\",\"key\":%d}", IndexApiCode_ParamError, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	if (strcmp(m_startSendRtpStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\",\"key\":%d}", IndexApiCode_secretError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���dst_port �˿�
	if (atoi(m_startSendRtpStruct.dst_port) <= 0 && (is_udp == 0 || is_udp == 1))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"dst_port: %s error ,dst_port must > 0 \",\"key\":%d}", IndexApiCode_ParamError, m_startSendRtpStruct.dst_port, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���src_port �˿�
	if (strlen(m_startSendRtpStruct.src_port) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"src_port parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//app ,stream �������ַ������治���� / 
	if (strstr(m_startSendRtpStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strstr(m_startSendRtpStruct.stream, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream parameter error\",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (!(is_udp == 0 || is_udp == 1 || is_udp == 2))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"is_udp parameter error ,is_udp in (0 , 1, 2 ) \",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strlen(m_startSendRtpStruct.payload) == 0 || atoi(m_startSendRtpStruct.payload) < 0 )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"payload parameter error \",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}
 
	//ssrc�����д�� 0 �����ɷ��������� 
	if (atoi(m_startSendRtpStruct.ssrc) == 0)
		sprintf(m_startSendRtpStruct.ssrc, "%d", ABL_nSSRC);
	ABL_nSSRC ++;
	if (ABL_nSSRC >= 0xFFFFFFEE)
		ABL_nSSRC = 28888; //ssrc��ת

	if ((strlen(m_startSendRtpStruct.ssrc) == 0 || atoi(m_startSendRtpStruct.ssrc) == 0) && m_startSendRtpStruct.RtpPayloadDataType[0] != 0x34 )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"ssrc parameter error , ssrc Cannot be Zero  \",\"key\":%d}", IndexApiCode_ParamError, 0);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_startSendRtpStruct.disableVideo,"1") == 0 && strcmp(m_startSendRtpStruct.disableAudio, "1") == 0)
	{// disableVideo , disableAudio ����ͬʱΪ 1
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"disableVideo , disableAudio Cannot be both 1 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_startSendRtpStruct.recv_disableVideo, "1") == 0 && strcmp(m_startSendRtpStruct.recv_disableAudio, "1") == 0)
	{// disableVideo , disableAudio ����ͬʱΪ 1
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"recv_disableVideo , recv_disableAudio Cannot be both 1 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//��ⷢ��recv_app, recv_stream ����ͬʱ����
	if ((strlen(m_startSendRtpStruct.recv_app) > 0 && strlen(m_startSendRtpStruct.recv_stream) == 0) || strlen(m_startSendRtpStruct.recv_app) == 0 && strlen(m_startSendRtpStruct.recv_stream) > 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"recv_app , recv_stream Must be set both \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���������Ƿ���� 
	if (strlen(m_startSendRtpStruct.recv_app) > 0 && strlen(m_startSendRtpStruct.recv_stream) >  0  )
	{
 		sprintf(szShareMediaURL, "/%s/%s", m_startSendRtpStruct.recv_app, m_startSendRtpStruct.recv_stream);
		auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
		if (tmpMediaSource != NULL)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s Already exists \",\"key\":%d}", IndexApiCode_ParamError, szShareMediaURL, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	//��� RtpPayloadDataType == 4 ��Ҫָ���汾��
	if (atoi(m_startSendRtpStruct.RtpPayloadDataType) == 4 && !(strcmp(m_startSendRtpStruct.jtt1078_version, "2013") == 0 || strcmp(m_startSendRtpStruct.jtt1078_version, "2016") == 0 || strcmp(m_startSendRtpStruct.jtt1078_version, "2019") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"It is necessary to set jtt1078_version equals 2013, 2016, 2019 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//TCP�������ͼ��󶨵Ķ˿��Ƿ������ļ�ʹ�� 
	if (atoi(m_startSendRtpStruct.src_port) > 0 && atoi(m_startSendRtpStruct.is_udp) == 2 )
	{ 
		if (CheckPortIfUse(atoi(m_startSendRtpStruct.src_port)))
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"src_port = %d The bound port is already in use in the configuration file\"}", IndexApiCode_ParamError,atoi(m_startSendRtpStruct.src_port));
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	if (strlen(m_startSendRtpStruct.app) > 0 && strlen(m_startSendRtpStruct.stream) > 0)
	{
		//���˿��Ƿ�ʹ��
		if (atoi(m_startSendRtpStruct.src_port) != 0 && CheckPortAlreadyUsed(atoi(m_startSendRtpStruct.src_port), 2, true) == true)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"src_port error: %s Already used .\",\"key\":%d}", IndexApiCode_PortAlreadyUsed, m_startSendRtpStruct.src_port, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}

		//���ssrc �Ƿ�ʹ��
		if (CheckSSRCAlreadyUsed(atoi(m_startSendRtpStruct.ssrc), true) == true)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"ssrc error: %s Already used .\",\"key\":%d}", IndexApiCode_SSRClreadyUsed, m_startSendRtpStruct.ssrc, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}

		/* ��������ͬ��IP���˿ڷ��Ͷ������������10000 �˿ڷ��͹����� ��� dst_url , dst_port  �Ƿ�ʹ��
		if (CheckDst_url_portAlreadyUsed(m_startSendRtpStruct.dst_url,atoi(m_startSendRtpStruct.dst_port), true) == true)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Media stream has already been sent to the same IP and port . ip: %s , port : %s \"}", IndexApiCode_dst_url_dst_portUsed, m_startSendRtpStruct.dst_url, m_startSendRtpStruct.dst_port);
			ResponseSuccess(szResponseBody);
			return false;
		}*/ 

		//��� app stream �Ƿ����
		sprintf(szShareMediaURL, "/%s/%s", m_startSendRtpStruct.app, m_startSendRtpStruct.stream);

 		auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
		if (tmpMediaSource == NULL)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s Not Found \",\"key\":%d}", IndexApiCode_ParamError, szShareMediaURL, 0);
			ResponseSuccess(szResponseBody);
			return false;
		}

		//���� RtpPayloadDataType == 2
		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32)
		{
			if (atoi(m_startSendRtpStruct.disableAudio) == 0 && atoi(m_startSendRtpStruct.disableVideo) == 0)
			{//�������� RtpPayloadDataType == 2 ������ disableVideo ���� disableAudio  ���� 1 
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\" If RtpPayloadDataType is equal to 2, disableVdieo must be equal to 1, or disableAudio must be equal to 1 \"}", IndexApiCode_ParamError);
				ResponseSuccess(szResponseBody);
				return false;
			}

			//���ֻ����Ƶ���Ͳ�����disable ��Ƶ 
			if (strlen(tmpMediaSource->m_mediaCodecInfo.szVideoName) > 0 && strlen(tmpMediaSource->m_mediaCodecInfo.szAudioName) == 0 && atoi(m_startSendRtpStruct.disableVideo) == 1)
			{
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\" This media source %s only has videos and cannot be disable Video . \"}", IndexApiCode_ParamError, tmpMediaSource->m_szURL);
				ResponseSuccess(szResponseBody);
				return false;
			}
			//���ֻ����Ƶ���Ͳ�����disable��Ƶ 
			if (strlen(tmpMediaSource->m_mediaCodecInfo.szVideoName) == 0 && strlen(tmpMediaSource->m_mediaCodecInfo.szAudioName) > 0 && atoi(m_startSendRtpStruct.disableAudio) == 1)
			{
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\" This media source %s only has audios and cannot be disable audios . \"}", IndexApiCode_ParamError, tmpMediaSource->m_szURL);
				ResponseSuccess(szResponseBody);
				return false;
			}
		}
#ifdef USE_BOOST
		boost::shared_ptr<CNetRevcBase> pClient = NULL;
#else

		std::shared_ptr<CNetRevcBase> pClient = NULL;
#endif
		if (is_udp == 1)
		{//udp ��ʽ 
			if (atoi(m_startSendRtpStruct.src_port) == 0)
			{
				while(true)
				{
					nRet = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port, NULL, &nMediaClient, onread, 1);
					nRet2 = XHNetSDK_BuildUdp(NULL, ABL_nGB28181Port+1, NULL, &nRtcpClient, onread, 1);
					ABL_nGB28181Port += 2;
					if (nRet == 0 && nRet2 == 0)
						break;
					else 
					{//�رյ�����һ���ɹ���udp 
						if(nRet == 0 )
						  XHNetSDK_DestoryUdp(nMediaClient);
						if(nRet2 == 0)
						  XHNetSDK_DestoryUdp(nRtcpClient);
					}
				}  
			}
			else
			{
				nRet = XHNetSDK_BuildUdp(NULL, atoi(m_startSendRtpStruct.src_port), NULL, &nMediaClient, onread, 1);
				nRet2 = XHNetSDK_BuildUdp(NULL, atoi(m_startSendRtpStruct.src_port)+1, NULL, &nRtcpClient, onread, 1);
			}
			if (nRet == 0 && nRet2 == 0)
			{//����������˿ڳɹ� 
				pClient = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181SendRtpUDP, 0, nMediaClient, "", 0, szShareMediaURL);
				if (pClient != NULL && nRtcpClient > 0)
				{
					strcpy(pClient->szClientIP,m_startSendRtpStruct.dst_url);
					pClient->nClientRtcp = nRtcpClient;
				}
			}
			else
			{//ֻҪ����һ�����ɹ�������Ҫ�ر� 
				nRet = -1; //�󶨶˿�ʧ��
				XHNetSDK_DestoryUdp(nMediaClient);
				XHNetSDK_DestoryUdp(nRtcpClient);
			}
		}
		else if(is_udp == 0)
		{//tcp �������ӷ�ʽ 
			if (atoi(m_startSendRtpStruct.src_port) == 0)
			{
				nRet = XHNetSDK_Connect((int8_t*)m_startSendRtpStruct.dst_url, atoi(m_startSendRtpStruct.dst_port), (int8_t*)(NULL), ABL_nGB28181Port, (uint64_t*)&nMediaClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);
				ABL_nGB28181Port += 2;
			}
			else
				nRet = XHNetSDK_Connect((int8_t*)m_startSendRtpStruct.dst_url, atoi(m_startSendRtpStruct.dst_port), (int8_t*)(NULL), atoi(m_startSendRtpStruct.src_port), (uint64_t*)&nMediaClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);

			if (nRet == 0)
			{
				pClient = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181SendRtpTCP_Connect, 0, nMediaClient, "", 1, szShareMediaURL);
				if(pClient)
				  strcpy(pClient->szClientIP,m_startSendRtpStruct.dst_url);
			}
		}
		else if (is_udp == 2)
		{//tcp �������� 
			if(atoi(m_startSendRtpStruct.src_port) > 0 )
			   nRet = XHNetSDK_Listen((int8_t*)("0.0.0.0"), atoi(m_startSendRtpStruct.src_port), &nMediaClient, onaccept, onread, onclose, true);
			else
			{
			  while (true)
			  {//һֱ�󶨵��ɹ�Ϊֹ
			    nRet = XHNetSDK_Listen((int8_t*)("0.0.0.0"), ABL_nGB28181Port, &nMediaClient, onaccept, onread, onclose, true);
			    ABL_nGB28181Port += 2;
				if (nRet == 0)
					break;
 			  }
			  sprintf(m_startSendRtpStruct.src_port, "%d", ABL_nGB28181Port - 2);//�޸�Ϊʵ�ʰ󶨵Ķ˿�
 			}

			WriteLog(Log_Debug, "XHNetSDK_Listen()  nMediaClient = %llu", nMediaClient);
			if (nRet == 0 && nMediaClient > 0)
			{
				pClient = CreateNetRevcBaseClient(NetBaseNetType_NetGB28181RtpSendListen, 0, nMediaClient, "", atoi(m_startSendRtpStruct.src_port), szShareMediaURL);
				if (pClient != NULL)
				{
					nRet = 0;
					memcpy((char*)&pClient->m_startSendRtpStruct, (char*)&m_startSendRtpStruct, sizeof(m_startSendRtpStruct)); //��http����� m_startSendRtpStruct ������listen����� m_startSendRtpStruct
				}
			}
 		}

		nReturnPort = ABL_nGB28181Port - 2;
		if (atoi(m_startSendRtpStruct.src_port) > 0)
			nReturnPort = atoi(m_startSendRtpStruct.src_port);

		if (nRet != 0)
		{//������Դ����ʧ�ܣ���Ҫɾ�� netGB28181RtpClient ����
 		    sprintf(szResponseBody, "{\"code\":%d,\"port\":%d,\"memo\":\"bind port %d Failed .\",\"key\":%d}", IndexApiCode_BindPortError, nReturnPort, nReturnPort, 0);
 		    ResponseSuccess(szResponseBody);
 	     }

		if (pClient != NULL)
		{
			memcpy((char*)&pClient->m_startSendRtpStruct, (char*)&m_startSendRtpStruct, sizeof(m_startSendRtpStruct));

			pClient->nClient_http = nClient; //��ֵ��http�������� 
			pClient->nReturnPort = pClient->nClientPort = nReturnPort;//���걾�ط��Ͷ˿�
 
			//��¼���ӹ����IP���˿� 
			strcpy(pClient->m_rtspStruct.szIP, m_startSendRtpStruct.dst_url);
			strcpy(pClient->m_rtspStruct.szPort, m_startSendRtpStruct.dst_port);

			if (is_udp == 1 && tmpMediaSource != NULL )
			{//��Ϊudp ʹ�ã���Ϊtcpʱ�� ��SendFirstRequst() �⺯������ 
  				memcpy((char*)&pClient->mediaCodecInfo, (char*)&tmpMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));

				tmpMediaSource->AddClientToMap(pClient->nClient);
			}

			if (is_udp == 1 || is_udp == 2)
			{
 			  if(nRet == 0)
			   sprintf(szResponseBody, "{\"code\":0,\"port\":%d,\"memo\":\"success\",\"key\":%llu}", nReturnPort, pClient->nClient);
			 else 
			   sprintf(szResponseBody, "{\"code\":%d,\"port\":%d,\"memo\":\"bind port %d Failed .\",\"key\":%d}", IndexApiCode_BindPortError, nReturnPort, nReturnPort, 0);

 			  ResponseSuccess(szResponseBody);
 			}
			else
			{//tcp ��Ҫ�ж��Ƿ����ӳɹ�

			}
		}

		if (atoi(m_startSendRtpStruct.src_port) == 0)
		{
			WriteLog(Log_Debug, "index_api_startSendRtp() nClient = %llu ,is_udp = %s, ʹ�õĶ˿�Ϊ %d ",nClient, m_startSendRtpStruct.is_udp,ABL_nGB28181Port - 2);
 			if (ABL_nGB28181Port >= ABL_MediaServerPort.GB28181RtpMaxPort)
				ABL_nGB28181Port = ABL_MediaServerPort.GB28181RtpMinPort;  //�˿����·�ת
		}
		else
		{
			WriteLog(Log_Debug, "index_api_startSendRtp() nClient = %llu ,is_udp = %s, ʹ�õĶ˿�Ϊ %s ", nClient, m_startSendRtpStruct.is_udp, m_startSendRtpStruct.src_port);
		}
	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter error\",\"key\":0}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	return true;
}

//��ȡý���б�
bool CNetServerHTTP::index_api_getMediaList()
{
	memset((char*)&m_getMediaListStruct, 0x00, sizeof(m_getMediaListStruct));

 	GetKeyValue("secret", m_getMediaListStruct.secret);
	GetKeyValue("vhost", m_getMediaListStruct.vhost);
	GetKeyValue("app", m_getMediaListStruct.app);
	GetKeyValue("stream", m_getMediaListStruct.stream);

	if (strcmp(m_getMediaListStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	memset(szMediaSourceInfoBuffer, 0x00, MaxMediaSourceInfoLength);
	int nMediaCount = GetAllMediaStreamSource(szMediaSourceInfoBuffer, m_getMediaListStruct);
	if (nMediaCount >= 0)
	{
 		ResponseSuccess(szMediaSourceInfoBuffer);
	}

	return true;
}

//��ȡ���ⷢ�͵������б�
bool CNetServerHTTP::index_api_getOutList()
{
	memset((char*)&m_getOutListStruct, 0x00, sizeof(m_getOutListStruct));

	GetKeyValue("secret", m_getOutListStruct.secret);
	GetKeyValue("mediaType", m_getOutListStruct.outType);

	if (strcmp(m_getOutListStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	memset(szMediaSourceInfoBuffer, 0x00, MaxMediaSourceInfoLength);
	int nMediaCount = GetAllOutList(szMediaSourceInfoBuffer, m_getOutListStruct.outType);
	if (nMediaCount >= 0)
	{
		ResponseSuccess(szMediaSourceInfoBuffer);
	}

	return true;
}

//��ȡϵͳ����
bool CNetServerHTTP::index_api_getServerConfig()
{
	memset((char*)&m_getServerConfigStruct, 0x00, sizeof(m_getServerConfigStruct));
	char  szTempBuffer[string_length_1024] = { 0 };
	GetKeyValue("secret", m_getServerConfigStruct.secret);
 
	if (strcmp(m_getServerConfigStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	memset(szMediaSourceInfoBuffer, 0x00, MaxMediaSourceInfoLength);
	sprintf(szMediaSourceInfoBuffer, "{\"code\":0,\"params\":[{\"secret\":\"%s\",\"memo\":\"server password\"},{\"ServerIP\":\"%s\",\"memo\":\"ABLMediaServer ip address\"},{\"mediaServerID\":\"%s\",\"memo\":\"media Server ID \"},{\"hook_enable\":%d,\"memo\":\"hook_enable = 1 open notice , hook_enable = 0 close notice \"},{\"enable_audio\":%d,\"memo\":\"enable_audio = 1 open Audio , enable_audio = 0 Close Audio \"},{\"httpServerPort\":%d,\"memo\":\"http api port \"},{\"rtspPort\":%d,\"memo\":\"rtsp port \"},{\"rtmpPort\":%d,\"memo\":\"rtmp port \"},{\"httpFlvPort\":%d,\"memo\":\"http-flv port \"},{\"hls_enable\":%d,\"memo\":\"hls whether enable \"},{\"hlsPort\":%d,\"memo\":\"hls port\"},{\"wsPort\":%d,\"memo\":\"websocket flv port\"},{\"mp4Port\":%d,\"memo\":\"http mp4 port\"},{\"ps_tsRecvPort\":%d,\"memo\":\"recv ts , ps Stream port \"},{\"hlsCutType\":%d,\"memo\":\"hlsCutType = 1 hls cut to Harddisk,hlsCutType = 2  hls cut Media to memory\"},{\"h265CutType\":%d,\"memo\":\" 1 h265 cut TS , 2 cut fmp4 \"},{\"RecvThreadCount\":%d,\"memo\":\" RecvThreadCount \"},{\"SendThreadCount\":%d,\"memo\":\"SendThreadCount\"},{\"GB28181RtpTCPHeadType\":%d,\"memo\":\"rtp Length Type\"},{\"ReConnectingCount\":%d,\"memo\":\"Try reconnections times .\"},{\"maxTimeNoOneWatch\":%.2f,\"memo\":\"maxTimeNoOneWatch .\"},{\"pushEnable_mp4\":%d,\"memo\":\"pushEnable_mp4 .\"},{\"fileSecond\":%d,\"memo\":\"fileSecond .\"},{\"fileKeepMaxTime\":%d,\"memo\":\"fileKeepMaxTime .\"},{\"httpDownloadSpeed\":%d,\"memo\":\"httpDownloadSpeed .\"},{\"RecordReplayThread\":%d,\"memo\":\"Total number of video playback threads .\"},{\"convertMaxObject\":%d,\"memo\":\"Max number of video Convert .\"},{\"version\":\"%s\",\"memo\":\"ABLMediaServer currrent Version .\"},{\"recordPath\":\"%s\",\"memo\":\"ABLMediaServer Record File Path  .\"},{\"picturePath\":\"%s\",\"memo\":\"ABLMediaServer Snap Picture Path  .\"}", 
		ABL_MediaServerPort.secret, ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.mediaServerID, ABL_MediaServerPort.hook_enable,ABL_MediaServerPort.nEnableAudio,ABL_MediaServerPort.nHttpServerPort, ABL_MediaServerPort.nRtspPort, ABL_MediaServerPort.nRtmpPort, ABL_MediaServerPort.nHttpFlvPort, ABL_MediaServerPort.nHlsEnable, ABL_MediaServerPort.nHlsPort, ABL_MediaServerPort.nWSFlvPort, ABL_MediaServerPort.nHttpMp4Port, ABL_MediaServerPort.ps_tsRecvPort, ABL_MediaServerPort.nHLSCutType, ABL_MediaServerPort.nH265CutType, ABL_MediaServerPort.nRecvThreadCount, ABL_MediaServerPort.nSendThreadCount, ABL_MediaServerPort.nGBRtpTCPHeadType, ABL_MediaServerPort.nReConnectingCount,
		ABL_MediaServerPort.maxTimeNoOneWatch, ABL_MediaServerPort.pushEnable_mp4,ABL_MediaServerPort.fileSecond,ABL_MediaServerPort.fileKeepMaxTime,ABL_MediaServerPort.httpDownloadSpeed, ABL_MediaServerPort.nRecordReplayThread, ABL_MediaServerPort.convertMaxObject, MediaServerVerson, ABL_MediaServerPort.recordPath, ABL_MediaServerPort.picturePath);

	sprintf(szTempBuffer, ",{\"noneReaderDuration\":%.2f,\"memo\":\"How many seconds does it take for no one to watch and send notifications  .\"}", ABL_MediaServerPort.noneReaderDuration);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_server_started\":\"%s\",\"memo\":\"Server starts sending event notifications  .\"}", ABL_MediaServerPort.on_server_started);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_server_keepalive\":\"%s\",\"memo\":\"Server Heartbeat Event Notification  .\"}", ABL_MediaServerPort.on_server_keepalive);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_play\":\"%s\",\"memo\":\"Play a certain stream to send event notifications  .\"}", ABL_MediaServerPort.on_play);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_publish\":\"%s\",\"memo\":\"Registering a certain stream to the server to send event notifications  .\"}", ABL_MediaServerPort.on_publish);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_stream_arrive\":\"%s\",\"memo\":\"Send event notification when a certain media source stream reaches its destination  .\"}", ABL_MediaServerPort.on_stream_arrive);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_stream_not_arrive\":\"%s\",\"memo\":\"A certain media source was registered but the stream timed out and did not arrive. Send event notification  .\"}", ABL_MediaServerPort.on_stream_not_arrive);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_stream_none_reader\":\"%s\",\"memo\":\"Send event notification when no one is watching a certain media source  .\"}", ABL_MediaServerPort.on_stream_none_reader);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_stream_disconnect\":\"%s\",\"memo\":\"Send event notification when a certain channel of media is disconnected  .\"}", ABL_MediaServerPort.on_stream_disconnect);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_stream_not_found\":\"%s\",\"memo\":\"Media source not found Send event notification .\"}", ABL_MediaServerPort.on_stream_not_found);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_record_mp4\":\"%s\",\"memo\":\"Send event notification when a recording is completed .\"}", ABL_MediaServerPort.on_record_mp4);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_delete_record_mp4\":\"%s\",\"memo\":\"Send event notification when a video recording is overwritten .\"}", ABL_MediaServerPort.on_delete_record_mp4);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_record_progress\":\"%s\",\"memo\":\"Sending event notifications every 1 second while recording .\"}", ABL_MediaServerPort.on_record_progress);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_record_ts\":\"%s\",\"memo\":\"Send event notification when hls slicing completes a section of ts file .\"}", ABL_MediaServerPort.on_record_ts);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"enable_GetFileDuration\":%d,\"memo\":\"Whether to enable the acquistition of record File duration  .\"}", ABL_MediaServerPort.enable_GetFileDuration);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"keepaliveDuration\":%d,\"memo\":\"Time interval for sending heartbeat .\"}", ABL_MediaServerPort.keepaliveDuration);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"captureReplayType\":%d,\"memo\":\"Capture Image Return Method .\"}", ABL_MediaServerPort.captureReplayType);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"pictureMaxCount\":%d,\"memo\":\"Maximum number of saved captured images  .\"}", ABL_MediaServerPort.pictureMaxCount);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"videoFileFormat\":%d,\"memo\":\"Video files are in sliced format (1  fmp4 , 2  mp4 , 3  ts )  .\"}", ABL_MediaServerPort.videoFileFormat);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"MaxDiconnectTimeoutSecond\":%d,\"memo\":\"Maximum timeout for recviving data  .\"}", ABL_MediaServerPort.MaxDiconnectTimeoutSecond);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"G711ConvertAAC\":%d,\"memo\":\"Do G711a and g711u transcode to aac  .\"}", ABL_MediaServerPort.nG711ConvertAAC);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"filterVideo_enable\":%d,\"memo\":\"Do you want to turn on video watermark  .\"}", ABL_MediaServerPort.filterVideo_enable);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"filterVideo_text\":\"%s\",\"memo\":\"Set Video watermark content .\"}", ABL_MediaServerPort.filterVideoText);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"FilterFontSize\":%d,\"memo\":\"Set Video watermark font size .\"}", ABL_MediaServerPort.nFilterFontSize);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"FilterFontColor\":\"%s\",\"memo\":\"Set Video watermark font color .\"}", ABL_MediaServerPort.nFilterFontColor);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"FilterFontLeft\":%d,\"memo\":\"Set the left coordinate of th Video watermark  .\"}", ABL_MediaServerPort.nFilterFontLeft);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"FilterFontTop\":%d,\"memo\":\"Set the top coordinate of th Video watermark  .\"}", ABL_MediaServerPort.nFilterFontTop);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"FilterFontAlpha\":%.2f,\"memo\":\"Set the transparency of th Video watermark  .\"}", ABL_MediaServerPort.nFilterFontAlpha);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"convertOutWidth\":%d,\"memo\":\"Set transcoding video Width  .\"}", ABL_MediaServerPort.convertOutWidth);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"convertOutHeight\":%d,\"memo\":\"Set transcoding video Height  .\"}", ABL_MediaServerPort.convertOutHeight);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"convertOutBitrate\":%d,\"memo\":\"Set th bitrate for video transcoding  .\"}", ABL_MediaServerPort.convertOutBitrate);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"webrtcPort\":%d,\"memo\":\"WebRtc Player port  .\"}", ABL_MediaServerPort.nWebRtcPort);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"WsRecvPcmPort\":%d,\"memo\":\"the port for recv audio by Websocket .\"}", ABL_MediaServerPort.WsRecvPcmPort);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"flvPlayAddMute\":%d,\"memo\":\"When playing HTTP FLV and WS FLV, do you want to turn on mute when there is no audio in the source stream .\"}", ABL_MediaServerPort.flvPlayAddMute);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"gb28181LibraryUse\":%d,\"memo\":\"Link Library for GB28181 PS Standard Packaging .\"}", ABL_MediaServerPort.gb28181LibraryUse);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"GB28181RtpMinPort\":%d,\"memo\":\"Recv GB28181 min Port .\"}", ABL_MediaServerPort.GB28181RtpMinPort);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"GB28181RtpMaxPort\":%d,\"memo\":\"Recv GB28181 max Port .\"}", ABL_MediaServerPort.GB28181RtpMaxPort);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"recordFileCutType\":%d,\"memo\":\"1 According to the local server time difference , 2  Calculate time based on the total frames rate and frame speed of the recorded video \"}", ABL_MediaServerPort.recordFileCutType);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"on_rtsp_replay\":\"%s\",\"memo\":\"Send event notification when replay rtsp url .\"}", ABL_MediaServerPort.on_rtsp_replay);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"SyncWritePacket\":\"%d\",\"memo\":\"Is it synchronous or asynchronous to send network packets  .\"}", ABL_MediaServerPort.nSyncWritePacket);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"1078Port\":%d,\"memo\":\"Recv 1078 Device Port .\"}", ABL_MediaServerPort.n1078Port);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);
	sprintf(szTempBuffer, ",{\"jtt1078Version\":%d,\"memo\":\"1078 Device Version .\"}", ABL_MediaServerPort.jtt1078Version);
	strcat(szMediaSourceInfoBuffer, szTempBuffer);

	strcat(szMediaSourceInfoBuffer, "]}");
#ifdef OS_System_Windows
	string strResponse = szMediaSourceInfoBuffer;
#ifdef USE_BOOST
	replace_all(strResponse, "\\", "\\\\"); 
#else
	ABL::replace_all(strResponse, "\\", "\\\\"); 
#endif
	memset(szMediaSourceInfoBuffer, 0x00, MaxMediaSourceInfoLength);
	strcpy(szMediaSourceInfoBuffer, strResponse.c_str());
#endif 
	ResponseSuccess(szMediaSourceInfoBuffer);

	return true;
}

//����app,stream ���ر�ָ��ý��Դ
bool  CNetServerHTTP::index_api_close_streams()
{
	char  szTemp2[string_length_2048] = { 0 };
 	memset((char*)&m_closeStreamsStruct, 0x00, sizeof(m_closeStreamsStruct));
	int   nDeleteCount = 0;

	GetKeyValue("secret", m_closeStreamsStruct.secret);
	GetKeyValue("vhost", m_closeStreamsStruct.vhost);
	GetKeyValue("app", m_closeStreamsStruct.app);
	GetKeyValue("stream", m_closeStreamsStruct.stream);
	GetKeyValue("vhost", m_closeStreamsStruct.vhost);
	GetKeyValue("schema", m_closeStreamsStruct.schema);
	GetKeyValue("force", szTemp2);
	m_closeStreamsStruct.force = atoi(szTemp2);

	memset(szResponseBody, 0x00, sizeof(szResponseBody));
	if (strcmp(m_closeStreamsStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strlen(szTemp2) == 0)
	{//force �������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"force param need \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (!(m_closeStreamsStruct.force >= 0 && m_closeStreamsStruct.force <= 1))
	{//force �������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"force value error ,( 0, 1 ) \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	nDeleteCount = CloseMediaStreamSource(m_closeStreamsStruct);
	if (nDeleteCount > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		//Sleep(200);

	sprintf(szResponseBody, "{\"code\":0,\"count_closed\":\"%d\"}", nDeleteCount);
	ResponseSuccess(szResponseBody);

	return true;
}

//��ʼ��ֹͣ¼��
bool  CNetServerHTTP::index_api_startStopRecord(bool bFlag)
{
	char szShareMediaURL[string_length_2048] = { 0 };

	memset((char*)&m_startStopRecordStruct, 0x00, sizeof(m_startStopRecordStruct));
 	GetKeyValue("secret", m_startStopRecordStruct.secret);
	GetKeyValue("vhost", m_startStopRecordStruct.vhost);
	GetKeyValue("app", m_startStopRecordStruct.app);
	GetKeyValue("stream", m_startStopRecordStruct.stream);
 
	if (strlen(m_startStopRecordStruct.secret) == 0 || strlen(m_startStopRecordStruct.app) == 0 || strlen(m_startStopRecordStruct.stream) == 0 )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret , app , stream ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_startStopRecordStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//app ,stream �������ַ������治���� / 
	if (strstr(m_startStopRecordStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strstr(m_startStopRecordStruct.stream, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strlen(m_startStopRecordStruct.app) > 0 && strlen(m_startStopRecordStruct.stream) > 0)
	{
		//��� app stream �Ƿ����
		sprintf(szShareMediaURL, "/%s/%s", m_startStopRecordStruct.app, m_startStopRecordStruct.stream);
		auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
		if (tmpMediaSource == NULL)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s Not Found \"}", IndexApiCode_ParamError, szShareMediaURL);
			ResponseSuccess(szResponseBody);
			return false;
		}

		if (bFlag)
		{//��ʼ¼��
			if (tmpMediaSource->enable_mp4 == false && tmpMediaSource->recordMP4 == 0)
			{//��δ��ʼ¼��
			  sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s start Record .\"}", IndexApiCode_OK, szShareMediaURL);
			  tmpMediaSource->enable_mp4 = true;
			}
			else
			{//�Ѿ���ʼ¼��
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s have record .\"}", IndexApiCode_RequestProcessFailed, szShareMediaURL);
			}
		}
		else
		{
			if (tmpMediaSource->enable_mp4 == true && tmpMediaSource->recordMP4 > 0 )
			{//�Ѿ���ʼ¼��
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s stop Record .\"}", IndexApiCode_OK, szShareMediaURL);
				tmpMediaSource->enable_mp4 = false ;
				pDisconnectBaseNetFifo.push((unsigned char*)&tmpMediaSource->recordMP4, sizeof(tmpMediaSource->recordMP4));
				tmpMediaSource->recordMP4 = 0;
			}
			else
			{//��δ��ʼ¼��
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s have not Record .\"}", IndexApiCode_RequestProcessFailed, szShareMediaURL);
			}
		}
		ResponseSuccess(szResponseBody);
	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
 	}

	return true;
}

//��ѯ¼���б�
bool  CNetServerHTTP::index_api_queryRecordList()
{
	char szShareMediaURL[string_length_2048] = { 0 };
	char szOutRecordPlayURL[string_length_1024] = { 0 };

	memset((char*)&m_queryRecordListStruct, 0x00, sizeof(m_queryRecordListStruct));
	GetKeyValue("secret", m_queryRecordListStruct.secret);
	GetKeyValue("vhost", m_queryRecordListStruct.vhost);
	GetKeyValue("app", m_queryRecordListStruct.app);
	GetKeyValue("stream", m_queryRecordListStruct.stream);
	GetKeyValue("starttime", m_queryRecordListStruct.starttime);
	GetKeyValue("endtime", m_queryRecordListStruct.endtime);

	if (strlen(m_queryRecordListStruct.secret) == 0 || strlen(m_queryRecordListStruct.app) == 0 || strlen(m_queryRecordListStruct.stream) == 0 ||
		strlen(m_queryRecordListStruct.starttime) == 0 || strlen(m_queryRecordListStruct.endtime) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret , app , stream , starttime , endtime ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_queryRecordListStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strlen(m_queryRecordListStruct.starttime) != 14 || strlen(m_queryRecordListStruct.endtime) != 14)
	{//��ʼʱ�䡢����ʱ��ĳ��̼�� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( starttime , endtime ) Length Error ,length must 14 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#ifdef USE_BOOST
	if ( boost::all(m_queryRecordListStruct.starttime, boost::is_digit()) == false || boost::all(m_queryRecordListStruct.endtime, boost::is_digit()) == false )
	{//��⿪ʼʱ�䡢����ʱ�� �Ƿ������ֵ��ַ�����0 ~ 9��
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( starttime , endtime ) error , must is number \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#else
	if (ABL::is_digits(m_queryRecordListStruct.starttime) == false || ABL::is_digits(m_queryRecordListStruct.endtime) == false)
	{//��⿪ʼʱ�䡢����ʱ�� �Ƿ������ֵ��ַ�����0 ~ 9��
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( starttime , endtime ) error , must is number \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#endif

	if ( atoll(m_queryRecordListStruct.endtime) < atoll(m_queryRecordListStruct.starttime))
	{//����ʱ�������ڿ�ʼʱ��
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"endtime must greater than starttime \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//������ʱ�����С�ڵ�ǰʱ���
	/*if (GetCurrentSecondByTime(m_queryRecordListStruct.endtime) > (GetCurrentSecond() - ABL_MediaServerPort.fileSecond))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"endtime must current time %llu second before \"}", IndexApiCode_ParamError, ABL_MediaServerPort.fileSecond);
		ResponseSuccess(szResponseBody);
		return false;
	}*/

	//����ѯʱ�������ܳ���7��
	if (GetCurrentSecondByTime(m_queryRecordListStruct.endtime) - GetCurrentSecondByTime(m_queryRecordListStruct.starttime)  >  (31 * 24 * 3600))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"query times No more than 31 days \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//app ,stream �������ַ������治���� / 
	if (strstr(m_queryRecordListStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strstr(m_queryRecordListStruct.stream, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//�ȼ��ж��Ƿ�洢¼��
	sprintf(szShareMediaURL, "/%s/%s", m_queryRecordListStruct.app, m_queryRecordListStruct.stream);
	auto pRecord = GetRecordFileSource(szShareMediaURL);
	if (pRecord == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"count\":0,\"memo\":\"MediaSource ( app: %s , stream: %s ) RecordFile Not Found .\"}", IndexApiCode_MediaSourceNotFound, m_queryRecordListStruct.app, m_queryRecordListStruct.stream);
		ResponseSuccess(szResponseBody);
		return false;
	}

	memset(szMediaSourceInfoBuffer, 0x00, MaxMediaSourceInfoLength);
	int nMediaCount = queryRecordListByTime(szMediaSourceInfoBuffer, m_queryRecordListStruct, szOutRecordPlayURL);
	if (nMediaCount >= 0)
	{
		if (nMediaCount > 0)
		{//�ȴ� url �ɹ�����
			int64_t nStartTime = GetTickCount64();
			while (true)
			{
				Sleep(100);
				auto tmpRecordSource = GetMediaStreamSource(szOutRecordPlayURL);
				if (tmpRecordSource != NULL )
				{
					if(strlen(tmpRecordSource->m_mediaCodecInfo.szVideoName) > 0)
					{
					   WriteLog(Log_Debug, "��ʽ����¼��ط��� %s ", szOutRecordPlayURL);
					   break;
					}
				}
 				if (GetTickCount() - nStartTime > 1000 * 5)
					break;
			}
		}
		ResponseSuccess(szMediaSourceInfoBuffer);
	}

	return true;
}

//����ץ��
bool  CNetServerHTTP::index_api_getSnap()
{
	char szShareMediaURL[string_length_2048] = { 0 };

	memset((char*)&m_getSnapStruct, 0x00, sizeof(m_getSnapStruct));
	sprintf(m_getSnapStruct.captureReplayType, "%d", ABL_MediaServerPort.captureReplayType);
	GetKeyValue("secret", m_getSnapStruct.secret);
	GetKeyValue("vhost", m_getSnapStruct.vhost);
	GetKeyValue("app", m_getSnapStruct.app);
	GetKeyValue("stream", m_getSnapStruct.stream);
	GetKeyValue("timeout_sec", m_getSnapStruct.timeout_sec);
	GetKeyValue("captureReplayType", m_getSnapStruct.captureReplayType);

	if (strlen(m_getSnapStruct.secret) == 0 || strlen(m_getSnapStruct.app) == 0 || strlen(m_getSnapStruct.stream) == 0 || strlen(m_getSnapStruct.timeout_sec) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret , app , stream,timeout_sec ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_getSnapStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#ifdef USE_BOOST
	if (boost::all(m_getSnapStruct.timeout_sec, boost::is_digit()) == false)
	{//��ⳬʱ����������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"[timeout_sec ] error , must is number \"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#else
	if (ABL::is_digits(m_getSnapStruct.timeout_sec) == false)
	{//��ⳬʱ����������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"[timeout_sec ] error , must is number \"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#endif

	//app ,stream �������ַ������治���� / 
	if (strstr(m_getSnapStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strstr(m_getSnapStruct.stream, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���ץ�������Ƿ񳬹��趨������
	int nCount = GetNetRevcBaseClientCountByNetType(NetBaseNetType_SnapPicture_JPEG,true);
	if (nCount > ABL_MediaServerPort.maxSameTimeSnap)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"current snap count over maxSameTimeSnap %d \"}", IndexApiCode_OverMaxSameTimeSnap, nCount);
		ResponseSuccess(szResponseBody);
		WriteLog(Log_Debug, "����ץ�� current snap count over maxSameTimeSnap %d, nClient = %llu ", nCount, nClient);
		return false;
	}
 
	if (strlen(m_getSnapStruct.app) > 0 && strlen(m_getSnapStruct.stream) > 0)
	{
		//��� app stream �Ƿ����
		sprintf(szShareMediaURL, "/%s/%s", m_getSnapStruct.app, m_getSnapStruct.stream);
		auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
		if (tmpMediaSource == NULL)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s Not Found \"}", IndexApiCode_ParamError, szShareMediaURL);
			ResponseSuccess(szResponseBody);
			return false;
		}

		//boost::shared_ptr<CNetRevcBase>  pClient = NULL;
		auto pClient = GetNetRevcBaseClientByNetTypeShareMediaURL(NetBaseNetType_SnapPicture_JPEG, szShareMediaURL,true );
		if (pClient)
		{//������ǰ����
			bResponseHttpFlag = false;
			pClient->SendFirstRequst();
		}
		else
		{
		    pClient = CreateNetRevcBaseClient(NetBaseNetType_SnapPicture_JPEG, 0, 0, m_addStreamProxyStruct.url, 0, szShareMediaURL);
			if (pClient != NULL)
			{
				memcpy((char*)&pClient->m_getSnapStruct, (char*)&m_getSnapStruct, sizeof(getSnapStruct));
				pClient->nClient_http = nClient; //��ֵ��http�������� 
				pClient->timeout_sec = atoi(m_getSnapStruct.timeout_sec);//��ʱ
				pClient->SendFirstRequst();
				WriteLog(Log_Debug, "����ץ�� nClient_http = %llu, nClient = %llu ",nClient,pClient->nClient);
			}
		}

	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
	}

	return true;
}

//��ѯͼƬ�б�
bool  CNetServerHTTP::index_api_queryPictureList()
{
	char szShareMediaURL[string_length_2048] = { 0 };

	memset((char*)&m_queryPictureListStruct, 0x00, sizeof(m_queryPictureListStruct));
	GetKeyValue("secret", m_queryPictureListStruct.secret);
	GetKeyValue("vhost", m_queryPictureListStruct.vhost);
	GetKeyValue("app", m_queryPictureListStruct.app);
	GetKeyValue("stream", m_queryPictureListStruct.stream);
	GetKeyValue("starttime", m_queryPictureListStruct.starttime);
	GetKeyValue("endtime", m_queryPictureListStruct.endtime);

	if (strlen(m_queryPictureListStruct.secret) == 0 || strlen(m_queryPictureListStruct.app) == 0 || strlen(m_queryPictureListStruct.stream) == 0 ||
		strlen(m_queryPictureListStruct.starttime) == 0 || strlen(m_queryPictureListStruct.endtime) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret , app , stream , starttime , endtime ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_queryPictureListStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strlen(m_queryPictureListStruct.starttime) != 14 || strlen(m_queryPictureListStruct.endtime) != 14)
	{//��ʼʱ�䡢����ʱ��ĳ��̼�� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( starttime , endtime ) Length Error ,length must 14 \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#ifdef USE_BOOST
	if (boost::all(m_queryPictureListStruct.starttime, boost::is_digit()) == false || boost::all(m_queryPictureListStruct.endtime, boost::is_digit()) == false)
	{//��⿪ʼʱ�䡢����ʱ�� �Ƿ������ֵ��ַ�����0 ~ 9��
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( starttime , endtime ) error , must is number \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#else
	if (ABL::is_digits(m_queryPictureListStruct.starttime) == false || ABL::is_digits(m_queryPictureListStruct.endtime) == false)
	{//��⿪ʼʱ�䡢����ʱ�� �Ƿ������ֵ��ַ�����0 ~ 9��
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( starttime , endtime ) error , must is number \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
#endif

	if (atoll(m_queryPictureListStruct.endtime) < atoll(m_queryPictureListStruct.starttime))
	{//����ʱ�������ڿ�ʼʱ��
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"endtime must greater than starttime \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//����ѯʱ�������ܳ���3��
	if (GetCurrentSecondByTime(m_queryPictureListStruct.endtime) - GetCurrentSecondByTime(m_queryPictureListStruct.starttime)  >  (3 * 24 * 3600))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"query times No more than 3 days \"}", IndexApiCode_ParamError, ABL_MediaServerPort.fileSecond);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//app ,stream �������ַ������治���� / 
	if (strstr(m_queryPictureListStruct.app, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"app parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strstr(m_queryPictureListStruct.stream, "/") != NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"stream parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//�ȼ��ж��Ƿ�洢ͼƬ
	sprintf(szShareMediaURL, "/%s/%s", m_queryPictureListStruct.app, m_queryPictureListStruct.stream);
	auto pPicture = GetPictureFileSource(szShareMediaURL,true);
	if (pPicture == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"count\":0,\"memo\":\"MediaSource ( app: %s , stream: %s ) PictureFile Not Found .\"}", IndexApiCode_OK, m_queryPictureListStruct.app, m_queryPictureListStruct.stream);
		ResponseSuccess(szResponseBody);
		return false;
	}

	memset(szMediaSourceInfoBuffer, 0x00, MaxMediaSourceInfoLength);
	int nMediaCount = queryPictureListByTime(szMediaSourceInfoBuffer, m_queryPictureListStruct);
	if (nMediaCount >= 0)
	{
		ResponseSuccess(szMediaSourceInfoBuffer);
	}

	return true;
}

/*����ͼƬjpg ,png ,bmp �ȵ� 
������
   char* szHttpURL  ͼƬ��ַ ���� /Media/Camera_00001/2022031917142607.jpg
*/
bool  CNetServerHTTP::index_api_downloadImage(char* szHttpURL)
{  
	string strImageFile = szHttpURL;
	char   szMediaURL[string_length_1024] = { 0 };
	char   szFileNumber[string_length_1024] = { 0 };
	int    nPos = 0;
	char   szImageFileName[string_length_2048] = { 0 };
	int    nImageFileSize = 0;
	bool   bSuccessFlag = false;
	string strHttpURL;

	nPos = strImageFile.rfind("/", strlen(szHttpURL));
	if (nPos > 0)
	{
		memcpy(szMediaURL, szHttpURL, nPos);
	    auto pPicture = GetPictureFileSource(szMediaURL,true);
		if (pPicture)
		{
			memcpy(szFileNumber, szHttpURL + nPos + 1, strlen(szHttpURL) - nPos - 5);
			if (pPicture->queryPictureFile(szFileNumber))
			{
				memmove(szHttpURL, szHttpURL + 1, strlen(szHttpURL) - 1);
				szHttpURL[strlen(szHttpURL) - 1] = 0x00;

#ifdef OS_System_Windows
				strHttpURL = szHttpURL;
#ifdef USE_BOOST
				replace_all(strHttpURL, "/", "\\");
#else
				ABL::replace_all(strHttpURL, "/", "\\");
#endif
				sprintf(szImageFileName, "%s%s", ABL_MediaServerPort.picturePath, strHttpURL.c_str());
 				struct _stat64 fileBuf;
				int error = _stat64(szImageFileName, &fileBuf);
				if (error == 0)
					nImageFileSize = fileBuf.st_size;
#else 
				sprintf(szImageFileName, "%s%s", ABL_MediaServerPort.picturePath, szHttpURL);

				struct stat fileBuf;
				int error = stat(szImageFileName, &fileBuf);
				if (error == 0)
					nImageFileSize = fileBuf.st_size;
#endif			 
				if (nImageFileSize > 0)
				{
					FILE* fImage = fopen(szImageFileName, "rb");
					if (fImage)
					{
						fread(szMediaSourceInfoBuffer, 1, nImageFileSize, fImage);
						fclose(fImage);
						bSuccessFlag = true;
						ResponseImage(nClient, HttpImageType_jpeg, (unsigned char*)szMediaSourceInfoBuffer, nImageFileSize, false);
					}
				}
 			}
		}
	}

	if (!bSuccessFlag)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Fille: %s Not Found \"}", IndexApiCode_RequestFileNotFound, szHttpURL);
		ResponseSuccess(szResponseBody);
	}

    if(ABL_MediaServerPort.httqRequstClose == 1)
	{
		//Sleep(10);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
	}

	return true;
}

/*  add by ZXT ��Ӧ��ˮӡ�Լ�ת������
url:http://127.0.0.1:7088/index/api/setTransFilter
parm:
	{
	"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc",
	"app" : "live",
	"stream" : "test",
	"text" : "ABL",
	"size" : 60,
	"color" : "red",
	"alpha" : 0.8,
	"left" : 40,
	"top" : 40,
	"trans" : 1
	}
*/
bool CNetServerHTTP::index_api_setTransFilter()
{
	char secret[string_length_256] = { 0 };
	char app[string_length_256] = { 0 };
	char stream[string_length_512] = { 0 };
	char text[string_length_2048] = { 0 };
	char size[string_length_256] = { 0 };
	char color[string_length_256] = { 0 };
	char alpha[string_length_256] = { 0 };
	char left[string_length_256] = { 0 };
	char top[string_length_256] = { 0 };
	char trans[string_length_256] = { 0 };

	GetKeyValue("secret", secret);
	GetKeyValue("app", app);
	GetKeyValue("stream", stream);
	GetKeyValue("text", text);
	GetKeyValue("size", size);
	GetKeyValue("color", color);
	GetKeyValue("alpha", alpha);
	GetKeyValue("left", left);
	GetKeyValue("top", top);
	GetKeyValue("trans", trans);

	if (strlen(secret) == 0 || strlen(app) == 0 || strlen(stream) == 0 || strlen(size) == 0 || strlen(color) == 0
		|| strlen(alpha) == 0 || strlen(left) == 0 || strlen(top) == 0 || strlen(trans) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\" parameter is not complete .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//��� app stream �Ƿ����
	char szShareMediaURL[string_length_512] = { 0 };
	sprintf(szShareMediaURL, "/%s/%s", app, stream);
	auto tmpMediaSource = GetMediaStreamSource(szShareMediaURL);
	if (tmpMediaSource == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"MediaSource: %s Not Found \"}", IndexApiCode_ParamError, szShareMediaURL);
		ResponseSuccess(szResponseBody);
		return false;
	}
	
	//����Ƿ���ת�빦�� 
	if (!(tmpMediaSource->m_h265ConvertH264Struct.H265ConvertH264_enable == 1 && ABL_MediaServerPort.filterVideo_enable == 1))
	{ 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Transcoding , VideoFilter has not been enabled . params ( H264DecodeEncode_enable , filterVideo_enable ) must be set 1 \"}", IndexApiCode_TranscodingVideoFilterNotEnable);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//�����ת�빦���Ƿ��Ѿ���Ч
	if ( !((tmpMediaSource->videoEncode.m_bInitFlag == true && tmpMediaSource->videoEncode.enableFilter == true) || tmpMediaSource->pFFVideoFilter != NULL) )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Transcoding , VideoFilter has not take effect . \"}", IndexApiCode_TranscodingVideoFilterTakeEffect);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���ò���
	bool transFlag = atoi(trans);
	//ת�����ȼ��ߣ�����ر�ת����ˮӡʧЧ; ת��򿪣�ˮӡ�ı�Ϊ��ҲˮӡʧЧ
	if (transFlag)
	{
        if(ABL_MediaServerPort.H265DecodeCpuGpuType == 0)
		{//��ת��
		  tmpMediaSource->videoEncode.ChangeVideoFilter(text, atoi(size), color, atof(alpha), atoi(left), atoi(top));
		}else  
		{//Ӳ��ת�� 
#ifdef OS_System_Windows
		  tmpMediaSource->videoEncode.ChangeVideoFilter(text, atoi(size), color, atof(alpha), atoi(left), atoi(top));
#else
          tmpMediaSource->ChangeVideoFilter(text, atoi(size), color, atof(alpha), atoi(left), atoi(top));
#endif			
		}
	}
 
	sprintf(szResponseBody, "{\"code\":0,\"memo\":\"success\"}");
	ResponseSuccess(szResponseBody);
	return true;
}

//���ƴ�����������
bool  CNetServerHTTP::index_api_controlStreamProxy()
{
	memset((char*)&m_controlStreamProxy, 0x00, sizeof(m_controlStreamProxy));
	GetKeyValue("secret", m_controlStreamProxy.secret);
	GetKeyValue("key", m_controlStreamProxy.key);
	GetKeyValue("command", m_controlStreamProxy.command);
	GetKeyValue("value", m_controlStreamProxy.value);
 
	if (strlen(m_controlStreamProxy.secret) == 0 || strlen(m_controlStreamProxy.key) == 0 || strlen(m_controlStreamProxy.command) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret , key , command ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(ABL_MediaServerPort.secret, m_controlStreamProxy.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	// command ����Ϊ pause , resume ,scale ,seek  
	if ( !(strcmp(m_controlStreamProxy.command, "pause") == 0 || strcmp(m_controlStreamProxy.command, "resume") == 0 || strcmp(m_controlStreamProxy.command, "scale") == 0 || strcmp(m_controlStreamProxy.command, "seek") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"command: %s parameter error\"}", IndexApiCode_ParamError, m_controlStreamProxy.command);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//��� value �Ƿ���д
	if (strcmp(m_controlStreamProxy.command, "scale") == 0 || strcmp(m_controlStreamProxy.command, "seek") == 0)
	{
		if (strlen(m_controlStreamProxy.value) == 0)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter: value must need \"}", IndexApiCode_ParamError);
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	/*��ⱶ�ٲ��ŵ�ֵ ���ٲ��ŵ�ֵ���ټ��  
	if (strcmp(m_controlStreamProxy.command, "scale") == 0 )
	{
		if (boost::all(m_controlStreamProxy.value, boost::is_digit()) == false)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter: value (%s) must number \"}", IndexApiCode_ParamError, m_controlStreamProxy.value);
			ResponseSuccess(szResponseBody);
			return false;
		}
	}*/

	auto pClientProxy = GetNetRevcBaseClient(atoi(m_controlStreamProxy.key));
	if (pClientProxy != NULL )
	{
 		if (pClientProxy->netBaseNetType == NetBaseNetType_addStreamProxyControl && pClientProxy->nMediaClient > 0 )
		{//CNetClientRecvRtsp
			auto pClientRtspPtr = GetNetRevcBaseClient(pClientProxy->nMediaClient);
 			if (pClientRtspPtr != NULL)
			{
				CNetClientRecvRtsp* pRtspClient = (CNetClientRecvRtsp*)pClientRtspPtr.get();
				if (strcmp(m_controlStreamProxy.command, "pause") == 0)
				{
					pRtspClient->RtspPause();
				}
				else if (strcmp(m_controlStreamProxy.command, "resume") == 0)
				{
					pRtspClient->RtspResume();
				}
				else if (strcmp(m_controlStreamProxy.command, "seek") == 0)
				{
					pRtspClient->RtspSeek(m_controlStreamProxy.value);
				}
				else if (strcmp(m_controlStreamProxy.command, "scale") == 0)
				{
					pRtspClient->RtspSpeed(m_controlStreamProxy.value);
				}
			   sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"command %s is success \"}", IndexApiCode_OK, m_controlStreamProxy.command);
			}else 
			   sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Key %s Not Found \"}", IndexApiCode_KeyNotFound, m_controlStreamProxy.key);
		}
		else
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Key %s is Not Rtsp \"}", IndexApiCode_ErrorRequest, m_controlStreamProxy.key);
		}
		ResponseSuccess(szResponseBody);
	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Key %s Not Found \"}", IndexApiCode_KeyNotFound, m_controlStreamProxy.key);
		ResponseSuccess(szResponseBody);
	}

	return true;
}

//�������ò���ֵ
bool   CNetServerHTTP::index_api_setConfigParamValue()
{
	char                 szTempURL[string_length_512] = { 0 };
	memset((char*)&m_setConfigParamValue, 0x00, sizeof(m_setConfigParamValue));
	GetKeyValue("secret", m_setConfigParamValue.secret);
	GetKeyValue("key", m_setConfigParamValue.key);
	GetKeyValue("value", szTempURL);
 	DecodeUrl(szTempURL, m_setConfigParamValue.value, sizeof(m_setConfigParamValue.value));
 
	if (strlen(m_setConfigParamValue.secret) == 0 || strlen(m_setConfigParamValue.key) == 0 || strlen(m_setConfigParamValue.value) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret , key , value ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(ABL_MediaServerPort.secret, m_setConfigParamValue.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(m_setConfigParamValue.key, "saveProxyRtspRtp") == 0 && (atoi(m_setConfigParamValue.value) == 0 || atoi(m_setConfigParamValue.value) == 1))
	{//����rtsp���������Ĳ���
		ABL_MediaServerPort.nSaveProxyRtspRtp = atoi(m_setConfigParamValue.value);
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s = %s update success. \"}", IndexApiCode_OK, m_setConfigParamValue.key, m_setConfigParamValue.value);
 	}else if (strcmp(m_setConfigParamValue.key, "saveGB28181Rtp") == 0 && (atoi(m_setConfigParamValue.value) == 0 || atoi(m_setConfigParamValue.value) == 1))
	{
		ABL_MediaServerPort.nSaveGB28181Rtp = atoi(m_setConfigParamValue.value);
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s = %s update success. \"}", IndexApiCode_OK, m_setConfigParamValue.key, m_setConfigParamValue.value);
	}
	else
	{
		if (WriteParamValue("ABLMediaServer", m_setConfigParamValue.key, m_setConfigParamValue.value))
		{
			ABL_MediaServerPort.nSaveGB28181Rtp = atoi(m_setConfigParamValue.value);
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s = %s update success. \"}", IndexApiCode_OK, m_setConfigParamValue.key, m_setConfigParamValue.value);
		}else
	      sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s = %s update failed. \"}", IndexApiCode_ErrorRequest, m_setConfigParamValue.key, m_setConfigParamValue.value);
 	}

	ResponseSuccess(szResponseBody);

	return true;
}

//�����������ò���
bool  CNetServerHTTP::index_api_setServerConfig()
{
	char                 szTempURL[string_length_512] = { 0 };
	char                 szSecret[string_length_256] = { 0 };
	char                 szSuccessParams[string_length_2048] = { 0 };
	char                 szFailedParams[string_length_2048] = { 0 };
	int                  nSuccessCount = 0;
	int                  nFailedCount = 0;

	GetKeyValue("secret", szSecret);

	if (strlen(szSecret) == 0 )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\" secret  parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (strcmp(ABL_MediaServerPort.secret, szSecret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	RequestKeyValue* pKey;
	RequestKeyValueMap::iterator it;

	for (it = requestKeyValueMap.begin(); it != requestKeyValueMap.end();++it)
	{
		pKey = (*it).second;
		if (strcmp(pKey->key, "secret") != 0)
		{
			memset(szTempURL, 0x00, sizeof(szTempURL));
			if(strlen(pKey->value) > 0 )
			  DecodeUrl(pKey->value, szTempURL, string_length_512);

			if (WriteParamValue("ABLMediaServer", pKey->key, szTempURL))
			{
				strcat(szSuccessParams, pKey->key);
				strcat(szSuccessParams," ");
				nSuccessCount ++;
				WriteLog(Log_Debug, "д����� %s=%s �ɹ�  ", pKey->key, szTempURL);
			}
			else
			{
			  WriteLog(Log_Debug, "д����� %s=%s ʧ��  ", pKey->key, szTempURL);
			  nFailedCount ++;
			  strcat(szFailedParams, pKey->key);
			  strcat(szFailedParams, " ");
			}
		}
 	}
  	sprintf(szResponseBody, "{\"code\":%d,\"changed\":%d,\"memo\":\"Params of successful modifications ( %s ) , Params of failed modifications ( %s ) \"}", IndexApiCode_OK, nSuccessCount, szSuccessParams, szFailedParams);
	ResponseSuccess(szResponseBody);

	return true;
}

//д�����ļ�
bool CNetServerHTTP::WriteParamValue(char* szSection, char* szKey, char* szValue)
{
	if (strcmp("secret", szKey) == 0)
		strcpy(ABL_MediaServerPort.secret, szValue);
	else if(strcmp("mediaServerID",szKey) == 0 )
		strcpy(ABL_MediaServerPort.mediaServerID, szValue); 
	else if (strcmp("localipAddress", szKey) == 0)
	{
		strcpy(ABL_MediaServerPort.ABL_szLocalIP, szValue);
		strcpy(ABL_szLocalIP, szValue);
	}
	else if (strcmp("maxTimeNoOneWatch", szKey) == 0)
		ABL_MediaServerPort.maxTimeNoOneWatch = atof(szValue);
	else if (strcmp("recordPath", szKey) == 0)
	{
	   WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu WriteParamValue() , szValue = %s  ", this, nClient, szValue);	
#ifdef OS_System_Windows
		if (szValue[strlen(szValue) - 1] != '\\')
		  strcat(szValue, "\\");
#else 
		if (szValue[strlen(szValue) - 1] != '/')
			strcat(szValue, "/");
#endif 
		strcpy(ABL_MediaServerPort.recordPath, szValue);
	    WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu WriteParamValue() ,׷�Ӻ� szValue = %s  ", this, nClient, szValue);			
	}
	else if (strcmp("picturePath", szKey) == 0)
	{
#ifdef OS_System_Windows
		if (szValue[strlen(szValue) - 1] != '\\')
			strcat(szValue, "\\");
#else 
		if (szValue[strlen(szValue) - 1] != '/')
			strcat(szValue, "/");
#endif 
		strcpy(ABL_MediaServerPort.picturePath, szValue);
	}
	else if (strcmp("maxSameTimeSnap", szKey) == 0)
		ABL_MediaServerPort.maxSameTimeSnap=atoi(szValue);
	else if (strcmp("snapOutPictureWidth", szKey) == 0)
		ABL_MediaServerPort.snapOutPictureWidth = atoi(szValue);
	else if (strcmp("snapOutPictureHeight", szKey) == 0)
		ABL_MediaServerPort.snapOutPictureHeight = atoi(szValue);
	else if (strcmp("snapObjectDestroy", szKey) == 0)
		ABL_MediaServerPort.snapObjectDestroy = atoi(szValue);
	else if (strcmp("snapObjectDuration", szKey) == 0)
		ABL_MediaServerPort.snapObjectDuration = atoi(szValue);
	else if (strcmp("captureReplayType", szKey) == 0)
		ABL_MediaServerPort.captureReplayType = atoi(szValue);
	else if (strcmp("pictureMaxCount", szKey) == 0)
		ABL_MediaServerPort.pictureMaxCount = atoi(szValue);
	else if (strcmp("pushEnable_mp4", szKey) == 0)
		ABL_MediaServerPort.pushEnable_mp4 = atoi(szValue);
	else if (strcmp("fileSecond", szKey) == 0)
		ABL_MediaServerPort.fileSecond = atoi(szValue);
	else if (strcmp("videoFileFormat", szKey) == 0)
		ABL_MediaServerPort.videoFileFormat = atoi(szValue);
	else if (strcmp("fileKeepMaxTime", szKey) == 0)
		ABL_MediaServerPort.fileKeepMaxTime = atoi(szValue);
	else if (strcmp("httpDownloadSpeed", szKey) == 0)
		ABL_MediaServerPort.httpDownloadSpeed = atoi(szValue);
	else if (strcmp("fileRepeat", szKey) == 0)
		ABL_MediaServerPort.fileRepeat = atoi(szValue);
	else if (strcmp("hook_enable", szKey) == 0)
		ABL_MediaServerPort.hook_enable = atoi(szValue);
	else if (strcmp("noneReaderDuration", szKey) == 0)
		ABL_MediaServerPort.noneReaderDuration = atoi(szValue);
	else if (strcmp("on_server_started", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_server_started, szValue);
	else if (strcmp("on_server_keepalive", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_server_keepalive, szValue);
	else if (strcmp("on_stream_arrive", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_stream_arrive, szValue);
	else if (strcmp("on_stream_not_arrive", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_stream_not_arrive, szValue);
	else if (strcmp("on_stream_none_reader", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_stream_none_reader, szValue);
	else if (strcmp("on_stream_disconnect", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_stream_disconnect, szValue);
	else if (strcmp("on_stream_not_found", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_stream_not_found, szValue);
	else if (strcmp("on_record_mp4", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_record_mp4, szValue);
	else if (strcmp("on_delete_record_mp4", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_delete_record_mp4, szValue);
	else if (strcmp("on_record_progress", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_record_progress, szValue);
	else if (strcmp("on_record_ts", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_record_ts, szValue);
	else if (strcmp("on_play", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_play, szValue);
	else if (strcmp("on_publish", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_publish, szValue);
	else if (strcmp("httpServerPort", szKey) == 0)
		ABL_MediaServerPort.nHttpServerPort = atoi(szValue);
	else if (strcmp("rtspPort", szKey) == 0)
		ABL_MediaServerPort.nRtspPort = atoi(szValue);
	else if (strcmp("rtmpPort", szKey) == 0)
		ABL_MediaServerPort.nRtmpPort = atoi(szValue);
	else if (strcmp("httpMp4Port", szKey) == 0)
		ABL_MediaServerPort.nHttpMp4Port = atoi(szValue);
	else if (strcmp("wsFlvPort", szKey) == 0)
		ABL_MediaServerPort.nWSFlvPort = atoi(szValue);
	else if (strcmp("httpFlvPort", szKey) == 0)
		ABL_MediaServerPort.nHttpFlvPort = atoi(szValue);
	else if (strcmp("ps_tsRecvPort", szKey) == 0)
		ABL_MediaServerPort.ps_tsRecvPort = atoi(szValue);
	else if (strcmp("hls_enable", szKey) == 0)
		ABL_MediaServerPort.nHlsEnable = atoi(szValue);
	else if (strcmp("hlsPort", szKey) == 0)
		ABL_MediaServerPort.nHlsPort = atoi(szValue);
	else if (strcmp("hlsCutTime", szKey) == 0)
		ABL_MediaServerPort.hlsCutTime = atoi(szValue);
	else if (strcmp("hlsCutType", szKey) == 0)
		ABL_MediaServerPort.nHLSCutType = atoi(szValue);
	else if (strcmp("h265CutType", szKey) == 0)
		ABL_MediaServerPort.nH265CutType = atoi(szValue);
	else if (strcmp("enable_audio", szKey) == 0)
		ABL_MediaServerPort.nEnableAudio = atoi(szValue);
	else if (strcmp("G711ConvertAAC", szKey) == 0)
		ABL_MediaServerPort.nG711ConvertAAC = atoi(szValue);
	else if (strcmp("IOContentNumber", szKey) == 0)
		ABL_MediaServerPort.nIOContentNumber = atoi(szValue);
	else if (strcmp("ThreadCountOfIOContent", szKey) == 0)
		ABL_MediaServerPort.nThreadCountOfIOContent = atoi(szValue);
	else if (strcmp("RecvThreadCount", szKey) == 0)
		ABL_MediaServerPort.nRecvThreadCount = atoi(szValue);
	else if (strcmp("SendThreadCount", szKey) == 0)
		ABL_MediaServerPort.nSendThreadCount = atoi(szValue);
	else if (strcmp("RecordReplayThread", szKey) == 0)
		ABL_MediaServerPort.nRecordReplayThread = atoi(szValue);
	else if (strcmp("GB28181RtpTCPHeadType", szKey) == 0)
		ABL_MediaServerPort.nGBRtpTCPHeadType = atoi(szValue);
	else if (strcmp("ReConnectingCount", szKey) == 0)
		ABL_MediaServerPort.nReConnectingCount = atoi(szValue);
	else if (strcmp("MaxDiconnectTimeoutSecond", szKey) == 0)
		ABL_MediaServerPort.MaxDiconnectTimeoutSecond = atoi(szValue);
	else if (strcmp("ForceSendingIFrame", szKey) == 0)
		ABL_MediaServerPort.ForceSendingIFrame = atoi(szValue);
	else if (strcmp("convertMaxObject", szKey) == 0)
		ABL_MediaServerPort.convertMaxObject = atoi(szValue);
	else if (strcmp("gb28181LibraryUse", szKey) == 0)
		ABL_MediaServerPort.gb28181LibraryUse = atoi(szValue);
	else if (strcmp("deleteSnapPicture", szKey) == 0)
		ABL_MediaServerPort.deleteSnapPicture = atoi(szValue);
	else if (strcmp("iframeArriveNoticCount", szKey) == 0)
		ABL_MediaServerPort.iframeArriveNoticCount = atoi(szValue);
	else if (strcmp("on_stream_iframe_arrive", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_stream_iframe_arrive, szValue);
	else if (strcmp("httqRequstClose", szKey) == 0)
		ABL_MediaServerPort.httqRequstClose = atoi(szValue);
	else if (strcmp("keepaliveDuration", szKey) == 0)
		ABL_MediaServerPort.keepaliveDuration = atoi(szValue);
	else if (strcmp("filterVideo_enable", szKey) == 0)
		ABL_MediaServerPort.filterVideo_enable = atoi(szValue);
	else if (strcmp("filterVideo_text", szKey) == 0)
		strcpy(ABL_MediaServerPort.filterVideoText, szValue);
	else if (strcmp("FilterFontSize", szKey) == 0)
		ABL_MediaServerPort.nFilterFontSize = atoi(szValue);
	else if (strcmp("FilterFontColor", szKey) == 0)
		strcpy(ABL_MediaServerPort.nFilterFontColor, szValue);
	else if (strcmp("FilterFontLeft", szKey) == 0)
		ABL_MediaServerPort.nFilterFontLeft = atoi(szValue);
	else if (strcmp("FilterFontTop", szKey) == 0)
		ABL_MediaServerPort.nFilterFontTop = atoi(szValue);
	else if (strcmp("FilterFontAlpha", szKey) == 0)
		ABL_MediaServerPort.nFilterFontAlpha = atof(szValue);
	else if (strcmp("convertOutWidth", szKey) == 0)
		ABL_MediaServerPort.convertOutWidth = atoi(szValue);
	else if (strcmp("convertOutHeight", szKey) == 0)
		ABL_MediaServerPort.convertOutHeight = atoi(szValue);
	else if (strcmp("convertOutBitrate", szKey) == 0)
		ABL_MediaServerPort.convertOutBitrate = atoi(szValue);
	else if (strcmp("webrtcPort", szKey) == 0)
	   ABL_MediaServerPort.nWebRtcPort = atoi(szValue);
	else if (strcmp("WsRecvPcmPort", szKey) == 0)
		ABL_MediaServerPort.WsRecvPcmPort = atoi(szValue);
	else if (strcmp("flvPlayAddMute", szKey) == 0)
		ABL_MediaServerPort.flvPlayAddMute = atoi(szValue);
	else if (strcmp("GB28181RtpMinPort", szKey) == 0)
		ABL_MediaServerPort.GB28181RtpMinPort = atoi(szValue);
	else if (strcmp("GB28181RtpMaxPort", szKey) == 0)
		ABL_MediaServerPort.GB28181RtpMaxPort = atoi(szValue);
	else if (strcmp("recordFileCutType", szKey) == 0)
		ABL_MediaServerPort.recordFileCutType = atoi(szValue);
	else if (strcmp("on_rtsp_replay", szKey) == 0)
		strcpy(ABL_MediaServerPort.on_rtsp_replay, szValue);
	else if (strcmp("1078Port", szKey) == 0)
		ABL_MediaServerPort.n1078Port = atoi(szValue);
	else if (strcmp("jtt1078Version", szKey) == 0)
		ABL_MediaServerPort.jtt1078Version = atoi(szValue);
	else if (strcmp("enable_GetFileDuration", szKey) == 0)
		ABL_MediaServerPort.enable_GetFileDuration = atoi(szValue);
 	else if (strcmp("wwwPath", szKey) == 0)
	{
 		if(strlen(szValue) == 0)
		  strcpy(ABL_wwwMediaPath,ABL_MediaSeverRunPath);
		else
		{
		  strcpy(ABL_MediaServerPort.wwwPath, szValue);
		}

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
		ABL_SetPathAuthority(ABL_wwwMediaPath);
#endif
	}
	else
		return false;
	
	ABL_ConfigFile.SetValue(szSection, szKey, szValue);
	SI_Error  rc = ABL_ConfigFile.SaveFile(szConfigFileName);

	if (rc == SI_OK)
	{
		return true;
	}
	else
	{
		return false;
	} 
}

//�˳���ý�������
bool   CNetServerHTTP::index_api_shutdownServer()
{
	char  szSecret[256] = { 0 };
	GetKeyValue("secret", szSecret);
 
	if (strlen(szSecret) == 0 )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(ABL_MediaServerPort.secret, szSecret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"ABLMediaServer shutdown Successed ! \"}", IndexApiCode_OK);
 	ResponseSuccess(szResponseBody);

	WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu index_api_shutdownServer() , %s  ", this, nClient, szResponseBody);
	//Sleep(1000);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	ABL_bMediaServerRunFlag = false;

	return true;
}

//������ý�������
bool   CNetServerHTTP::index_api_restartServer()
{
	char  szSecret[string_length_256] = { 0 };
	GetKeyValue("secret", szSecret);

	if (strlen(szSecret) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret  parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(ABL_MediaServerPort.secret, szSecret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"ABLMediaServer restartServer Successed ! \"}", IndexApiCode_OK);
	ResponseSuccess(szResponseBody);

	WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu index_api_restartServer() , %s  ", this, nClient, szResponseBody);
	//Sleep(1500);
	std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	ABL_bMediaServerRunFlag = false;
	ABL_bRestartServerFlag = true;

	return true;
}

//��ȡ��ǰת������
bool  CNetServerHTTP::index_api_getTranscodingCount()
{
	char  szSecret[string_length_512] = { 0 };
	GetKeyValue("secret", szSecret);

	if (strlen(szSecret) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\" secret parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(ABL_MediaServerPort.secret, szSecret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	sprintf(szResponseBody, "{\"code\":%d,\"currentTranscodingCount\":%d}", IndexApiCode_OK, CMediaStreamSource::nConvertObjectCount);
	ResponseSuccess(szResponseBody);

	WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu index_api_getTranscodingCount() , %s  ", this, nClient, szResponseBody);

	return true;
}

//��ȡ������ռ�ö˿�
bool CNetServerHTTP::index_api_listServerPort()
{
	memset((char*)&m_listServerPortStruct, 0x00, sizeof(m_listServerPortStruct));

	GetKeyValue("secret", m_listServerPortStruct.secret);
	GetKeyValue("vhost", m_listServerPortStruct.vhost);
	GetKeyValue("app", m_listServerPortStruct.app);
	GetKeyValue("stream", m_listServerPortStruct.stream);

	if (strcmp(m_listServerPortStruct.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	memset(szMediaSourceInfoBuffer, 0x00, MaxMediaSourceInfoLength);
	int nMediaCount = GetALLListServerPort(szMediaSourceInfoBuffer, m_listServerPortStruct);
	if (nMediaCount >= 0)
	{
		ResponseSuccess(szMediaSourceInfoBuffer);
	}

	return true;
}

//������ͣ������
bool  CNetServerHTTP::RequesePauseRtpServer(bool bFlag)
{
	memset((char*)&m_pauseResumeRtpServer, 0x00, sizeof(m_pauseResumeRtpServer));
	GetKeyValue("secret", m_pauseResumeRtpServer.secret);
	GetKeyValue("key", m_pauseResumeRtpServer.key);

	if (strcmp(m_pauseResumeRtpServer.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���Key ��ֵ
	if (strlen(m_pauseResumeRtpServer.key) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"key parameter error\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	auto pClient = GetNetRevcBaseClient(atoi(m_pauseResumeRtpServer.key));
	if (pClient == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"key %s Not Found .\"}", IndexApiCode_KeyNotFound, m_pauseResumeRtpServer.key);
		ResponseSuccess(szResponseBody);
		return false;
	}
	else
	{
		WriteLog(Log_Debug, "CNetServerHTTP = %X  nClient = %llu  ִ�й��� m_bPauseFlag = %d  ", this, atoi(m_pauseResumeRtpServer.key), bFlag);
		if (pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerUDP || pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerTCP_Active)
		{
			pClient->m_bPauseFlag = bFlag;
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"success\"}", IndexApiCode_OK);
		}
		else if (pClient->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen)
		{
			auto pMediaClient = GetNetRevcBaseClient(pClient->nMediaClient);
			if (pMediaClient != NULL)
			{
				pMediaClient->m_bPauseFlag = bFlag;
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"success\"}", IndexApiCode_OK);
			}else
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"success\"}", IndexApiCode_ErrorRequest);
		}
		else
 			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"success\"}", IndexApiCode_ErrorRequest);
 
		ResponseSuccess(szResponseBody);
		return true;
	}
}

//������ͣ
bool  CNetServerHTTP::index_api_pauseRtpServer()
{
	return RequesePauseRtpServer(true);
}

//�������
bool  CNetServerHTTP::index_api_resumeRtpServer()
{
	return RequesePauseRtpServer(false);
}

//���͵�һ������
int CNetServerHTTP::SendFirstRequst()
{
  	return 0;
}

//����m3u8�ļ�
bool  CNetServerHTTP::RequestM3u8File()
{
	return true;
}

//���ƴ�����������
bool  CNetServerHTTP::index_api_controlRecordPlay()
{
	memset((char*)&m_controlStreamProxy, 0x00, sizeof(m_controlStreamProxy));
	GetKeyValue("secret", m_controlStreamProxy.secret);
	GetKeyValue("key", m_controlStreamProxy.key);
	GetKeyValue("command", m_controlStreamProxy.command);
	GetKeyValue("value", m_controlStreamProxy.value);
	uint64_t             nTotalSecond = 1 ;

	if (strlen(m_controlStreamProxy.secret) == 0 || strlen(m_controlStreamProxy.key) == 0 || strlen(m_controlStreamProxy.command) == 0)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( secret , key , command ) parameter need .\"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	if (strcmp(ABL_MediaServerPort.secret, m_controlStreamProxy.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	// command ����Ϊ pause , resume ,scale ,seek  
	if (!(strcmp(m_controlStreamProxy.command, "pause") == 0 || strcmp(m_controlStreamProxy.command, "resume") == 0 || strcmp(m_controlStreamProxy.command, "scale") == 0 || strcmp(m_controlStreamProxy.command, "seek") == 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"command: %s parameter error\"}", IndexApiCode_ParamError, m_controlStreamProxy.command);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//��� value �Ƿ���д
	if (strcmp(m_controlStreamProxy.command, "scale") == 0 || strcmp(m_controlStreamProxy.command, "seek") == 0)
	{
		if (strlen(m_controlStreamProxy.value) == 0)
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"parameter: value must need \"}", IndexApiCode_ParamError);
			ResponseSuccess(szResponseBody);
			return false;
		}
	}

	auto pClientProxy = GetNetRevcBaseClient(atoi(m_controlStreamProxy.key));
	if (pClientProxy != NULL)
	{
		if (pClientProxy->netBaseNetType == NetBaseNetType_NetServerReadMultRecordFile )
		{//CNetServerReadMultRecordFile
			CNetServerReadMultRecordFile* pClientRecordPtr = (CNetServerReadMultRecordFile*)pClientProxy.get();
			if (pClientRecordPtr != NULL)
			{ 
 				if (strcmp(m_controlStreamProxy.command, "pause") == 0)
				{
					pClientRecordPtr->UpdatePauseFlag(true);
				}else if (strcmp(m_controlStreamProxy.command, "resume") == 0)
				{
					pClientRecordPtr->UpdatePauseFlag(false);
				}
 				else if (strcmp(m_controlStreamProxy.command, "seek") == 0)
				{
					if (atoi(m_controlStreamProxy.value) <= 0)
					{
						sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"value: %s parameter error\"}", IndexApiCode_ParamError, m_controlStreamProxy.value);
						ResponseSuccess(szResponseBody);
						return false;
					}

					nTotalSecond = GetCurrentSecondByTime(pClientRecordPtr->m_queryRecordListStruct.endtime) - GetCurrentSecondByTime(pClientRecordPtr->m_queryRecordListStruct.starttime);
					if (atoll(m_controlStreamProxy.value) > nTotalSecond)
					{
						sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Seek time ( %s second )  is greater than the total recording time ( %llu second . From  %s to %s ) \"}", IndexApiCode_secretError, m_controlStreamProxy.value, nTotalSecond, pClientRecordPtr->m_queryRecordListStruct.starttime, pClientRecordPtr->m_queryRecordListStruct.endtime);
						ResponseSuccess(szResponseBody);
						return false;
					}
					pClientRecordPtr->ReaplyFileSeek(atoll(m_controlStreamProxy.value));
				}
				else if (strcmp(m_controlStreamProxy.command, "scale") == 0)
				{
					pClientRecordPtr->UpdateReplaySpeed(atof(m_controlStreamProxy.value), RtspPlayerType_RecordReplay);
				}
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"command %s Execution successful \"}", IndexApiCode_OK, m_controlStreamProxy.command);
			}
			else
				sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Key %s Not Found \"}", IndexApiCode_KeyNotFound, m_controlStreamProxy.key);
		}
		else
		{
			sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Key %s is Not Rtsp \"}", IndexApiCode_ErrorRequest, m_controlStreamProxy.key);
		}
		ResponseSuccess(szResponseBody);
	}
	else
	{//�������� 
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Key %s Not Found \"}", IndexApiCode_KeyNotFound, m_controlStreamProxy.key);
		ResponseSuccess(szResponseBody);
	}

	return true;
}

//����1078�Խ���Ƶ����
bool  CNetServerHTTP::index_api_sendJtt1078Talk()
{
	memset((char*)&m_sendJtt1078Talk, 0x00, sizeof(m_sendJtt1078Talk));

	GetKeyValue("secret", m_sendJtt1078Talk.secret);
	GetKeyValue("vhost", m_sendJtt1078Talk.vhost);
	GetKeyValue("app", m_sendJtt1078Talk.app);
	GetKeyValue("stream", m_sendJtt1078Talk.stream);
	GetKeyValue("send_app", m_sendJtt1078Talk.send_app);
	GetKeyValue("send_stream_id", m_sendJtt1078Talk.send_stream_id);

	if (strcmp(m_sendJtt1078Talk.secret, ABL_MediaServerPort.secret) != 0)
	{//������
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"secret error\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}
	if (!(strlen(m_sendJtt1078Talk.app) > 0 && strlen(m_sendJtt1078Talk.stream) > 0 && strlen(m_sendJtt1078Talk.send_app) > 0 && strlen(m_sendJtt1078Talk.send_stream_id) > 0))
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"( app , stream, send_app ,send_stream_id) All values must be specified\"}", IndexApiCode_secretError);
		ResponseSuccess(szResponseBody);
		return false;
	}
  
	char szURL[string_length_512] = { 0 };
	sprintf(szURL, "/%s/%s", m_sendJtt1078Talk.app, m_sendJtt1078Talk.stream);
	auto tmpMediaSource1 = GetMediaStreamSource(szURL);
	if (tmpMediaSource1 == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s 1078 device intercom voice stream has not been connected yet\"}",IndexApiCode_MediaSourceNotFound, szURL);
		ResponseSuccess(szResponseBody);
		return false;
	}

	sprintf(szURL, "/%s/%s", m_sendJtt1078Talk.send_app, m_sendJtt1078Talk.send_stream_id);
	auto  tmpMediaSource2 = GetMediaStreamSource(szURL);
	if (tmpMediaSource2 == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s The voice to be sent is not ready\"}", IndexApiCode_MediaSourceNotFound, szURL);
		ResponseSuccess(szResponseBody);
		return false;
	}

	sprintf(szURL, "/%s/%s", m_sendJtt1078Talk.app, m_sendJtt1078Talk.stream);
	auto  pClient =  GetNetRevcBaseClientByNetTypeShareMediaURL(NetBaseNetType_NetGB28181RtpServerTCP_Server, szURL, true);
	if (pClient == NULL)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s Object not found \"}", IndexApiCode_KeyNotFound, szURL);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���ö����Ƿ�Ϊ����1078�豸
	if (pClient->m_openRtpServerStruct.RtpPayloadDataType[0] != 0x34)
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s The connected device is not a 1078 car mounted device \"}", IndexApiCode_ParamError, szURL);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//����Ƿ��Ѿ����͹��Խ���Ƶ
	if (pClient->addThreadPoolFlag == true )
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Voice intercom has already been sent \"}", IndexApiCode_ParamError);
		ResponseSuccess(szResponseBody);
		return false;
	}

	//���������Խ�
	char szMediaSource[string_length_1024] = { 0 };
	sprintf(szMediaSource, "/%s/%s", m_sendJtt1078Talk.send_app, m_sendJtt1078Talk.send_stream_id);
	auto pSendMediaSource = GetMediaStreamSource(szMediaSource);
	if (pSendMediaSource)
	{
		pClient->addThreadPoolFlag = true;
		pSendMediaSource->AddClientToMap(pClient->nClient);

		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"Successfully sent voice intercom audio \"}", IndexApiCode_OK);
		ResponseSuccess(szResponseBody);
		return false;
	}
	else
	{
		sprintf(szResponseBody, "{\"code\":%d,\"memo\":\"%s Object not found \"}", IndexApiCode_KeyNotFound, szMediaSource);
		ResponseSuccess(szResponseBody);
		return false;
	}
 
	return true;
}

//���˿��Ƿ������ļ��еĶ˿�ռ�ã��Ƿ����ö˿�
bool CNetServerHTTP::CheckPortIfUse(int nPort)
{
	if (ABL_MediaServerPort.nHttpServerPort == nPort || ABL_MediaServerPort.nRtspPort     == nPort || ABL_MediaServerPort.nRtmpPort    == nPort ||
		ABL_MediaServerPort.nHttpFlvPort    == nPort || ABL_MediaServerPort.nWSFlvPort    == nPort || ABL_MediaServerPort.nHttpMp4Port == nPort ||
		ABL_MediaServerPort.ps_tsRecvPort   == nPort || ABL_MediaServerPort.WsRecvPcmPort == nPort || ABL_MediaServerPort.n1078Port    == nPort ||
		ABL_MediaServerPort.nHlsPort        == nPort)
	{
		WriteLog(Log_Debug, "nPort = %d ,�Ѿ��������ļ�ʹ�ã�Ҳ����ϵͳ�����Ѿ��󶨸ö˿�", nPort);
		return true;
	}
	return false;
}