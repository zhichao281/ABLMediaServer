
#ifdef USE_BOOST


#include "server_manager.h"
server_manager::server_manager(void)
{
}

server_manager::~server_manager(void)
{
	close_all_servers();
}

bool server_manager::push_server(server_ptr& s)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (s)
	{
		std::pair<const_servermapiter, bool> ret = m_servers.insert(std::make_pair(s->get_id(), s));
		return ret.second;
	}

	return false;
}

bool server_manager::pop_server(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	const_servermapiter iter = m_servers.find(id);
	if (m_servers.end() != iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}
		m_servers.erase(iter);

		return true;
	}

	return false;
}

void server_manager::close_all_servers()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	for (const_servermapiter iter = m_servers.begin(); m_servers.end() != iter; ++iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}
	}

	m_servers.clear();
}

server_ptr server_manager::get_server(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	server_ptr s;
	const_servermapiter iter = m_servers.find(id);
	if (m_servers.end() != iter)
	{
		s = iter->second;
	}

	return s;
}


#else

#include "server_manager.h"

server_manager::server_manager(void)
{
}

server_manager::~server_manager(void)
{
	close_all_servers();
}

bool server_manager::push_server(server_ptr& s)
{

	std::lock_guard<std::mutex> lock(m_climtx);


	if (s)
	{
		auto ret = m_servers.insert(std::make_pair(s->get_id(), s));
		return ret.second;
	}

	return false;
}

bool server_manager::pop_server(NETHANDLE id)
{

	std::lock_guard<std::mutex> lock(m_climtx);


	auto iter = m_servers.find(id);
	if (m_servers.end() != iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}
		m_servers.erase(iter);

		return true;
	}

	return false;
}

void server_manager::close_all_servers()
{

	std::lock_guard<std::mutex> lock(m_climtx);


	for (auto iter : m_servers)
	{
		if (iter.second)
		{
			iter.second->close();
		}
	}

	m_servers.clear();
}

server_ptr server_manager::get_server(NETHANDLE id)
{

	std::lock_guard<std::mutex> lock(m_climtx);


	server_ptr s = nullptr;
	auto iter = m_servers.find(id);
	if (m_servers.end() != iter)
	{
		s = iter->second;
	}
	return s;
}

server_manager& server_manager::getInstance()
{
		static server_manager instance;
	return instance;
}


#endif
