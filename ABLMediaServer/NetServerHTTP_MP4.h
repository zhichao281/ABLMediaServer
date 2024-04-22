#ifndef _NetServerHTTP_MP4_H
#define _NetServerHTTP_MP4_H

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
#define     Send_MP4File_MaxPacketCount          1024*16
#define     Send_DownloadFile_MaxPacketCount     1024*256

//把mp4切片写入文件
//#define  WriteMp4BufferToFile     1

enum HttpMP4Type
{
	HttpMp4Type_Unknow = 0 ,    //未知
	HttpMp4Type_Play = 1 ,      //播放
	HttpMp4Type_Download = 2 ,  //下载 
};

class CNetServerHTTP_MP4 : public CNetRevcBase
{
public:
	CNetServerHTTP_MP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetServerHTTP_MP4() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   bool                    ResponseError(char* szErrorMsg);
   int                     nPos;
   int                     nSendErrorCount;
   bool                    SendTSBufferData(unsigned char* pTSData, int nLength);

   volatile    bool        bWaitIFrameSuccessFlag;
   volatile    bool        bAddSendThreadToolFlag;
   char                    httpResponseData[1024];
   unsigned char           netDataCache[1024*64]; //网络数据缓存
   int                     netDataCacheLength;//网络数据缓存大小
   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
   int                     MaxNetDataCacheCount;
   int                     data_Length;
   char                    szMP4Name[string_length_2048];
   volatile bool           bFindMP4NameFlag;
   volatile  bool          bCheckHttpMP4Flag; //检测是否为http-MP4协议 
   int                     nWriteRet;

#ifdef WriteMp4BufferToFile
   FILE*               fWriteMP4;
#endif
   std::mutex            mediaMP4MapLock;
   int                   avtype;

   unsigned char*       pMP4Buffer;
   int                  nMp4BufferLength;
   int                  vcl;
   int                  update;
   unsigned char        s_packet[1024*256];
   int                  s_packetLength;
   unsigned char        szExtenVideoData[4 * 1024];
   int                  extra_data_size;
   hls_fmp4_t*          hlsFMP4;
   int                  track_video;
   struct mpeg4_aac_t   aacHandle;
   int                  track_aac;
   unsigned char        szExtenAudioData[256];
   int                  nExtenAudioDataLength;
   int                  nAACLength;
   int                   flags;
   volatile      bool     hls_init_segmentFlag;
   unsigned char          pFmp4SPSPPSBuffer[Send_DownloadFile_MaxPacketCount];
   int                    nFmp4SPSPPSLength;
   int                    fTSFileWriteByteCount;

   struct mpeg4_hevc_t    hevc;
   struct mpeg4_avc_t     avc;
   bool                   VideoFrameToFMP4File(unsigned char* szVideoData, int nLength);
   int                    nHttpDownloadSpeed;//下载速度
   HttpMP4Type            httpMp4Type;
   FILE*                  fFileMp4;//读取mp4文件
   int                    nReadLength;
   int                    nRecordFileSize ;
   CABLSipParse           mp4Parse;

};

#endif