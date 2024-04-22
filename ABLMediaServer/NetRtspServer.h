#ifndef _NetRtspServer_H
#define _NetRtspServer_H

#include "MediaStreamSource.h"
#include "rtp_packet.h"
#include "RtcpPacket.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#else

#endif


#include "mpeg4-avc.h"

#define  MaxRtspValueCount          64 
#define  MaxRtspProtectCount        24 
#define   MaxRtpHandleCount         2

//#define  WriteRtpDepacketFileFlag   1 //是否写rtp解包文件
//#define    WriteVideoDataFlag         1 //写入视频标志  
//#define      WritRtspMp3FileFlag         1 //写mp3文件

#define    RtspServerRecvDataLength             1024*32      //用于切割缓存区的大小 
#define    MaxRtpSendVideoMediaBufferLength     1024*64      //用于拼接RTP包，准备发送 
#define    MaxRtpSendAudioMediaBufferLength     1024*8       //用于拼接RTP包，准备发送 
#define    VideoStartTimestampFlag              0xEFEFEFEF   //视频开始时间戳 

//HTTP头结构
struct HttpHeadStruct
{
	char szKey[128];
	char szValue[512];
};

struct RtspFieldValue
{
	char szKey[64];
	char szValue[384];
};

//rtsp 协议数据
struct RtspProtect
{
	char  szRtspCmdString[512];//  OPTIONS rtsp://190.15.240.36:554/Camera_00001.sdp RTSP/1.0
	char  szRtspCommand[64];//  rtsp命令名字 ,OPTIONS ANNOUNCE SETUP  RECORD   
	char  szRtspURL[512];// rtsp://190.15.240.36:554/Camera_00001.sdp

	RtspFieldValue rtspField[MaxRtspValueCount];
	char  szRtspContentSDP[1024]; //  媒体描述内容
	int   nRtspSDPLength;
};

class CNetRtspServer : public CNetRevcBase
{
public:
	CNetRtspServer(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetRtspServer() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据

   virtual int SendVideo() ;//发送视频数据
   virtual int SendAudio() ;//发送音频数据
   virtual int SendFirstRequst();//发送第一个请求
   virtual bool RequestM3u8File();//请求m3u8文件

   uint32_t        nVdeoFrameNumber ;
   uint32_t        nAudioFrameNumber ;
   bool            responseUdpSetup();
   bool            createTcpRtpDecode();//创建rtp解包
   char            responseTransport[string_length_512];
   int             nRecvRtpPacketCount;
   unsigned short  nMaxRtpLength;
   unsigned char   szFullMp3Buffer[2048];
   unsigned char   szMp3HeadFlag[8];
   void            SplitterMp3Buffer(unsigned char* szMp3Buffer, int nLength);
#ifdef  WritRtspMp3FileFlag 
   FILE*         fWriteMp3File;
#endif
#ifdef WriteVideoDataFlag 
   FILE*   fWriteVideoFile;
#endif 

   static  unsigned short nRtpPort;
   bool                   GetAVClientPortByTranspot(char* szTransport);
   sockaddr_in            addrClientVideo[MaxRtpHandleCount];
   sockaddr_in            addrClientAudio[MaxRtpHandleCount];
   NETHANDLE              nServerVideoUDP[MaxRtpHandleCount];
   NETHANDLE              nServerAudioUDP[MaxRtpHandleCount];
   unsigned short         nVideoServerPort[MaxRtpHandleCount];
   unsigned short         nAudiServerPort[MaxRtpHandleCount];
   unsigned short         nVideoClientPort[MaxRtpHandleCount];
   unsigned short         nAudioClientPort[MaxRtpHandleCount];
   unsigned short         nSetupOrder;

   int                     nRtspPlayCount;//play的次数
   bool                    FindRtpPacketFlag();

   unsigned char           s_extra_data[string_length_2048];
   int                     extra_data_size;
   struct mpeg4_avc_t      avc;
#ifdef WriteRtpDepacketFileFlag
   bool                     bStartWriteFlag ;
#endif 

   int                     nSendRtpFailCount;//累计发送rtp包失败次数 

   bool                    GetSPSPPSFromDescribeSDP();

   CRtcpPacketSR           rtcpSR;
   CRtcpPacketRR           rtcpRR;
   unsigned char           szRtcpSRBuffer[string_length_2048];
   unsigned int            rtcpSRBufferLength;
   unsigned char           szRtcpDataOverTCP[string_length_2048];
   void                    SendRtcpReportData(unsigned int nSSRC,int nChan);//发送rtcp 报告包,发送端
   void                    SendRtcpReportDataRR(unsigned int nSSRC, int nChan);//发送rtcp 报告包,接收端
   void                    ProcessRtcpData(char* szRtpData, int nDataLength, int nChan);

   int                     GetRtspPathCount(char* szRtspURL);//统计rtsp URL 路径数量

   volatile                uint64_t tRtspProcessStartTime; //开始时间

   unsigned  char          szSendRtpVideoMediaBuffer[MaxRtpSendVideoMediaBufferLength];
   unsigned  char          szSendRtpAudioMediaBuffer[MaxRtpSendAudioMediaBufferLength];
   int                     nSendRtpVideoMediaBufferLength; //已经积累的长度  视频
   int                     nSendRtpAudioMediaBufferLength; //已经积累的长度  音频
   uint32_t                nStartVideoTimestamp; //上一帧视频初始时间戳 ，
   uint32_t                nCurrentVideoTimestamp;// 当前帧时间戳
   int                     nCalcAudioFrameCount;//积累音频包数量

   void                    ProcessRtpVideoData(unsigned char* pRtpVideo, int nDataLength);
   void                    ProcessRtpAudioData(unsigned char* pRtpAudio, int nDataLength);
   void                    SumSendRtpMediaBuffer(unsigned char* pRtpMedia,int nRtpLength);//累积rtp包，准备发送
   std::mutex              MediaSumRtpMutex;

   unsigned short          nVideoRtpLen, nAudioRtpLen;

   _rtp_packet_sessionopt  optionVideo;
   _rtp_packet_input       inputVideo;
   _rtp_packet_sessionopt  optionAudio;
   _rtp_packet_input       inputAudio;
   uint32_t                hRtpVideo, hRtpAudio;
   uint32_t                nVideoSSRC;

   bool                    GetRtspSDPFromMediaStreamSource(RtspSDPContentStruct sdpContent,bool bGetFlag);
   char                    szRtspSDPContent[string_length_2048];
   char                    szRtspAudioSDP[string_length_2048];
   bool                    GetMediaURLFromRtspSDP(); //获取rtsp地址中的媒体文件 ，比如 /Media/Camera_00001

   bool                    GetMediaInfoFromRtspSDP();
   void                    SplitterRtpAACData(unsigned char* rtpAAC, int nLength);
   int32_t                 XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain);
   bool                    ReadRtspEnd();

   int                     au_header_length;
   uint8_t                 *ptr, *pau, *pend;
   int                     au_size ; // only AU-size
   int                     au_numbers ;
   int                     SplitterSize[16];

   volatile bool           bRunFlag;

   std::mutex              netDataLock;
   unsigned char           netDataCache[MaxNetDataCacheBufferLength]; //网络数据缓存
   int                     netDataCacheLength;//网络数据缓存大小
   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
   int                     MaxNetDataCacheCount;

   unsigned char           data_[RtspServerRecvDataLength];//每一帧rtsp数据，包括rtsp 、 rtp 包 
   unsigned int            data_Length;
   unsigned short          nRtpLength;
   int                     nContentLength;

   RtspProtect      RtspProtectArray[MaxRtspProtectCount];
   int              RtspProtectArrayOrder;
 
   int             FindHttpHeadEndFlag();
   int             FindKeyValueFlag(char* szData);
   void            GetHttpModemHttpURL(char* szMedomHttpURL);
   int             FillHttpHeadToStruct();
   bool            GetFieldValue(char* szFieldName, char* szFieldValue);

   bool            bReadHeadCompleteFlag; //是否读取完毕HTTP头
   int             nRecvLength;           //已经读取完毕的长度
   unsigned char   szHttpHeadEndFlag[8];  //Http头结束标志
   int             nHttpHeadEndLength;    //Http头结束标志点的长度 
   char            szResponseHttpHead[string_length_2048];
   char            szCSeq[string_length_2048];
   char            szTransport[string_length_2048];

   char            szResponseBuffer[string_length_4096];
   int             nSendRet;
  static   uint64_t Session ;
  uint64_t         currentSession;
  char             szCurRtspURL[string_length_2048];
  int64_t           nPrintCount;

   //只处理rtsp命令，比如 OPTIONS,DESCRIBE,SETUP,PALY 
   void            InputRtspData(unsigned char* pRecvData, int nDataLength);

   void           AddADTSHeadToAAC(unsigned char* szData, int nAACLength);
   unsigned char  aacData[string_length_2048];
   int            timeValue;
   uint32_t       hRtpHandle[MaxRtpHandleCount];
   char           szVideoName[64];
   char           szAudioName[64];
   int            nVideoPayload;
   int            nAudioPayload;
   int            sample_index;//采样频率所对应的序号 
   int            nChannels; //音频通道数
   int            nSampleRate; //音频采样频率
   char           szRtspContentSDP[string_length_1024];
   char           szVideoSDP[string_length_2048];
   char           szAudioSDP[string_length_2048];
   CABLSipParse   sipParseV, sipParseA;   //sdp 信息分析

#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

   volatile bool  bIsInvalidConnectFlag; //是否为非法连接 
   volatile bool  bExitProcessFlagArray[3];

#ifdef WriteRtpDepacketFileFlag
   FILE*          fWriteRtpVideo;
   FILE*          fWriteRtpAudio;
#endif
};

#endif