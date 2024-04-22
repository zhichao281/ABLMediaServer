#ifndef _NetClientRecvHttpHLS_H
#define _NetClientRecvHttpHLS_H

#include "mpeg-ps.h"
#include "mpeg-ts.h"


#define   DefaultM3u8Number                 -88888 
#define  MaxHttp_HLSCNetCacheBufferLength   1024*128 
#define  MaxDefaultContentBodyLength        1024*1024*3 //缺省长度
#define  TsStreamBlockBufferLength          188         //TS流数据块长度
#define  MaxDefaultMediaFifoLength          1024*1024*12 //HLS视频缓存长度

//#define  SaveAudioToAACFile                 1
//#define  SaveTSBufferToFile                 1 //保存TS到文件

enum HLSRequestFileStatus
{
	HLSRequestFileStatus_NoRequsetFile   = 0, //未执行请求
	HLSRequestFileStatus_SendRequest     = 1,//已经发出请求
	HLSRequestFileStatus_RecvHttpHead    = 2,//收到Http头
	HLSRequestFileStatus_RequestSuccess  = 3,//接收完整
};

struct HistoryM3u8
{
	int64_t nRecvTime;
	char   szM3u8Data[512];

	HistoryM3u8()
	{
		nRecvTime = 0;
		memset(szM3u8Data, 0x00, sizeof(szM3u8Data));
	}
};

typedef list<HistoryM3u8> HistoryM3u8List;

class CNetClientRecvHttpHLS : public CNetRevcBase
{
public:
	CNetClientRecvHttpHLS(NETHANDLE hServer,NETHANDLE hClient,char* szIP,unsigned short nPort, char* szShareMediaURL);
	~CNetClientRecvHttpHLS();
   
    virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
	virtual int ProcessNetData() ;

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据

	virtual int SendVideo() ;//发送视频数据
	virtual int SendAudio() ;//发送音频数据

	virtual int  SendFirstRequst() ;//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	void         AddAdtsToAACData(unsigned char* szData, int nAACLength);

	int64_t         nOldPTS;
	uint64_t        nCallBackVideoTime;
	ts_demuxer_t *   ts;
	char             szSourceURL[string_length_2048];
#ifdef USE_BOOST
	boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
	std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

	CMediaFifo       hlsVideoFifo;
	CMediaFifo       hlsAudioFifo;

	volatile bool    bExitCallbackThreadFlag ;
	unsigned char    aacData[string_length_2048]; 
#ifdef SaveAudioToAACFile
	FILE*           fileSaveAAC;
#endif
private :
#ifdef  SaveTSBufferToFile
	int64_t                 nTsFileOrder;
#endif
	HistoryM3u8List         historyM3u8List;
	bool                    FindTsFileAtHistoryList(char* szTsFile);

	volatile   int          nHLSRequestFileStatus;//请求文件状态
	int64_t                 nRequestM3u8Time;//最后一次发送m3u8文件时间
	int64_t                 nSendTsFileTime;//最后一次请求视频文件时间
    volatile  bool          bCanRequestM3u8File;//允许请求m3u8文件 

	bool                    AddM3u8ToFifo(char* szM3u8Data, int nDataLength);
	CABLSipParse            httpParse;
	std::mutex              netDataLock;
	unsigned char           szResponseHead[string_length_2048];//http响应头
	char                    szRequestFile[string_length_2048];

	int                     nContentLength; //实际长度
	int                     nRecvContentLength;//已经收到的长度
	volatile  bool          bRecvHttpHeadFlag;//已经接收完毕Http 头
	unsigned   char*        pContentBody;//内容 
	int                     nContentBodyLength;//ContentBody Buffer  长度 

	unsigned char           netDataCache[MaxHttp_HLSCNetCacheBufferLength + 4]; //网络数据缓存
	int                     netDataCacheLength;//网络数据缓存大小
	int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
	int                     MaxNetDataCacheCount;

	char                    szHttpURL[string_length_2048];
	char                    szRequestM3u8File[string_length_2048];
	char                    szRequestBuffer[string_length_2048];

	CMediaFifo              requestFileFifo;
	int64_t                 nOldRequestM3u8Number;
	int                     nAudioSize;
};

#endif