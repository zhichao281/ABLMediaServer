#ifndef _RTP_PACKET_DEPACKET_RTP_DEPACKET_H_
#define _RTP_PACKET_DEPACKET_RTP_DEPACKET_H_

#if (defined _WIN32 || defined _WIN64)
#define RTP_DEPACKET_CALL_METHOD _stdcall
#ifdef LIB_RTP_DEPACKET_STATIC
#define RTP_DEPACKET_API
#else 
#ifdef LIB_RTP_DEPACKET_EXPORT
#define RTP_DEPACKET_API _declspec(dllexport)
#else
#define RTP_DEPACKET_API _declspec(dllimport)
#endif
#endif
#else
#define RTP_DEPACKET_CALL_METHOD
#define RTP_DEPACKET_API
#endif

#include <stdint.h>

enum e_rtp_depacket_error
{
	e_rtpdepkt_err_noerror = 0,

	e_rtpdepkt_err_invalidparam,

	e_rtpdepkt_err_mallocsessionfail,
	
	e_rtpdepkt_err_managersessionfail,

	e_rtpdepkt_err_invalidsessionhandle,

	e_rtpdepkt_err_notfindsession,

	e_rtpdepkt_err_invaliddata,

	e_rtpdepkt_err_malformedpacket,

	e_rtpdepkt_err_notfinddepacket,

	e_rtpdepkt_err_inlost,

	e_rtpdepkt_err_seqerror,

	e_rtpdepkt_err_bufferfull,
};

enum e_rtp_depacket_stream_type
{
	e_rtpdepkt_st_unknow = 0,			//unkunow type

	e_rtpdepkt_st_mpeg1v = 0x01,		//mpeg1 video

	e_rtpdepkt_st_mpeg2v = 0x02,		//mpeg2 video

	e_rtpdepkt_st_mpeg1a = 0x03,		//mpeg1 audio

	e_rtpdepkt_st_mpeg2a = 0x04,		//mpeg2 audio

	e_rtpdepkt_st_mjpeg = 0x0e,			//mjpeg

	e_rtpdepkt_st_aac = 0x0f,			//aac

	e_rtpdepkt_st_mpeg4 = 0x10,			//mpeg4

	e_rtpdepkt_st_aac_latm = 0x11,		//aac latm

	e_rtpdepkt_st_h264 = 0x1b,			//h264

	e_rtpdepkt_st_h265 = 0x24,			//h265

	e_rtpdepkt_st_svacv = 0x80,			//svac video

	e_rtpdepkt_st_pcm = 0x81,			//pcm

	e_rtpdepkt_st_pcmlaw = 0x82,		//pcm mulaw

	e_rtpdepkt_st_g711a = 0x90,			//g711 a

	e_rtpdepkt_st_g711u = 0x91,			//g711 u

	e_rtpdepkt_st_g7221 = 0x92,			//g722.1

	e_rtpdepkt_st_g7231 = 0x93,			//g723.1

	e_rtpdepkt_st_g726le = 0x94,		//g726le

	e_rtpdepkt_st_g729 = 0x99,			//g729

	e_rtpdepkt_st_svaca = 0x9b,			//svac audio

	e_rtpdepkt_st_hkp = 0xf0,			//HK private

	e_rtpdepkt_st_hwp = 0xf1,			//HW private

	e_rtpdepkt_st_dhp = 0xf2,			//DH private

	e_rtpdepkt_st_gbps = 0xf3 ,         //国标 PS 

	e_rtpdepkt_st_xhb = 0xf4,         //一家公司 xhb
	e_rtpdepkt_st_mp3 = 0xf5,         //mp3
};

struct _rtp_depacket_cb
{
	uint32_t handle;				//session handle
	uint32_t ssrc;					//ssrc,in local order
	uint32_t timestamp;				//timestamp, in network byte order
	uint8_t payload;				//payload type
	uint32_t streamtype;			//stream type, a e_rtp_depacket_stream_type value
	void* userdata;					//user data
	uint8_t*  data;					//stream data pointer
	uint32_t  datasize;				//stream data size
};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	typedef void (RTP_DEPACKET_CALL_METHOD *rtp_depacket_callback)(_rtp_depacket_cb* cb);

	RTP_DEPACKET_API int32_t rtp_depacket_start(rtp_depacket_callback cb, void* userdata, uint32_t* h);

	RTP_DEPACKET_API int32_t rtp_depacket_stop(uint32_t h);

	RTP_DEPACKET_API int32_t rtp_depacket_input(uint32_t h, uint8_t* data, uint32_t datasize);

	RTP_DEPACKET_API int32_t rtp_depacket_setpayload(uint32_t h, uint8_t payload, uint32_t streamtype);

	RTP_DEPACKET_API int32_t rtp_depacket_setmediaoption(uint32_t h, int8_t* opt, int8_t* parm);

#ifdef __cplusplus
}
#endif

#endif