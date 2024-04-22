#ifndef _StreamRecordMP4_H
#define _StreamRecordMP4_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost;

#else

#endif

#include "mov-writer.h"
#include "mov-format.h"
#include "mpeg4-avc.h"
#include "mpeg4-aac.h"
#include "mpeg4-hevc.h"

#define  MaxWriteMp4BufferCount  1024*1024*2


struct mov_h264_test_t
{
	mov_writer_t* mov;
	struct mpeg4_avc_t avc;
	struct mpeg4_aac_t aac;

	struct mpeg4_hevc_t hevc;

	int track;
	int trackAudio;

	int width;
	int height;
	uint32_t pts, dts;
	const uint8_t* ptr;

	uint32_t ptsAudio, dtsAudio;

	int    vcl;
};

class CStreamRecordMP4 : public CNetRevcBase
{
public:
	CStreamRecordMP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
    ~CStreamRecordMP4() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   uint8_t                asc[128];
   int                    ascLength;
   char                   szFileNameOrder[64];
   char                   szFileName[256];

   bool                   OpenMp4File(int nWidth, int nHeight);
   bool                   CloseMp4File();

   bool                   AddVideo(char* szVideoName, unsigned char* pVideoData, int nVideoDataLength);
   bool                   AddAudio(char* szAudioName, unsigned char* pAudioData, int nAudioDataLength);

   int                    nVideoCodec;
   int                    nAudioCodec;
   int                    nAudioChannels;
   int                    nSampleRate;
   long                   nRecordNumber;//录像句柄序号

   int64_t                nVideoFrameCount;
   int                    vcl;
   int                    update;
   int                    nSize;
   int                    extra_data_size;
   int                    nAACLength;

   std::mutex             writeMp4Lock;
   unsigned char*         s_buffer;
   unsigned char          s_extra_data[48 * 1024];
   FILE*                  fWriteMP4;
   struct mov_h264_test_t ctx;
   bool                   m_bOpenFlag;

};

#endif