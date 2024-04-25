#pragma once
#ifdef USE_BOOST

#include <boost/unordered_map.hpp>
#include <boost/serialization/singleton.hpp>
#include "server.h"
#include "auto_lock.h"

class server_manager
{
public:
	server_manager(void);
	~server_manager(void);

	bool push_server(server_ptr& s);
	bool pop_server(NETHANDLE id);
	void close_all_servers();
	server_ptr get_server(NETHANDLE id);

private:
	typedef boost::unordered_map<NETHANDLE, server_ptr>::iterator servermapiter;
	typedef boost::unordered_map<NETHANDLE, server_ptr>::const_iterator const_servermapiter;

private:
	boost::unordered_map<NETHANDLE, server_ptr> m_servers;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif
};
typedef boost::serialization::singleton<server_manager> server_manager_singleton;


#else
#include <map>
#include <mutex>

#include "server.h"
#include "auto_lock.h"

class server_manager
{
public:
	bool push_server(server_ptr& s);
	bool pop_server(NETHANDLE id);
	void close_all_servers();
	server_ptr get_server(NETHANDLE id);

public:
	static server_manager& getInstance();

private:
	server_manager();

	~server_manager();

	server_manager(const server_manager&) = delete;

	server_manager& operator=(const server_manager&) = delete;

	
private:
	std::map<NETHANDLE, server_ptr> m_servers;

	std::mutex          m_climtx;

};


#endif
