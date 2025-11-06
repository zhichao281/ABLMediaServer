#ifndef _Packet_H
#define _Packet_H
#include "rtp_packet.h"
#include "rtp_def.h"

class packet
{
public:
   packet();
   ~packet();
 
   bool        set_option(_rtp_packet_sessionopt* op);
   uint32_t    handle(uint8_t* data, uint32_t datasize, uint32_t inTimestamp);
   int32_t     handle_common(uint8_t* data, uint32_t datasize);
   int32_t     aac_adts(uint8_t* data, uint32_t datasize);
   int32_t     SplitterH264VideoData(unsigned char* pVideoData, int nLength);
   uint32_t    VideoSingleNalu(unsigned char* pFrameData, int nFrameLength, bool bLast);
   uint32_t    H264FuaNalu(unsigned char* pFrameData, int nFrameLength, bool bLast);
   int32_t     SplitterH265VideoData(unsigned char* pVideoData, int nLength);
   uint32_t    H265FusNalu(unsigned char* pFrameData, int nFrameLength, bool bLast);

   int            AdtsHeaderLength;
   unsigned char  aacBuffer[1920];
   _rtp_header    m_rtphead;
   _rtp_packet_cb m_out;
   uint8_t        m_outbuff[1920];
   uint32_t       m_outbufsize;
   uint32_t       m_ttbase;
   uint16_t       m_seqbase;

   _rtp_packet_sessionopt rtp_sessionOpt;//参数 

   uint32_t               m_inTimestamp;
   void*                  userdata;
   uint32_t               nID;//对象ID
   rtp_packet_callback    m_cb;
};

#endif