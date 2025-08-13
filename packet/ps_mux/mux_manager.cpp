
#include <memory>

#include "mux_manager.h"



ps_mux_ptr ps_mux_manager::malloc(ps_mux_callback cb, void* userdata, int32_t alignmode, int32_t ttmode, int32_t ttincre)
{
	ps_mux_ptr p;

	try
	{
		p = std::make_shared<ps_mux>(cb, userdata, alignmode, ttmode, ttincre);
	}
	catch (const std::bad_alloc&)
	{
	}
	catch (...)
	{
	}
	

	return p;
}

void ps_mux_manager::free(ps_mux_ptr& p)
{

}

bool ps_mux_manager::push(ps_mux_ptr p)
{
	if (!p)
	{
		return false;
	}


	std::lock_guard<std::mutex> lg(m_muxmtx);



	std::pair<std::unordered_map<uint32_t, ps_mux_ptr>::iterator, bool> ret = m_muxmap.insert(std::make_pair(p->get_id(), p));

	return ret.second;
}

bool ps_mux_manager::pop(uint32_t h)
{

	std::lock_guard<std::mutex> lg(m_muxmtx);




	std::unordered_map<uint32_t, ps_mux_ptr>::iterator it = m_muxmap.find(h);
	if (m_muxmap.end() != it)
	{
		m_muxmap.erase(it);

		return true;
	}

	return false;
}

ps_mux_ptr ps_mux_manager::get(uint32_t h)
{

	std::lock_guard<std::mutex> lg(m_muxmtx);
	ps_mux_ptr p;
	auto it = m_muxmap.find(h);
	if (m_muxmap.end() != it)
	{
		p = it->second;
	}

	return p;
}

ps_mux_manager& ps_mux_manager::getInstance()
{
	static ps_mux_manager instance;
	return instance;
}


