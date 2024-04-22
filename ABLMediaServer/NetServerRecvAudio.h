#ifndef _CCNetServerRecvAudio_H
#define _CCNetServerRecvAudio_H

typedef enum {
	WDT_MINDATA = -20, // 0x0：标识一个中间数据包
	WDT_TXTDATA = -19, // 0x1：标识一个text类型数据包
	WDT_BINDATA = -18, // 0x2：标识一个binary类型数据包
	WDT_DISCONN = -17, // 0x8：标识一个断开连接类型数据包
	WDT_PING = -16, // 0x8：标识一个断开连接类型数据包
	WDT_PONG = -15, // 0xA：表示一个pong类型数据包
	WDT_ERR = -1,
	WDT_NULL = 0
}WebsocketData_Type;

//#define  WritePCMDaFile               1             //写pcm数据

#define  MaxPcmChacheBufferLength     1024*512
#define  SplitterMaxPcmLength_G711    1280          //g711a\g711u切割最大pcm数据长度 
#define  SplitterMaxPcmLength_AAC     2048          //aac切割最大pcm数据长度 

#pragma pack(1)

struct WebSocketPCMHead
{
	unsigned char   head[4];
	unsigned char   nType;
	unsigned short  nLength;
};

//音频注册包
struct AudioRegisterStruct
{
	char  method[128];   //方法 regiser , 
	char  app[string_length_256];
	char  stream[string_length_512];
	char  audioCodec[128];//原始音频格式
	short channels;
	int   sampleRate;
	char  targetAudioCodec[128];//目标音频格式 
};

#pragma pack()

class CNetServerRecvAudio : public CNetRevcBase
{
public:
	CNetServerRecvAudio(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetServerRecvAudio();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//塞入视频数据
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//塞入音频数据

	virtual int SendVideo();//发送视频数据
	virtual int SendAudio();//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	int             webSocket_dePackage(unsigned char* data, unsigned int dataLen, unsigned char* package, unsigned int packageMaxLen, unsigned int* packageLen, unsigned int* packageHeadLen);

	CAACEncode       aacEnc;
	int              nAACEncodeLength;
	int              nRetunEncodeLength;
#ifdef USE_BOOST
	boost::shared_ptr<CMediaStreamSource> pMediaSouce;
#else
	std::shared_ptr<CMediaStreamSource> pMediaSouce;
#endif

	unsigned char   szG711Encodec[1024];
	unsigned char   pPcmCacheBuffer[MaxPcmChacheBufferLength];
	int             nPcmCacheBufferLength;
	void            ProcessPcmCacheBuffer();
	AudioRegisterStruct audioRegisterStruct;
	WebSocketPCMHead  pcmHead;
	unsigned short    nPcmLength;
	unsigned char     pPcmSplitterBuffer[MaxPcmChacheBufferLength];
	unsigned short    nPcmSplitterLength;
	unsigned short    nSplitterCount;
	SwrContext* swr_ctx;
	void              AudioPcmResamle(unsigned char* inPCM, int nPcmDataLength);
	int               nInChannels;
	int               in_channel_layout;
	int               out_buffer_size;
	unsigned int      out_ch_layout;
	int               out_channel_nb;
	int               outSampleRate;
	unsigned short    pPcmForEncodeLength;
	unsigned short    nResampleLength;
	uint8_t** pcm_buffer;
	uint8_t** pcm_OutputBuffer;
	int               src_linesize;
	int               dst_linesize;
	int               SplitterMaxPcmLength;

#ifdef WritePCMDaFile
	uint64_t        nWritePCMCount;
	FILE* fWritePCMFile;
#endif

	virtual bool    SendWebSocketData(unsigned char* pData, int nDataLength);
	unsigned char   webSocketHead[64];
	unsigned short  wsLength16;
	uint32_t        wsLenght64;
	int             nWriteRet, nWriteRet2;

	int            m_nMaxRecvBufferLength;//允许最大接收字节数量
	unsigned char* dePacket;
	unsigned int   packageLen;
	unsigned int   packageHeadLen;
	CABLSipParse   wsParse;

	bool           Create_WS_FLV_Handle();
	int            nWebSocketCommStatus;

	std::mutex     NetServerWS_FLVLock;

private:
	volatile  bool         bCheckHttpFlvFlag; //检测是否为http-flv协议 

	char                    szSec_WebSocket_Key[256];
	char                    szWebSocketResponse[512];
	char                    szSec_WebSocket_Protocol[256];

	char                    httpResponseData[512];
	unsigned char* netDataCache; //网络数据缓存
	int                     netDataCacheLength;//网络数据缓存大小
	int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
	int                     MaxNetDataCacheCount;
	int                     data_Length;
	char                    szFlvName[string_length_2048];
	volatile bool           bFindFlvNameFlag;
};

#endif