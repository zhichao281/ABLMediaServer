#ifndef _PS_MUX_DEMUX_DEMUX_MANAGER_CONSUMER_H_
#define _PS_MUX_DEMUX_DEMUX_MANAGER_CONSUMER_H_

#include <mutex>
#include <map>

#include "demux.h"

class demux_manager 
{
public:
	ps_demux_ptr malloc(ps_demux_callback cb, void* userdata, int32_t mode);
	void free();

	bool push(ps_demux_ptr p);
	bool pop(uint32_t h);
	ps_demux_ptr get(uint32_t h);

public:
	static demux_manager& getInstance();
private:
	demux_manager() = default;

	~demux_manager() = default;

	demux_manager(const demux_manager&) = delete;

	demux_manager& operator=(const demux_manager&) = delete;


private:
	std::map<uint32_t, ps_demux_ptr> m_duxmap;
	std::mutex  m_duxmtx;


};


#endif