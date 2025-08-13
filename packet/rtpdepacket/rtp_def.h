#ifndef _RTP_PACKET_DEPACKET_RTP_DEF_H_
#define _RTP_PACKET_DEPACKET_RTP_DEF_H_

#include <stdint.h>

#define RTP_VERSION 2

const uint8_t RTP_H264_SLICESTARTCODE[] = { 0x00, 0x00, 0x00, 0x01 };
const uint8_t RTP_H264_NALUSTARTCODE[] = { 0x00, 0x00, 0x01 };

const uint8_t RTP_H265_SLICESTARTCODE[] = { 0x00, 0x00, 0x00, 0x01 };
const uint8_t RTP_H265_NALUSTARTCODE[] = { 0x00, 0x00, 0x01 };

enum e_rtcp_packet_type
{
	e_rtcp_pkt_sr = 200,
	e_rtcp_pkt_rr,
	e_rtcp_pkt_sdes,
	e_rtcp_pkt_bye,
	e_rtcp_pkt_app,
};

enum e_rtp_h264_mode
{
	e_rtp_h264mode_minnalunit = 1,
	e_rtp_h264mode_maxnalunit = 23,
	e_rtp_h264mode_stapa = 24,
	e_rtp_h264mode_stapb = 25,
	e_rtp_h264mode_mtap16 = 26,
	e_rtp_h264mode_mtap24 = 27,
	e_rtp_h264mode_fua = 28,
	e_rtp_h264mode_fub = 29,
};

enum e_rtp_h265_mode
{	
	e_rtp_h265mode_minnalunit = 0,
	e_rtp_h265mode_maxnalunit = 47,
	e_rtp_h265mode_ap = 48,
	e_rtp_h265mode_fu = 49,
	e_rtp_h265mode_paci = 50,
	e_rtp_h265mode_maxnallike = 63,
};

#pragma pack(push, 1)

struct _rtp_header
{
	uint8_t cc : 4;
	uint8_t x : 1;
	uint8_t p : 1;
	uint8_t v : 2;
	uint8_t payload : 7;
	uint8_t mark : 1;
	uint16_t seq;
	uint32_t timestamp;
	uint32_t ssrc;

	_rtp_header()
		: v(2), p(0), x(0), cc(0)
		, mark(0), payload(0)
		, seq(0)
		, timestamp(0)
		, ssrc(0)
	{
	}
};

//fu-a for h264
struct _rtp_fua_indicator
{
	uint8_t m : 5;	//28-fu-a
	uint8_t n : 2;	//nalu nal_ref_idc
	uint8_t f : 1;	//nalu forbidden_zero_bit

	_rtp_fua_indicator()
		: f(0), n(0), m(28)
	{
	}
};

struct  _rtp_fua_header
{
	uint8_t t : 5;	//nalu nal_unit_type
	uint8_t r : 1;	//reserved, must be '0'
	uint8_t e : 1;	//end flag
	uint8_t s : 1;	//begin flag

	_rtp_fua_header()
		: s(0), e(0), r(0), t(0)
	{
	}
};

//fus for h265
struct _rtp_fus_header
{
	uint8_t  t : 6;	//nalu nal_unit_type
	uint8_t  e : 1; //end flag
	uint8_t  s : 1;	//begin flag

	_rtp_fus_header()
		: s(0), e(0), t(0)
	{
	}
};

struct _rtp_paci_header
{
	uint16_t y : 1;
	uint16_t f : 3;
	uint16_t phs : 5;
	uint16_t c : 6;
	uint16_t a : 1;

	_rtp_paci_header()
		: a(0), c(0), phs(0), f(0), y(0)
	{
	}
};

#pragma pack(pop)

#endif
