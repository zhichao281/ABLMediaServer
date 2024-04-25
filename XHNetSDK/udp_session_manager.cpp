
#ifdef USE_BOOST

#include "udp_session_manager.h"


udp_session_manager::udp_session_manager()
{
}


udp_session_manager::~udp_session_manager()
{
}

bool udp_session_manager::push_udp_session(udp_session_ptr& s)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (s)
	{
		std::pair<boost::unordered_map<NETHANDLE, udp_session_ptr>::iterator, bool> ret = m_sessions.insert(std::make_pair(s->get_id(), s));
		return ret.second;
	}

	return false;
}

bool udp_session_manager::pop_udp_session(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	boost::unordered_map<NETHANDLE, udp_session_ptr>::iterator iter = m_sessions.find(id);
	if (m_sessions.end() != iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}
		m_sessions.erase(iter);

		return true;
	}

	return false;
}

void udp_session_manager::pop_all_udp_sessions()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	for (boost::unordered_map<NETHANDLE, udp_session_ptr>::iterator iter = m_sessions.begin(); m_sessions.end() != iter; )
	{
		if (iter->second)
		{
			iter->second->close();
		}

		m_sessions.erase(iter++);
	}
}

udp_session_ptr udp_session_manager::get_udp_session(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	udp_session_ptr s;
	boost::unordered_map<NETHANDLE, udp_session_ptr>::iterator iter = m_sessions.find(id);
	if (m_sessions.end() != iter)
	{
		s = iter->second;
	}

	return s;
}

#else


#include "udp_session_manager.h"
#include <iostream>

udp_session_manager::udp_session_manager()
{
}


udp_session_manager::~udp_session_manager()
{
	std::cout << "delete udp_session_manager start " << std::endl;
}

bool udp_session_manager::push_udp_session(udp_session_ptr& s)
{
	std::lock_guard<std::mutex> lock(m_climtx);

	if (s)
	{
		auto ret = m_sessions.insert(std::make_pair(s->get_id(), s));
		return ret.second;
	}

	return false;
}

bool udp_session_manager::pop_udp_session(NETHANDLE id)
{

	std::lock_guard<std::mutex> lock(m_climtx);
#
	auto iter = m_sessions.find(id);
	if (m_sessions.end() != iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}
		m_sessions.erase(iter);

		return true;
	}

	return false;
}

void udp_session_manager::pop_all_udp_sessions()
{

	std::lock_guard<std::mutex> lock(m_climtx);

	for (auto iter = m_sessions.begin(); m_sessions.end() != iter; )
	{
		if (iter->second)
		{
			iter->second->close();
		}

		m_sessions.erase(iter++);
	}
}

udp_session_ptr udp_session_manager::get_udp_session(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	std::lock_guard<std::mutex> lock(m_climtx);
#endif

	udp_session_ptr s = nullptr;
	auto iter = m_sessions.find(id);
	if (m_sessions.end() != iter)
	{
		s = iter->second;
	}

	return s;
}

udp_session_manager& udp_session_manager::getInstance()
{

	static udp_session_manager instance;
	return instance;

}
#endif
