#include <memory>

#include "session_manager.h"
#include "depacket.h"
rtp_session_ptr rtp_session_manager::malloc(rtp_depacket_callback cb, void* userdata)
{
	rtp_session_ptr s;
	try
	{
		s = std::make_shared<rtp_session>(cb, userdata);
	}
	catch (const std::bad_alloc& /*e*/)
	{
	}
	catch (...)
	{
	}

	return s;
}

void rtp_session_manager::free(rtp_session_ptr s)
{

}

bool rtp_session_manager::push(rtp_session_ptr s)
{
	if (!s)
	{
		return false;
	}


	std::lock_guard<std::mutex> lg(m_sesMapMtx);



	std::pair<std::unordered_map<uint32_t, rtp_session_ptr>::iterator, bool> ret = m_sessionMap.insert(std::make_pair(s->get_id(), s));

	return ret.second;
}

bool rtp_session_manager::pop(uint32_t h)
{
	std::lock_guard<std::mutex> lg(m_sesMapMtx);
	std::unordered_map<uint32_t, rtp_session_ptr>::iterator it = m_sessionMap.find(h);
	if (m_sessionMap.end() != it)
	{
		m_sessionMap.erase(it);

		return true;
	}

	return false;
}

rtp_session_ptr rtp_session_manager::get(uint32_t h)
{

	std::lock_guard<std::mutex> lg(m_sesMapMtx);
	rtp_session_ptr s;
	std::unordered_map<uint32_t, rtp_session_ptr>::iterator it = m_sessionMap.find(h);
	if (m_sessionMap.end() != it)
	{
		s = it->second;
	}

	return s;
	
}