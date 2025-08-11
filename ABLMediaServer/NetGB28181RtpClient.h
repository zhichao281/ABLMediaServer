#ifndef _NetGB28181RtpClient_H
#define _NetGB28181RtpClient_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost;
#else

#endif

#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"


#define  MaxGB28181RtpSendVideoMediaBufferLength  1024*64 
#define  GB28181VideoStartTimestampFlag           0xEFEFEFEF   //��Ƶ��ʼʱ��� 
#define  JTT1078_MaxPacketLength                  950          //���һ�����ݳ��� 

//#define  WriteGB28181PSFileFlag   1 //����ps ����
//#define   WriteRecvPSDataFlag   1 //����PS����־
//#define     WriteJtt1078SrcVideoFlag  1 //����1078�����ԭʼ�� 

class CNetGB28181RtpClient : public CNetRevcBase
{
public:
	CNetGB28181RtpClient(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetGB28181RtpClient() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   int                          Find1078HeadFromCacheBuffer(unsigned char* pData, int nLength);
   void                         SplitterJt1078CacheBuffer();
   void                         SplitterJt1078CacheBuffer2019();
   unsigned char*               p1078VideoFrameBuffer;
   int                          n1078VideoFrameBufferLength;
   unsigned short               nPayloadSize;
   int                          n1078Pos;
   int                          n1078CacheBufferLength;
   int                          nVideoPT, nAudioPT;
   int                          n1078CurrentProcCountLength;//��ǰ�������ܳ��� 
   int                          nFind1078FlagPos;//���ҵ���1078��־ͷλ�� 
   int                          n1078NewPosition;//���ҵ���1078��־ͷλ�ú����¼���λ��

   int                          nRecvSampleRate, nRecvChannels;
   //1078 ���ݷ��� 
   Jt1078VideoRtpPacket_T  jt1078VideoHead;
   Jt1078AudioRtpPacket_T  jt1078AudioHead;
   Jt1078OtherRtpPacket_T  jt1078OtherHead;
   Jt1078VideoRtpPacket2019_T  jt1078VideoHead2019;
   Jt1078AudioRtpPacket2019_T  jt1078AudioHead2019;
   Jt1078OtherRtpPacket2019_T  jt1078OtherHead2019;
   int                     nPacketOrder, pPacketCount;
   int                     jt1078SendPacketLenth;//1078�����ݳ���
   int                     nSrcVideoPos;
   void                    SendJtt1078VideoPacket();
   void                    SendJtt1078AduioPacket();
   void                    SendJtt1078VideoPacket2019();
   void                    SendJtt1078AduioPacket2019();
   int                     nPFrameCount; //��ǰһ��I֮֡���P֡���� 
#ifdef WriteJtt1078SrcVideoFlag
   FILE*  fWrite1078SrcFile;
#endif

#ifdef WriteRecvPSDataFlag
   FILE* fWritePSDataFile;
#endif

   char                     m_recvMediaSource[string_length_2048];
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pRecvMediaSource;//����ý��Դ
#else
   std::shared_ptr<CMediaStreamSource> pRecvMediaSource;//����ý��Դ
#endif

   ps_demuxer_t*           psBeiJingLaoChenDemuxer;
   bool                    RtpDepacket(unsigned char* pData, int nDataLength);
   uint32_t                hRtpHandle;             //����rt���

   void    GB28181PsToRtPacket(unsigned char* pPsData, int nLength);
   void    SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength);
   void    GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength);
 
   unsigned char*          netDataCache; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
   int                     MaxNetDataCacheCount;
   unsigned char           rtpHeadOfTCP[4];
   unsigned short          nRtpLength;
   int                     nRtpRtcpPacketType;//�Ƿ���RTP����0 δ֪��2 rtp ��
   _rtp_header*            rtpHeadPtr;

   void                    CreateRtpHandle();
   int                     nMaxRtpSendVideoMediaBufferLength;//ʵ�����ۼ���Ƶ����Ƶ���� ��������귢����Ƶʱ������Ϊ64K�����ֻ������Ƶ������640���� 
   unsigned  char          szSendRtpVideoMediaBuffer[MaxGB28181RtpSendVideoMediaBufferLength];
   int                     nSendRtpVideoMediaBufferLength; //�Ѿ����۵ĳ���  ��Ƶ
   uint32_t                nStartVideoTimestamp; //��һ֡��Ƶ��ʼʱ��� ��
   uint32_t                nCurrentVideoTimestamp;// ��ǰ֡ʱ���
   unsigned short          nVideoRtpLen;
   uint32_t                nSendRet;

   _ps_mux_init  init; 
   _ps_mux_input input;
   uint32_t      psMuxHandle;
   
   //�����ϳ�
   char*   s_buffer;
   int     nVideoStreamID , nAudioStreamID ;
   ps_muxer_t* psBeiJingLaoChen ;
   struct ps_muxer_func_t handler;
   uint64_t videoPTS , audioPTS  ;
   int      nflags;
   uint32_t       nVdeoFrameNumber;

   _rtp_packet_sessionopt  optionPS;
   _rtp_packet_input       inputPS;
   uint32_t                hRtpPS ;
   sockaddr_in             gbDstAddr, gbDstAddrRTCP;

   int64_t                 nSendRtcpTime;
   CRtcpPacketSR           rtcpSR;//�����߱���
   unsigned char           szRtcpSRBuffer[string_length_2048];
   unsigned int            rtcpSRBufferLength;
#ifdef  WriteGB28181PSFileFlag
   FILE*   writePsFile;
#endif
};

#endif