#ifndef _NetClientRecvRtmp_H
#define _NetClientRecvRtmp_H

#include "rtmp-client.h"
#include "flv-writer.h"
#include "flv-proto.h"
#include "flv-demuxer.h"
#include "flv-muxer.h"

#include "MediaStreamSource.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else
#include <memory>
#endif


//#define  WriteFlvFileByDebug   1 //用于定义是否写FLV文件 

//#define    WriteFlvToEsFileFlag  1 //用于写FLV解包后的ES流，测试flv解包视频、音频是否正确 

class CNetClientRecvRtmp : public CNetRevcBase
{
public:
	CNetClientRecvRtmp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientRecvRtmp();

	uint32_t       nVideoDTS;
	uint32_t       nAudioDTS;
	uint32_t       nWriteRet;
	volatile  int  nWriteErrorCount;
	char           szRtmpName[256];

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//塞入视频数据
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//塞入音频数据
	virtual int SendVideo();//发送视频数据
	virtual int SendAudio();//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	bool                         GetAppStreamByURL(char* app, char* stream);
	struct rtmp_client_handler_t handler;
	rtmp_client_t* rtmp;
	volatile bool                bCheckRtspVersionFlag;

	flv_demuxer_t* flvDemuxer;
	char                         szURL[string_length_2048];
#ifdef USE_BOOST
	boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
	std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif
	volatile bool                         bDeleteRtmpPushH265Flag; //因为推rtmp265被删除标志 

#ifdef  WriteFlvFileByDebug
	void* s_flv;
#endif

#ifdef  WriteFlvToEsFileFlag
	FILE* fWriteVideo;
	FILE* fWriteAudio;
#endif
};

#endif