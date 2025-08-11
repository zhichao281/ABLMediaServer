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
//#define   WriteSendPsFileFlag   1 //����ظ���PS����

//#define     WriteRtpTimestamp      1  //���õ�����rtp��ͷʱ���Ϊ0ʱ�����ø�дʱ���
//#define       WriteJt1078VideoFlag   1  //д1078����Ƶ

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

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   //1078 ���ݷ��� 
   int                     jtt1078_KeepOpenPortType;
   char                    sim[string_length_512];
   Jt1078VideoRtpPacket_T  jt1078VideoHead;
   Jt1078AudioRtpPacket_T  jt1078AudioHead;
   Jt1078OtherRtpPacket_T  jt1078OtherHead;
   Jt1078VideoRtpPacket2019_T  jt1078VideoHead2019;
   Jt1078AudioRtpPacket2019_T  jt1078AudioHead2019;
   Jt1078OtherRtpPacket2019_T  jt1078OtherHead2019;
   int                     nPacketOrder, pPacketCount;
   int                     jt1078SendPacketLenth;//1078�����ݳ���
   int                     nSrcVideoPos;
   void                    SendJtt1078VideoPacket(unsigned char* pData, int nLength);
   void                    SendJtt1078AduioPacket(unsigned char* pData, int nLength);
   void                    SendJtt1078VideoPacket2019(unsigned char* pData, int nLength);
   void                    SendJtt1078AduioPacket2019(unsigned char* pData, int nLength);
   int                     nPFrameCount; //��ǰһ��I֮֡���P֡���� 

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
   int                          n1078CurrentProcCountLength;//��ǰ�������ܳ��� 
   int                          nFind1078FlagPos;//���ҵ���1078��־ͷλ�� 
   int                          n1078NewPosition;//���ҵ���1078��־ͷλ�ú����¼���λ��

#ifdef  WriteRtpTimestamp
   uint32_t       nStartTimestap ;
   uint32_t       nWriteTimeStamp;
   volatile bool  bCheckRtpTimestamp;
   unsigned char  szRtpDataArray[3][16*1024];
   int            nRtpDataArrayLength[3];
   int            nRptDataArrayOrder;
   volatile int   nRtpTimestampType;//rtpʱ������� 1 Ϊ�յ�ʱ�����2Ϊ����ʱ��� 
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
   //--�ظ�
#ifdef WriteSendPsFileFlag
   FILE*   fWriteSendPsFile;
#endif 
   void          SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength);
   void          GB28181PsToRtPacket(unsigned char* pPsData, int nLength);
   void          GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength);

   int                     nMaxRtpSendVideoMediaBufferLength;//ʵ�����ۼ���Ƶ����Ƶ���� ��������귢����Ƶʱ������Ϊ64K�����ֻ������Ƶ������640���� 
   unsigned  char          szSendRtpVideoMediaBuffer[MaxGB28181RtpSendVideoMediaBufferLength];
   int                     nSendRtpVideoMediaBufferLength; //�Ѿ����۵ĳ���  ��Ƶ
   uint32_t                nStartVideoTimestamp; //��һ֡��Ƶ��ʼʱ��� ��
   uint32_t                nCurrentVideoTimestamp;// ��ǰ֡ʱ���
   unsigned short          nVideoRtpLen;
   uint32_t                nSendRet;

   uint64_t      nAddSend_app_streamDatetime;//�ظ�����ʱ���
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

   int                     nRtpRtcpPacketType;//�Ƿ���RTP����0 δ֪��2 rtp ��
   unsigned char           szRtcpDataOverTCP[string_length_2048];
   sockaddr_in*            pRtpAddress,*pSrcAddress;
   int64_t                 nSendRtcpTime;
   CRtcpPacketRR           rtcpRR;//�����߱���
   unsigned char           szRtcpSRBuffer[string_length_2048];
   unsigned int            rtcpSRBufferLength;

   //rtp ���
   bool   RtpDepacket(unsigned char* pData, int nDataLength);
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif
   volatile  bool bInitFifoFlag;

   std::mutex              netDataLock;
   unsigned char*          netDataCache; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
   int                     MaxNetDataCacheCount;
   unsigned char           rtpHeadOfTCP[4];
   unsigned short          nRtpLength;

   FILE*  fWritePsFile;
   FILE*  pWriteRtpFile;
};

#endif