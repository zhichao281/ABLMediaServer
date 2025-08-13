#pragma once

#include <unordered_map>
#include <mutex>



#include "mux.h"


class ps_mux_manager
{
public:
	ps_mux_ptr malloc(ps_mux_callback cb, void* userdata, int32_t alignmode, int32_t ttmode, int32_t ttincre);
	void free(ps_mux_ptr& p);

	bool push(ps_mux_ptr p);
	bool pop(uint32_t h);
	ps_mux_ptr get(uint32_t h);



public:
	static ps_mux_manager& getInstance();
	
private:
	ps_mux_manager() = default;
	~ps_mux_manager() = default;
	ps_mux_manager(const ps_mux_manager&) = delete;
	ps_mux_manager& operator=(const ps_mux_manager&) = delete;

	

private:
	std::unordered_map<uint32_t, ps_mux_ptr> m_muxmap;


	std::mutex  m_muxmtx;




};

