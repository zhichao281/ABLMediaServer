#ifndef _CNetServerHLS_H
#define _CNetServerHLS_H

#define  MaxHttp_FlvNetCacheBufferLength    1024*32 
#define  Send_TsFile_MaxPacketCount         1024*64  //TS 最大发送字节数量
#define  MaxDefaultTsFmp4FileByteCount      1024*1024*5 //缺省TS、FMP4文件字节大小 

class CNetServerHLS : public CNetRevcBase
{
public:
	CNetServerHLS(NETHANDLE hServer,NETHANDLE hClient,char* szIP,unsigned short nPort, char* szShareMediaURL);
	~CNetServerHLS();
   
    virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
	virtual int ProcessNetData() ;

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据

	virtual int SendVideo() ;//发送视频数据
	virtual int SendAudio() ;//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	int          SendLiveHLS();//发送实况的hls 
	int          SendRecordHLS();//发送录像回放的hls 

private :
	unsigned char*          pTsFileBuffer;//读取TS、FMP4文件缓存
	int                     nCurrentTsFileBufferSize; //当前TS、FMP4缓存字节大小 

	bool                    ReadHttpRequest();
	char                    szHttpRequestBuffer[string_length_4096];

	std::mutex              netDataLock;
 	char                    szCookieNumber[64];
	char                    szDateTime1[64];
	char                    szDateTime2[64];
	bool                    bRequestHeadFlag; //采用head请求方式 
	char                    szOrigin[string_length_2048];//来源
	int64_t                 GetTsFileNameOrder(char* szTsFileName);
	int64_t                 nTsFileNameOrder; //TS切片文件序号 0 ，1 ，2 ，3 ~ N
	char                    szConnectionType[64];//Close、 Keep-live
	CABLSipParse            httpParse;
	unsigned long           fFileByteCount;
	bool                    GetHttpRequestFileName(char* szGetRequestFile, char* szHttpHeadData);
	int                     nWriteRet, nWriteRet2;

	char                    httpResponseData[string_length_4096];
	char                    szM3u8Content[string_length_512K];
	unsigned char           netDataCache[MaxHttp_FlvNetCacheBufferLength+4]; //网络数据缓存
	int                     netDataCacheLength;//网络数据缓存大小
	int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
	int                     MaxNetDataCacheCount;
 	int                     data_Length;
	char                    szPushName[string_length_4096];//hls 对于的推流名字
	char                    szRequestFileName[string_length_4096];//http请求的文件名字 
	char                    szReadFileName[string_length_4096];
	volatile bool           bFindHLSNameFlag;
};

#endif