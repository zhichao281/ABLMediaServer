#ifndef _DePacket_H
#define _DePacket_H

#include  "rtp_depacket.h" 
#include  "rtp_def.h"

#define RTP_DEPACKET_MAX_SIE  1024*1024*3 
#define      MaxNaluBlockCount    32   //最大Nalu段
#define DEPACKET_VpsSpsPpsBuffer_SIZE (8192)

#define  H265_UseSplitterVideoFlag     1  //针对H265在高分辨率2K、4K、5K 、8K 可能采用一帧视频有2个以上的slice打包，进行拼接、切割

struct NaluHeardPosition
{
	int nPos;        //出现位置
	int NaluHeardLength;//Nalu标志出现的位置
};

class depacket
{
public:
	 depacket();
	 ~depacket();
	 bool                bFindFrameFlag;
	 bool                CopyVpsSpsPps(uint8_t* pData, int nLength);
	 bool                CheckIdrFrame(uint8_t* pData, int nLength);
	 unsigned char       szVpsSpsPpsBuffer[DEPACKET_VpsSpsPpsBuffer_SIZE];
	 int                 nVpsSpsPpsBufferSize;

	 int32_t             SplitterRtpDepacketBuffer(uint8_t* data, uint32_t datasize);
	 NaluHeardPosition   nPosArray[MaxNaluBlockCount];
	 unsigned int        nFindCount;
	 int                 nPos1, nPos2, nNaluLength;
	 int                 sclen, payloadlen;
	 bool                bSetOptFlag;
	 bool                bIsIFrameFlag;
	 bool                bLastFlag;
	 unsigned char       nFrameType;
	 unsigned char       bFullNaluFlag;

	 int32_t handle(unsigned char* pData, int nLength);
	 int32_t handle_common(unsigned char* pData, int nLength);
	 int32_t handle_h264(unsigned char* pData, int nLength);
	 int32_t handle_h265(unsigned char* pData, int nLength);

	 int32_t               SingleDepacket(unsigned char* pData, int nLength);
	 int32_t               Stap_aDepacket(unsigned char* pData, int nLength);

	 int32_t               H264FuaDepacket(unsigned char* pData, int nLength);
	 int32_t               H265FusDepacket(unsigned char* pData, int nLength);

	 _rtp_header*            rtpHead;
	 uint32_t                exLength;
	 _rtp_fua_indicator_h265 indH265;
	 _rtp_fus_header         fusH265;
	 unsigned char           nNalu;
	 uint32_t                timestamp;
	 uint8_t                 m_buff[RTP_DEPACKET_MAX_SIE];
	 uint8_t*                cbBuff;
	 uint32_t                m_buffsize;

	 _rtp_depacket_cb      cb;
	 rtp_depacket_callback m_cb;  //回调函数
	 void*                 m_userdata;//用户句柄
	 uint32_t              m_ID;//ID

	 uint8_t               payload;//rtp 的 payload 
	 uint32_t              streamtype;//音频、视频类型 
};

#endif