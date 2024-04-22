#pragma once

#if (defined _WIN32 || defined _WIN64)
#define PS_MUX_CALL_METHOD _stdcall
#ifdef LIB_PS_MUX_STATIC
#define PS_MUX_API
#else 
#ifdef LIB_PS_MUX_EXPORT
#define PS_MUX_API _declspec(dllexport)
#else
#define PS_MUX_API _declspec(dllimport)
#endif
#endif
#else
#define PS_MUX_CALL_METHOD
#define PS_MUX_API __attribute__((visibility("default")))
#endif

#include <stdint.h>

enum e_ps_mux_error
{
	e_ps_mux_err_noerror = 0,

	e_ps_mux_err_invalidparameter,

	e_ps_mux_err_mallocmuxerror,

	e_ps_mux_err_managermuxerror,

	e_ps_mux_err_invalidhandle,

	e_ps_mux_err_invaliddata,

	e_ps_mux_err_invalidmediatype,

	e_ps_mux_err_mallocprivatecachefail,

	e_ps_mux_err_privatedatalengthtoolong,

	e_ps_mux_err_mallocaudiocachefail,

	e_ps_mux_err_audiodatalengthtoolong,

	e_ps_mux_err_outbufferisfull,

	e_ps_mux_err_producepsheadererror,

	e_ps_mux_err_producepssysheadererror,

	e_ps_mux_err_producepspsmerror,

	e_ps_mux_err_producepsprivateerror,

	e_ps_mux_err_producepesverror,

	e_ps_mux_err_producepesaerror,
};

enum e_psmux_media_type
{
	e_psmux_mt_unknown = 0,		//unknown

	e_psmux_mt_video,			//video

	e_psmux_mt_audio,			//audio

};

enum e_ps_mux_stream_type
{
	e_psmux_st_unknow = 0,			

	e_psmux_st_mpeg1v = 0x01,		//mpeg1 video

	e_psmux_st_mpeg2v = 0x02,		//mpeg2 video

	e_psmux_st_mpeg1a = 0x03,		//mpeg1 audio

	e_psmux_st_mpeg2a = 0x04,		//mpeg2 audio

	e_psmux_st_mjpeg = 0x0e,		//mjpeg

	e_psmux_st_aac = 0x0f,			//aac

	e_psmux_st_mpeg4 = 0x10,		//mpeg4

	e_psmux_st_aac_latm = 0x11,		//aac latm

	e_psmux_st_h264 = 0x1b,			//h264

	e_psmux_st_h265 = 0x24,			//h265

	e_psmux_st_svacv = 0x80,		//svac video

	e_psmux_st_pcm = 0x81,			//pcm

	e_psmux_st_pcmlaw = 0x82,		//pcm mulaw

	e_psmux_st_g711a = 0x90,		//g711 a

	e_psmux_st_g711u = 0x91,		//g711 u

	e_psmux_st_g7221 = 0x92,		//g722.1

	e_psmux_st_g7231 = 0x93,		//g723.1

	e_psmux_st_g729 = 0x99,			//g729

	e_psmux_st_svaca = 0x9b,		//svac audio

	e_psmux_st_hkp = 0xf0,			//HK private

	e_psmux_st_hwp = 0xf1,			//HW private

	e_psmux_st_dhp = 0xf2,			//DH private
};

enum e_psmux_frame_type
{
	e_psmux_ft_unknown = 0,		//unknown

	e_psmux_ft_i,				//i frame

	e_psmux_ft_p,				//p frame

	e_psmux_ft_b,				//b frame

};

enum e_ps_mux_align_mode
{
	e_psmux_am_nonalign = 0,

	e_psmux_am_4octet,

	e_psmux_am_8octet,
};

enum e_ps_mux_timestamp_mode
{
	e_psmux_ttm_increment = 0,

	e_psmux_ttm_quantity,
};

struct _ps_mux_cb
{
	uint32_t handle;
	void* userdata;
	uint8_t* data;
	uint32_t datasize;
};

struct _ps_mux_init
{
	void* cb;
	void* userdata;
	uint32_t* h;
	int32_t alignmode;
	int32_t ttmode;
	int32_t ttincre;
};

struct _ps_mux_input
{
	uint32_t handle;
	int32_t streamid;
	int32_t streamtype;
	int32_t mediatype;
	int32_t frametype;
	uint64_t pts;
	uint64_t dts;
	uint8_t* data;
	uint32_t datasize;
};

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (PS_MUX_CALL_METHOD *ps_mux_callback)(_ps_mux_cb* cb);

	PS_MUX_API int32_t ps_mux_start(_ps_mux_init* init);

	PS_MUX_API int32_t ps_mux_stop(uint32_t h);

	PS_MUX_API int32_t ps_mux_input(_ps_mux_input* input);

#ifdef __cplusplus
}
#endif


