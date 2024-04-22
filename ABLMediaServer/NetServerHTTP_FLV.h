#ifndef _CNetServerHTTP_FLV_H
#define _CNetServerHTTP_FLV_H

#include "flv-proto.h"
#include "flv-muxer.h"

#define  MaxHttp_FlvNetCacheBufferLength    1024*32 

//#define  WriteHttp_FlvFileFlag              1  //写FLV文件，用于调试 

class CNetServerHTTP_FLV : public CNetRevcBase
{
public:
	CNetServerHTTP_FLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetServerHTTP_FLV();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//塞入视频数据
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//塞入音频数据

	virtual int SendVideo();//发送视频数据
	virtual int SendAudio();//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	int64_t        nPrintTime;

	std::mutex     NetServerHTTP_FLVLock;
	flv_muxer_t* flvMuxer;
	void* flvWrite;
	volatile uint32_t   nWriteRet;
	volatile int        nWriteErrorCount;
	CABLSipParse   flvParse;
private:
	volatile  bool bCheckHttpFlvFlag; //检测是否为http-flv协议 

	void                    MuxerVideoFlV(char* codeName, unsigned char* pVideo, int nLength);
	void                    MuxerAudioFlV(char* codeName, unsigned char* pAudio, int nLength);

	char                    httpResponseData[512];
	unsigned char           netDataCache[MaxHttp_FlvNetCacheBufferLength]; //网络数据缓存
	int                     netDataCacheLength;//网络数据缓存大小
	int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
	int                     MaxNetDataCacheCount;
	int                     data_Length;
	char                    szFlvName[string_length_2048];
	volatile bool           bFindFlvNameFlag;
};

#endif