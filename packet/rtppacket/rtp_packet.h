#ifndef _RTP_PACKET_DEPACKET_RTP_PACKET_H_
#define _RTP_PACKET_DEPACKET_RTP_PACKET_H_


#if (defined _WIN32 || defined _WIN64)
#define RTP_PACKET_CALL_METHOD _stdcall
#ifdef LIB_RTP_PACKET_STATIC
#define RTP_PACKET_API
#else 
#ifdef LIB_RTP_PACKET_EXPORT
#define RTP_PACKET_API _declspec(dllexport)
#else
#define RTP_PACKET_API _declspec(dllimport)
#endif
#endif
#else
#define RTP_PACKET_CALL_METHOD
#define RTP_PACKET_API __attribute__((visibility("default")))
#endif

#include <stdint.h>

enum e_rtp_packet_error
{
	e_rtppkt_err_noerror = 0,

	e_rtppkt_err_invalidparam,

	e_rtppkt_err_mallocsessionerror,

	e_rtppkt_err_managersessionerror,

	e_rtppkt_err_invalidsessionhandle,

	e_rtppkt_err_notfindsession,

	e_rtppkt_err_notfindpacket,

	e_rtppkt_err_existingssrc,

	e_rtppkt_err_mallocpacketerror,

	e_rtppkt_err_managerpacketerror,

	e_rtppkt_err_nonexistingssrc,

	e_rtppkt_err_invaliddata,

	e_rtppkt_err_malformedframe,

	e_rtppkt_err_unsupportedmethod,

	e_rtppkt_err_unsupportednalustream,

	e_rtppkt_err_invalidmeidatype,
};

enum e_rtp_packet_media_type
{
	e_rtppkt_mt_unknown = 0,		//unknown

	e_rtppkt_mt_video,				//video

	e_rtppkt_mt_audio,				//audio

};

enum e_rtp_packet_stream_type
{
	e_rtppkt_st_unknow = 0,			//unkunow type

	e_rtppkt_st_mpeg1v = 0x01,		//mpeg1 video

	e_rtppkt_st_mpeg2v = 0x02,		//mpeg2 video

	e_rtppkt_st_mpeg1a = 0x03,		//mpeg1 audio

	e_rtppkt_st_mpeg2a = 0x04,		//mpeg2 audio

	e_rtppkt_st_mjpeg = 0x0e,		//mjpeg

	e_rtppkt_st_aac = 0x0f,			//aac

	e_rtppkt_st_mpeg4 = 0x10,		//mpeg4

	e_rtppkt_st_aac_latm = 0x11,	//aac latm

	e_rtppkt_st_h264 = 0x1b,		//h264

	e_rtppkt_st_h265 = 0x24,		//h265

	e_rtppkt_st_svacv = 0x80,		//svac video

	e_rtppkt_st_pcm = 0x81,			//pcm

	e_rtppkt_st_pcmlaw = 0x82,		//pcm mulaw

	e_rtppkt_st_g711a = 0x90,		//g711 a

	e_rtppkt_st_g711u = 0x91,		//g711 u

	e_rtppkt_st_g7221 = 0x92,		//g722.1

	e_rtppkt_st_g7231 = 0x93,		//g723.1

	e_rtppkt_st_g729 = 0x99,		//g729

	e_rtppkt_st_svaca = 0x9b,		//svac audio

	e_rtppkt_st_hkp = 0xf0,			//HK private

	e_rtppkt_st_hwp = 0xf1,			//HW private

	e_rtppkt_st_dhp = 0xf2,			//DH private
	e_rtppkt_st_gb28181 = 0xf3,     //国标PS 打包 
};

enum e_rtp_packet_align_mode
{
	e_rtppkt_am_nonalign = 0,

	e_rtppkt_am_4octet,

	e_rtppkt_am_8octet,
};

struct _rtp_packet_input
{
	uint32_t handle;
	uint32_t ssrc;
	uint8_t* data;
	uint32_t datasize;
	uint32_t timestamp;
	_rtp_packet_input()
	{
		handle = 0 ;
		ssrc = 0;
		data = 0;
		datasize = 0 ;
		timestamp = 0 ;
	}
};

struct _rtp_packet_cb
{
	uint32_t handle;
	uint32_t ssrc;
	void* userdata;
	uint8_t* data;
	uint32_t datasize;
};

struct _rtp_packet_sessionopt
{
	uint32_t handle;
	uint8_t payload;
	uint32_t ssrc;
	int32_t mediatype;
	int32_t streamtype;
	int32_t alimod;
	int32_t ttincre;
};

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (RTP_PACKET_CALL_METHOD *rtp_packet_callback)(_rtp_packet_cb* cb);

	 RTP_PACKET_API int32_t rtp_packet_start(rtp_packet_callback cb, void* userdata, uint32_t* h);

	 RTP_PACKET_API int32_t rtp_packet_stop(uint32_t h);

	 RTP_PACKET_API int32_t rtp_packet_input(_rtp_packet_input* input);

	 RTP_PACKET_API int32_t rtp_packet_setsessionopt(_rtp_packet_sessionopt* opt);

	 RTP_PACKET_API int32_t rtp_packet_resetsessionopt(_rtp_packet_sessionopt* opt);

#ifdef __cplusplus
}
#endif


#endif