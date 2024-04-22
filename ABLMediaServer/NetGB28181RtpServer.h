#ifndef _NetGB28181RtpServer_H
#define _NetGB28181RtpServer_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

//#define   WriteRecvAACDataFlag   1
//#define   WriteRtpFileFlag    1 
//#define   WriteSendPsFileFlag   1 //保存回复的PS数据

//#define     WriteRtpTimestamp      1  //启用当国标rtp包头时间戳为0时，启用覆写时间戳
//#define       WriteJt1078VideoFlag   1  //写1078的视频

using namespace boost;
#else


#endif
class CNetGB28181RtpServer : public CNetRevcBase
{
public:
	CNetGB28181RtpServer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetGB28181RtpServer() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
   virtual int SendVideo();//发送视频数据
   virtual int SendAudio();//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

#ifdef WriteJt1078VideoFlag
   FILE*                        fWrite1078File;
#endif
   int                          nRecvChannels, nRecvSampleRate;
   int                          nRecvRtpPacketCount;
   int                          nMaxRtpLength;
   int                          Find1078HeadFromCacheBuffer(unsigned char* pData, int nLength);
   void                         SplitterJt1078CacheBuffer();
   void                         SplitterJt1078CacheBuffer2019();
   unsigned char*               p1078VideoFrameBuffer;
   int                          n1078VideoFrameBufferLength;
   unsigned short               nPayloadSize;
   int                          n1078Pos;
   int                          n1078CacheBufferLength;
   int                          nVideoPT,nAudioPT;
   int                          n1078CurrentProcCountLength;//当前出来的总长度 
   int                          nFind1078FlagPos;//查找到的1078标志头位置 
   int                          n1078NewPosition;//查找到的1078标志头位置后重新计算位置

#ifdef  WriteRtpTimestamp
   uint32_t       nStartTimestap ;
   uint32_t       nWriteTimeStamp;
   volatile bool  bCheckRtpTimestamp;
   unsigned char  szRtpDataArray[3][16*1024];
   int            nRtpDataArrayLength[3];
   int            nRptDataArrayOrder;
   volatile int   nRtpTimestampType;//rtp时间戳类型 1 为空的时间戳，2为正常时间戳 
   _rtp_header*  writeRtpHead;
#endif

   void         ProcessRtcpData(unsigned char* szRtcpData, int nDataLength, int nChan);

   void            AddADTSHeadToAAC(unsigned char* szData, int nAACLength);
   unsigned char   aacData[4096];
   int             aacDataLength;
   uint8_t         sampling_frequency_index;

#ifdef WriteRecvAACDataFlag
   FILE*    fWriteRecvAACFile;
#endif
   //--回复
#ifdef WriteSendPsFileFlag
   FILE*   fWriteSendPsFile;
#endif 
   void          SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength);
   void          GB28181PsToRtPacket(unsigned char* pPsData, int nLength);
   void          GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength);

   int                     nMaxRtpSendVideoMediaBufferLength;//实际上累计视频，音频总量 ，如果国标发送视频时，可以为64K，如果只发送音频就设置640即可 
   unsigned  char          szSendRtpVideoMediaBuffer[MaxGB28181RtpSendVideoMediaBufferLength];
   int                     nSendRtpVideoMediaBufferLength; //已经积累的长度  视频
   uint32_t                nStartVideoTimestamp; //上一帧视频初始时间戳 ，
   uint32_t                nCurrentVideoTimestamp;// 当前帧时间戳
   unsigned short          nVideoRtpLen;
   uint32_t                nSendRet;

   volatile bool addThreadPoolFlag;
   uint64_t      nAddSend_app_streamDatetime;//回复码流时间戳
   void          CreateSendRtpByPS();
   char*         s_buffer;
   int           nVideoStreamID, nAudioStreamID;
   ps_muxer_t* psBeiJingLaoChenMuxer;
   struct ps_muxer_func_t handler;
   uint64_t videoPTS, audioPTS;
   int      nflags;

   _rtp_packet_sessionopt  optionPS;
   _rtp_packet_input       inputPS;
   uint32_t                hRtpPS;

#ifdef  WriteRtpFileFlag
   FILE*                  fWriteRtpFile;
#endif
   _rtp_header*            rtpHeadPtr;
   ps_demuxer_t*           psBeiJingLaoChen;

   int                     nRtpRtcpPacketType;//是否是RTP包，0 未知，2 rtp 包
   unsigned char           szRtcpDataOverTCP[string_length_2048];
   sockaddr_in*            pRtpAddress,*pSrcAddress;
   int64_t                 nSendRtcpTime;
   CRtcpPacketRR           rtcpRR;//接收者报告
   unsigned char           szRtcpSRBuffer[string_length_2048];
   unsigned int            rtcpSRBufferLength;

   //rtp 解包
   bool   RtpDepacket(unsigned char* pData, int nDataLength);
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif
   volatile  bool bInitFifoFlag;

   std::mutex              netDataLock;
   unsigned char*          netDataCache; //网络数据缓存
   int                     netDataCacheLength;//网络数据缓存大小
   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
   int                     MaxNetDataCacheCount;
   unsigned char           rtpHeadOfTCP[4];
   unsigned short          nRtpLength;

   FILE*  fWritePsFile;
   FILE*  pWriteRtpFile;
};

#endif