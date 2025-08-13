

#include <memory>
#include "demux_manager.h"

ps_demux_ptr demux_manager::malloc(ps_demux_callback cb, void* userdata, int32_t mode)
{
	ps_demux_ptr p;

	try
	{
		p = std::make_shared<ps_demux>(cb, userdata, mode);
	}
	catch (const std::bad_alloc& /*e*/)
	{
	}
	catch (...)
	{
	}

	return p;
}

void demux_manager::free()
{
}

bool demux_manager::push(ps_demux_ptr p)
{
	if (!p)
	{
		return false;
	}
	std::lock_guard<std::mutex> lg(m_duxmtx);
	auto ret = m_duxmap.insert(std::make_pair(p->get_id(), p));
	return ret.second;
}

bool demux_manager::pop(uint32_t h)
{

	std::lock_guard<std::mutex> lg(m_duxmtx);
	auto it = m_duxmap.find(h);
	if (m_duxmap.end() != it)
	{
		m_duxmap.erase(it);

		return true;
	}

	return false;
}

ps_demux_ptr demux_manager::get(uint32_t h)
{

	std::lock_guard<std::mutex> lg(m_duxmtx);
	ps_demux_ptr p;
	auto it = m_duxmap.find(h);
	if (m_duxmap.end() != it)
	{
		p = it->second;
	}

	return p;
}

demux_manager& demux_manager::getInstance()
{
	static demux_manager instance;
	return instance;
}

