#pragma once

#include <stdint.h>
#include <memory>
#include <cstring>
#include <unordered_map>
#include <map>
#include "ps_demux.h"
#include "consumer_base.h"

class ps_demux
{
public:
	ps_demux(ps_demux_callback cb, void* userdata, int32_t duxmode);
	~ps_demux();

	uint32_t get_id() const { return m_id; }

	int32_t handle(uint8_t* data, uint32_t datasize);

private:
	int32_t handle_by_timestamp(uint8_t* data, uint32_t datasize);
	int32_t handle_by_packet(uint8_t* data, uint32_t datasize);

	bool check_startcode(uint8_t* data, uint32_t datasize, uint8_t& sid);

	consumer_base_ptr& get_consumer(uint8_t sid, bool create);
	bool get_streamtype(uint8_t sid, uint8_t& st);

	void output();

private:
	const uint32_t m_id;
	const ps_demux_callback m_cb;
	const void* m_userdata;
	const int32_t m_duxmode;

	std::unordered_map<uint8_t, consumer_base_ptr> m_consumermap;

	_ps_demux_cb m_out;
};
typedef std::shared_ptr<ps_demux> ps_demux_ptr;

