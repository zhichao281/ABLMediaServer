#ifndef _StreamRecordTS_H
#define _StreamRecordTS_H

#include "hls-fmp4.h"
#include "mpeg-ps.h"
#include "hls-m3u8.h"
#include "hls-media.h"
#include "hls-param.h"
#include "mpeg-ps.h"
#include "mov-format.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"

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
#include <memory>
#include <unordered_map>
#endif

class CStreamRecordTS : public CNetRevcBase
{
public:
	CStreamRecordTS(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CStreamRecordTS();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//塞入视频数据
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//塞入音频数据
	virtual int SendVideo();//发送视频数据
	virtual int SendAudio();//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	int64_t               nVideoOrder;
	int                   avtype;
	int                   audioType;
	int64_t               ptsVideo;
	std::map<int, int>    streamsTS;
	bool                  H264H265FrameToTSFile(unsigned char* szVideo, int nLength);
	void* tsPacketHandle;
	FILE* fTSFileWrite;
	int                    fTSFileWriteByteCount;
	struct mpeg_ts_func_t  tshandler;
	char                   s_bufferH264TS[188];
	int                    ts_stream(void* ts, int codecid);

	char                    szFileNameOrder[64];
	char                    szFileName[512];

	std::mutex            mediaMP4MapLock;

	int                  nAACLength;
	int                   flags;
	volatile      bool     hls_init_segmentFlag;
	int                    nFmp4SPSPPSLength;

	struct mpeg4_hevc_t    hevc;
	struct mpeg4_avc_t     avc;
};

#endif