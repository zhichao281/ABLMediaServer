#ifndef _PS_MUX_DEMUX_PS_DEF_H_
#define _PS_MUX_DEMUX_PS_DEF_H_

#include <stdint.h>

const uint8_t PS_START_CODE[] = { 0x00, 0x00, 0x01 };

enum e_ps_segment_type
{
	e_ps_segment_unknow = 0,

	e_ps_segment_psheader = 0xba,

	e_ps_segment_pssysheader = 0xbb,

	e_ps_segment_pspsm = 0xbc,
};

enum e_ps_streamid_type
{
	e_ps_streamid_unknow = 0,

	e_ps_streamid_psm = 0xbc,

	e_ps_streamid_private1 = 0xbd,

	e_ps_streamid_padding = 0xbe, 

	e_ps_streamid_private2 = 0xbf,

	e_ps_streamid_minaudio = 0xc0,

	e_ps_streamid_maxaudio = 0xdf,

	e_ps_streamid_minvideo = 0xe0, 

	e_ps_streamid_maxvideo = 0xef,

	e_ps_streamid_ecm = 0xf0,

	e_ps_streamid_emm = 0xf1, 

	e_ps_streamid_dsm = 0xf2,

	e_ps_streamid_iso13522 = 0xf3,

	e_ps_streamid_minreserved = 0xf4,

	e_ps_streamid_maxreserved = 0xfe, 

	e_ps_streamid_psd = 0xff,
};

#pragma pack(push, 1)

// ps header  : at least 14 bytes
typedef struct _ps_header
{	
	uint8_t startcode[4];		//32bit start code, must be '0x000001ba'

	uint8_t scr_2 : 2;			//33bit system_clock_reference_base  [29..28]
	uint8_t mark_1 : 1;			//mark , must be '1'
	uint8_t scr_1 : 3;			//33bit system_clock_reference_base  [32..30]
	uint8_t fix : 2;			//fix bit ,must be '01'

	uint8_t scr_3;				//33bit system_clock_reference_base  [27..20]

	uint8_t scr_5 : 2;			//33bit system_clock_reference_base  [14..13]
	uint8_t mark_2 : 1;			//mark , must be '1'
	uint8_t scr_4 : 5;			//33bit system_clock_reference_base  [19..15]

	uint8_t scr_6;				//33bit system_clock_reference_base  [12..5]

	uint8_t scre_1 : 2;			//9bit system_clock_reference_extension [8..7]
	uint8_t mark_3 : 1;			//mark , must be '1'
	uint8_t scr_7 : 5;			//33bit system_clock_reference_base  [4..0]

	uint8_t mark_4 : 1;			//mark , must be '1'
	uint8_t scre_2 : 7;			//9bit system_clock_reference_extension [6..0]
		
	uint8_t pmr_1;				//22bit program_mux_rate [21..14]

	uint8_t pmr_2;				//22bit program_mux_rate [13..6]

	uint8_t mark_5 : 1;			//mark , must be '1'
	uint8_t mark_6 : 1;			//mark , must be '1'
	uint8_t pmr_3 : 6;			//22bit program_mux_rate [5..0]

	uint8_t psl : 3;			//pack_stuffing_length 
	uint8_t r : 5;				//reserved

	_ps_header()
		: scr_2(0), mark_1(1), scr_1(0), fix(1)
		, scr_3(0)
		, scr_5(0), mark_2(1), scr_4(0)
		, scr_6(0)
		, scre_1(0), mark_3(1), scr_7(0)
		, mark_4(1), scre_2(0)
		, pmr_1(0)
		, pmr_2(0)
		, mark_5(1), mark_6(1), pmr_3(0)
		, psl(0), r(0x1f)
	{
		startcode[0] = 0x00;
		startcode[1] = 0x00;
		startcode[2] = 0x01;
		startcode[3] = 0xba;
	}

	uint64_t get_system_clock_reference_base()
	{
		uint64_t val = (scr_1 << 30) | (scr_2 << 28)
						| (scr_3 << 20) | (scr_4 << 15)
						| (scr_5 << 13) | (scr_6 << 5)
						| (scr_7);

		return val;
	}

	void set_system_clock_reference_base(uint64_t val)
	{
		scr_1 = (val >> 30) & 0x07;
		scr_2 = (val >> 28) & 0x03;
		scr_3 = (val >> 20) & 0xff;
		scr_4 = (val >> 15) & 0x1f;
		scr_5 = (val >> 13) & 0x03;
		scr_6 = (val >> 5) & 0xff;
		scr_7 = val & 0x1f;
	}

	uint16_t get_system_clock_reference_extension()
	{
		uint16_t val = (scre_1 << 7) | (scre_2);

		return val;
	}

	void set_system_clock_reference_extension(uint16_t val)
	{
		scre_1 = (val >> 7) & 0x03;
		scre_2 = val & 0x7f;
	}

	uint32_t get_program_mux_rate()
	{
		uint32_t val = (pmr_1 << 14) | (pmr_2 << 6) | pmr_3;

		return val;
	}

	void set_program_mux_rate(uint32_t val)
	{
		pmr_1 = (val >> 14) & 0xff;
		pmr_2 = (val >> 6) & 0xff;
		pmr_3 = val & 0x3f;
	}

}PS_HEADER, *PPS_HEADER;

// ps system header
typedef struct _ps_pstd
{
	uint8_t sid;			//stream id

	uint8_t bzb_1 : 5;		//13bit P-STD_buffer_size_bound [12..8]
	uint8_t bbs : 1;		//P-STD_buffer_bound_scale
	uint8_t fix : 2;		//fixed bit, must be '11'

	uint8_t bzb_2;			//13bit P-STD_buffer_size_bound [7..0]

	_ps_pstd()
		: sid(0)
		, bzb_1(0), bbs(0), fix(0x03)
		, bzb_2(0)
	{
	}

	uint16_t get_pstd_buffer_size_bound()
	{
		uint16_t val = (bzb_1 << 8) | bzb_2;

		return val;
	}

	void set_pstd_buffer_size_bound(uint16_t val)
	{
		bzb_1 = (val >> 8) & 0x1f;
		bzb_2 = val & 0xff;
	}

}PS_PSTD, *PPS_PSTD;

typedef struct _ps_sys_header
{
	uint8_t startcode[4];							//32bit start code , must be '0x000001BB'

	uint8_t headlen[2];								//16bit header length, in network byte order

	uint8_t rb_1 : 7;								//22bit  rate_bound [21..15]
	uint8_t mark_1 : 1;								//marker bit, must be '1'

	uint8_t rb_2;									//22bit  rate_bound [14..7]

	uint8_t mark_2 : 1;								//marker bit, must be '1'
	uint8_t rb_3 : 7;								//22bit  rate_bound  [6..0]

	uint8_t csps : 1;								//CSPS_flag
	uint8_t fix : 1;								//fixed_flag
	uint8_t ab : 6;									//audio_bound

	uint8_t vb : 5;									//vedio_bound
	uint8_t mark_3 : 1;								//marker bit, must be '1'
	uint8_t svlf : 1;								//system_video_lock_flag
	uint8_t salf : 1;								//system_audio_lock_flag

	uint8_t r : 7;									//reserved
	uint8_t prrf : 1;								//packet_rate_restriction_flag

	_ps_sys_header()
		: rb_1(0), mark_1(1)
		, rb_2(0)
		, mark_2(1), rb_3(0)
		, csps(0), fix(0), ab(1)
		, vb(1), mark_3(1), svlf(0), salf(0)
		, r(0x7f), prrf(1)
	{
		startcode[0] = 0x00;
		startcode[1] = 0x00;
		startcode[2] = 0x01;
		startcode[3] = 0xbb;

		headlen[0] = 0x00;
		headlen[1] = 0x00;

	}

	uint32_t get_rate_bound()
	{
		uint32_t val = (rb_1 << 15) | (rb_2 << 7) | rb_3;

		return val;
	}

	void set_rate_bound(uint32_t val)
	{
		rb_1 = (val >> 15) & 0x7f;
		rb_2 = (val >> 7) & 0xff;
		rb_3 = val & 0x7f;
	}

	uint16_t get_header_length()
	{
		uint16_t val = (headlen[0] << 8) | headlen[1];

		return val;
	}

	void set_header_length(uint16_t val)
	{
		headlen[0] = (val >> 8) & 0xff;
		headlen[1] = val & 0xff;
	}

}PS_SYS_HEADER,*PPS_SYS_HEADER;


//ps psm
typedef struct _ps_elementary_stream
{
	uint8_t st;			//stream type   

	uint8_t sid;		//stream id

	uint8_t sil[2];		//stream info length

	_ps_elementary_stream()
		: st(0)
		, sid(0)
	{
		sil[0] = 0x00;
		sil[1] = 0x00;
	}

	uint16_t get_stream_info_length()
	{
		uint16_t val = (sil[0] << 8) | sil[1];

		return val;
	}

	void set_stream_info_length(uint16_t val)
	{
		sil[0] = (val >> 8) & 0xff;
		sil[1] = val & 0xff;
	}

}PS_ELEMENTARY_STREAM, *PPS_ELEMENTARY_STREAM;

typedef struct _ps_psm
{
	uint8_t startcode[3];							//startcode, must be '0x000001'

	uint8_t sid;									//map_stream_id, must be '0xbc'

	uint8_t psmlen[2];								//16bit program_stream_map_length, in network byte order

	uint8_t v : 5;									//program_stream_map_version
	uint8_t r_1 : 2;
	uint8_t cni : 1;								//current_next_indicator

	uint8_t mark : 1;								//marker bit, must be '1'
	uint8_t r_2 : 7;								//reserved

	uint8_t sil[2];									//program_stream_info_length

	//uint8_t esml[2];								//elementary_stream_map_length

	_ps_psm()
		: sid(0xbc)
		, v(0), r_1(0x03), cni(1)
		, mark(1), r_2(0x7f)
	{
		startcode[0] = 0x00;
		startcode[1] = 0x00;
		startcode[2] = 0x01;

		psmlen[0] = 0x00;
		psmlen[1] = 0x00;

		sil[0] = 0x00;
		sil[1] = 0x00;

		//esml[0] = 0x00;
		//esml[1] = 0x00;

	}

	uint16_t get_stream_map_length()
	{
		uint16_t val = (psmlen[0] << 8) | psmlen[1];
		
		return val;
	}

	void set_stream_map_length(uint16_t val)
	{
		psmlen[0] = (val >> 8) & 0xff;
		psmlen[1] = val & 0xff;
	}

	uint16_t get_stream_info_length()
	{
		uint16_t val = (sil[0] << 8) | sil[1];

		return val;
	}

	void set_stream_info_length(uint16_t val)
	{
		sil[0] = (val >> 8) & 0xff;
		sil[1] = val & 0xff;
	}

	/*
	uint16_t get_elementary_stream_map_length()
	{
		uint16_t val = (esml[0] << 8) | esml[1];
	}

	void set_elementary_stream_map_length(uint16_t val)
	{
		esml[0] = (val >> 8) & 0xFF;
		esml[1] = val & 0xFF;
	}
	*/

}PS_PSM, *PPS_PSM;

//ps pes

typedef struct _ps_pts
{
	uint8_t mark_1 : 1;		//marker bit, must be '1'
	uint8_t pts_1 : 3;		//33bit pts [32..30]
	uint8_t fix : 4;		//fixed bit

	uint8_t pts_2;			//33bit pts [29..22]

	uint8_t mark_2 : 1;		//marker bit, must be '1'
	uint8_t pts_3 : 7;		//33bit pts [21..15]

	uint8_t pts_4;			//33bit pts [14..7]

	uint8_t mark_3 : 1;		//marker bit, must be '1'
	uint8_t pts_5 : 7;		////33bit pts [6..0]

	_ps_pts()
		: mark_1(1), pts_1(0), fix(0)
		, pts_2(0)
		, mark_2(1), pts_3(0)
		, pts_4(0)
		, mark_3(1)
		, pts_5(0)
	{
	}


	uint64_t get()
	{
		uint64_t val = (pts_1 << 30) |
			(pts_2 << 22) |
			(pts_3 << 15) |
			(pts_4 << 7) |
			pts_5;

		return val;
	}

	void set(uint64_t val)
	{
		pts_1 = (val >> 30) & 0x07;
		pts_2 = (val >> 22) & 0xff;
		pts_3 = (val >> 15) & 0x7f;
		pts_4 = (val >> 7) & 0xff;
		pts_5 = val & 0x7f;
	}


}PS_PTS, *PPS_PTS;

typedef _ps_pts _ps_dts;

typedef _ps_dts PS_DTS;

typedef PS_DTS* PPS_DTS;

typedef struct _ps_pes
{
	uint8_t startcode[3];							//startcode, must be '0x000001'

	uint8_t sid;									//map_stream_id

	uint8_t ppl[2];									//PES_packet_length

	uint8_t ooc : 1;								//original_or_copy, '1'  original, '0' copy
	uint8_t cr : 1;									//copyright
	uint8_t dai : 1;								//data_alignment_indicator
	uint8_t pp : 1;									//PES_priority
	uint8_t psc : 2;								//PES_scrambling_control, '00' no scrambling, '01' '10' '11' custom scrambling
	uint8_t fix : 2;								//fixed bit, must be '10'

	uint8_t pef : 1;								//PES_extension_flag
	uint8_t pcf : 1;								//PES_CRC_flag
	uint8_t acif : 1;								//additional_copy_info_flag
	uint8_t dtmf : 1;								//DSM_trick_mode_flag
	uint8_t erf : 1;								//ES_rate_flag
	uint8_t ef : 1;									//ESCR_flag
	uint8_t pdf : 2;								//PTS_DTS_flags, '00' not pts, no dts, '01' invalid, must not be, '10' have pts, no dts, '11' have pts, have dts
	
	uint8_t	phdl;									//PES_header_data_length

	_ps_pes()
		: sid(0)
		, ooc(1), cr(1), dai(0), pp(0), psc(0), fix(0x02)
		, pef(0), pcf(0), acif(0), dtmf(0), erf(0), ef(0), pdf(0)
		, phdl(0)
	{
		startcode[0] = 0x00;
		startcode[1] = 0x00;
		startcode[2] = 0x01;

		ppl[0] = 0;
		ppl[1] = 0;
	 }

	uint16_t get_pes_packet_length()
	{
		uint16_t val = (ppl[0] << 8) | ppl[1];

		return val;
	}

	void set_pes_packet_length(uint16_t val)
	{
		ppl[0] = (val >> 8) & 0xff;
		ppl[1] = val & 0xff;
	}

}PS_PES, *PPS_PES;

#pragma pack(pop)

#endif