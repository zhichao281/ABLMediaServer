#pragma once

#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <string.h> // »ò #include <cstring>

#include "ps_mux.h"
#include "ps_def.h"

#define PS_MUX_OUTPUT_BUFFER_MAX_SIZE (3 * 1024 * 1024)
#define PS_MUX_PRIVATE_STREAMING_BUFFER_MAX_SIZE (512 * 1024)
#define PS_MUX_AUDIO_BUFFER_MAX_SIZE (512 * 1024)
#define PS_MUX_PER_PES_PAYLOAD_MAX_SIZE 40960

class ps_mux
{
public:
	ps_mux(ps_mux_callback cb, void* userdata, int32_t alignmode, int32_t ttmode, int32_t ttincre);
	~ps_mux();

	uint32_t get_id() const;

	int32_t handle(const _ps_mux_input* in);

private:
	void add_stream(int32_t mt, uint8_t st, uint8_t sid);
	void add_pstd(int32_t mt, uint8_t sid);
	uint8_t generate_video_sid();
	uint8_t generate_audio_sid();

	int32_t produce_psheader(uint64_t timestamp);
	int32_t produce_pssysheader();
	int32_t produce_psm();
	int32_t produce_private(const _ps_mux_input* in);
	int32_t produce_pesv(const _ps_mux_input* in);
	int32_t produce_pesa(const _ps_mux_input* in);

	uint8_t get_sid(int32_t mt, uint8_t st, uint8_t sid);
	
private:
	uint32_t m_id;
	uint8_t m_outbuf[PS_MUX_OUTPUT_BUFFER_MAX_SIZE];
	uint32_t m_outbufsize;
	_ps_mux_cb m_out;
	uint8_t m_sidbasev;
	uint8_t m_sidbasea;

	ps_mux_callback m_cb;
	int32_t m_alimod;
	int32_t m_ttmod;
	int32_t m_ttincre;
	uint32_t m_ttbase;
	int32_t m_ttincrea;
	uint32_t m_ttbasea;

	std::unordered_map<uint8_t, uint8_t> m_stsidmap;

	_ps_header m_psh;
	_ps_sys_header m_pssys;
	std::vector<_ps_pstd> m_pstd;
	_ps_psm m_psm;
	_ps_pes m_pes;

	bool have_audio_cache_;
	uint32_t auido_cache_count_;

};
typedef std::shared_ptr<ps_mux> ps_mux_ptr;

