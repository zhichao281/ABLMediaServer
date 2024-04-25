#pragma once
#ifdef USE_BOOST



#include <boost/unordered_map.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include "client.h"
#include "unordered_object_pool.h"

#define CLIENT_POOL_OBJECT_COUNT 1000
#define CLIENT_POOL_MAX_KEEP_COUNT 100 

typedef simple_pool::unordered_object_pool<client> client_pool;
typedef boost::shared_ptr<client_pool> client_pool_ptr;

class client_manager
{
public:
	client_manager(void);
	~client_manager(void);

	client_ptr malloc_client(boost::asio::io_context& ioc,
		NETHANDLE srvid,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	void free_client(client* cli);
	bool push_client(client_ptr& cli);
	bool pop_client(NETHANDLE id);
	void pop_server_clients(NETHANDLE srvid);
	void pop_all_clients();
	client_ptr get_client(NETHANDLE id);

private:
	typedef boost::unordered_map<NETHANDLE, client_ptr>::iterator climapiter;
	typedef boost::unordered_map<NETHANDLE, client_ptr>::const_iterator const_climapiter;

private:
	client_pool m_pool;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_poolmtx;
#else
	auto_lock::al_spin m_poolmtx;
#endif

	boost::unordered_map<NETHANDLE, client_ptr> m_clients;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_climtx;
#else
	std::mutex          m_climtx;
#endif
};

typedef boost::serialization::singleton<client_manager> client_manager_singleton;



#else
#include <memory>
#include <map>
#include "client.h"
#include "unordered_object_pool.h"


#define CLIENT_POOL_OBJECT_COUNT 1000
#define CLIENT_POOL_MAX_KEEP_COUNT 100 


typedef simple_pool::unordered_object_pool<client> client_pool;
typedef std::shared_ptr<client_pool> client_pool_ptr;



class client_manager
{
public:
	client_manager(void);
	~client_manager(void);


	client_ptr malloc_client(asio::io_context& ioc,
		NETHANDLE srvid,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);


	void free_client(client* cli);
	bool push_client(client_ptr& cli);
	bool pop_client(NETHANDLE id);
	void pop_server_clients(NETHANDLE srvid);
	void pop_all_clients();
	client_ptr get_client(NETHANDLE id);
public:
	// 获取线程池实例
	static client_manager& getInstance();
private:

	client_manager(const client_manager&) = delete;
	client_manager& operator=(const client_manager&) = delete;
private:
	client_pool m_pool;
	std::mutex          m_poolmtx;
	std::mutex          m_climtx;
	std::map<NETHANDLE, client_ptr> m_clients;


};


#endif

