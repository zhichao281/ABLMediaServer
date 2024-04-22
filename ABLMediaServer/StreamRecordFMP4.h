#ifndef _StreamRecordFMP4_H
#define _StreamRecordFMP4_H

#include "hls-fmp4.h"
#include "mpeg-ps.h"
#include "hls-m3u8.h"
#include "hls-media.h"
#include "hls-param.h"
#include "mpeg-ps.h"
#include "mov-format.h"
#include "mpeg-ts.h"


#include "mov-format.h"
#include "fmp4-writer.h"

#include "MediaFifo.h"
#include "mpeg4-hevc.h"
#include "mpeg4-aac.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif

class CStreamRecordFMP4 : public CNetRevcBase
{
public:
	CStreamRecordFMP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CStreamRecordFMP4();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//塞入视频数据
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//塞入音频数据
	virtual int SendVideo();//发送视频数据
	virtual int SendAudio();//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	char                    szFileNameOrder[64];
	bool                    writeTSBufferToMP4File(unsigned char* pTSData, int nLength);
	char                    szFileName[512];

	volatile    bool        bWaitIFrameSuccessFlag;
	volatile    bool        bAddSendThreadToolFlag;
	char                    httpResponseData[string_length_4096];
	unsigned char           netDataCache[1024 * 32]; //网络数据缓存
	int                     netDataCacheLength;//网络数据缓存大小
	int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
	int                     MaxNetDataCacheCount;
	int                     data_Length;
	char                    szMP4Name[512];
	volatile bool           bFindMP4NameFlag;
	volatile  bool          bCheckHttpMP4Flag; //检测是否为http-MP4协议 
	int                     nWriteRet;

	FILE* fWriteMP4;
	std::mutex            mediaMP4MapLock;
	int                   avtype;

	unsigned char        pH265Buffer[MediaStreamSource_VideoFifoLength];
	int                  nMp4BufferLength;
	int                  vcl;
	int                  update;
	unsigned char        s_packet[1024 * 256];
	int                  s_packetLength;
	unsigned char        szExtenVideoData[4 * 1024];
	int                  extra_data_size;
	hls_fmp4_t* hlsFMP4;
	int                  track_video;
	struct mpeg4_aac_t   aacHandle;
	int                  track_aac;
	unsigned char        szExtenAudioData[256];
	int                  nExtenAudioDataLength;
	int                  nAACLength;
	int                   flags;
	volatile      bool     hls_init_segmentFlag;
	int                    nFmp4SPSPPSLength;
	int                    fTSFileWriteByteCount;

	struct mpeg4_hevc_t    hevc;
	struct mpeg4_avc_t     avc;
	bool                   VideoFrameToFMP4File(unsigned char* szVideoData, int nLength);
};

#endif