#include <memory>

#include "session_manager.h"


rtp_session_ptr rtp_session_manager::malloc(rtp_packet_callback cb, void* userdata)
{
	rtp_session_ptr p;

	try
	{
		p = std::make_shared<rtp_session_packet>(cb, userdata);
	}
	catch (const std::bad_alloc& /*e*/)
	{	
	}
	catch (...)
	{
	}
	
	return p;
}
// 获取单实例对象
rtp_session_manager& rtp_session_manager::getInstance() {
	static rtp_session_manager instance;
	return instance;
}
void rtp_session_manager::free(rtp_session_ptr p)
{
}

bool rtp_session_manager::push(rtp_session_ptr s)
{
	if (!s)
	{
		return false;
	}

	std::lock_guard<std::mutex> lg(m_sesmapmtx);

	 auto ret = m_sessionmap.insert(std::make_pair(s->get_id(), s));

	return ret.second;
}

bool rtp_session_manager::pop(uint32_t h)
{
	std::lock_guard<std::mutex> lg(m_sesmapmtx);
	auto it = m_sessionmap.find(h);
	if (m_sessionmap.end() != it)
	{
		m_sessionmap.erase(it);

		return true;
	}

	return false;
}

rtp_session_ptr rtp_session_manager::get(uint32_t h)
{
	std::lock_guard<std::mutex> lg(m_sesmapmtx);

	rtp_session_ptr s;

	auto it = m_sessionmap.find(h);
	if (m_sessionmap.end() != it)
	{
		s = it->second;
	}

	return s;

}