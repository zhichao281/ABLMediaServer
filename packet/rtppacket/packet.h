#pragma once
#include <memory>
#include "rtp_packet.h"
#include "rtp_def.h"

class rtp_packet
{
public: 
	rtp_packet(rtp_packet_callback cb, void* userdata, const _rtp_packet_sessionopt& opt);
	~rtp_packet();

	int32_t handle(uint8_t* data, uint32_t datasize, uint32_t inTimestamp);

private:
	int32_t handle_nalu_stream(int32_t st, uint8_t* data, uint32_t datasize); //for h264, h265, svac video

	int32_t singlenalu(uint8_t* data, uint32_t datasize, bool last);

	int32_t h264_fua(uint8_t* data, uint32_t datasize, bool last);
	int32_t h264_stapa(uint8_t* data, uint32_t datasize, bool last);

	int32_t h265_fus(uint8_t* data, uint32_t datasize, bool last);
	int32_t h265_aps(uint8_t* data, uint32_t datasize, bool last);

	int32_t svacv_fua(uint8_t* data, uint32_t datasize, bool last);
	int32_t svacv_stapa(uint8_t* data, uint32_t datasize, bool last);

	int32_t handle_common(uint8_t* data, uint32_t datasize);

	int32_t aac_adts(uint8_t* data, uint32_t datasize);

private:
	const rtp_packet_callback m_cb;
	const void* m_userdata;
	const _rtp_packet_sessionopt m_attr;
	const uint32_t RTP_PAYLOAD_MAX_SIZE;
	_rtp_packet_cb m_out;
	_rtp_header m_rtphead;
	uint8_t m_outbuff[1500];
	uint32_t m_outbufsize;
	uint32_t m_ttbase;
	uint16_t m_seqbase;
	uint8_t             aacBuffer[1500];
	int                 AdtsHeaderLength; //动态改变adts头长度
	volatile uint32_t   m_inTimestamp;//输入的时间戳
 };

typedef std::shared_ptr<rtp_packet> rtp_packet_ptr;

