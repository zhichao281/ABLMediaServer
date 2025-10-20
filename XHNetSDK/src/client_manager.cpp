
#include "client_manager.h"

#ifdef USE_BOOST
#include <boost/make_shared.hpp>

#else

#endif


struct client_deletor
{
	void operator()(client* cli)
	{
		if (cli)
		{
#ifdef USE_BOOST
			typedef boost::serialization::singleton<client_manager> client_manager_singleton;


#else
			client_manager::get_instance().free_client(cli);

#endif

		
		}
	}
};

#ifdef USE_BOOST
client_manager::client_manager(void)
	: m_pool(CLIENT_POOL_OBJECT_COUNT, CLIENT_POOL_OBJECT_COUNT)
{
}
#else
client_manager::client_manager(void)
{

}
#endif

client_manager::~client_manager(void)
{
	pop_all_clients();
}

client_ptr client_manager::malloc_client(
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread,
	bool bSSLFlag,
	ClientType nCLientType,
	accept_callback  fnaccept)
{
//#ifdef LIBNET_USE_CORE_SYNC_MUTEX
//	auto_lock::al_lock<auto_lock::al_mutex> al(m_poolmtx);
//#else
//	auto_lock::al_lock<auto_lock::al_spin> al(m_poolmtx);
//#endif
//
//	client_ptr cli;
//	cli.reset(m_pool.construct(srvid, fnread, fnclose, autoread, bSSLFlag, nCLientType,fnaccept), client_deletor());
//
//	return cli;

	std::unique_lock<std::mutex> _lock(m_poolmtx);
	// 使用 std::make_shared 创建 shared_ptr，并传入自定义删除器
	client_ptr cli = std::shared_ptr<client>(
		new client(srvid, fnread, fnclose, autoread, bSSLFlag, nCLientType, fnaccept),
		[](client* cli) {
			if (cli) {
				client_manager::get_instance().free_client(cli);
			}
		}
	);

	return cli;
}

void client_manager::free_client(client* cli)
{

	std::lock_guard<std::mutex> lock(m_poolmtx);
	if (cli)
	{
		delete cli;  // 直接删除对象，不再使用 m_pool.destroy()
		cli = nullptr;
	}


}

bool client_manager::push_client(client_ptr& cli)
{
	std::lock_guard<std::mutex> lock(m_climtx);

	if (cli)
	{
		std::pair<const_climapiter, bool> ret = m_clients.insert(std::make_pair(cli->get_id(), cli));
		return ret.second;
	}

	return false;
}

bool client_manager::pop_client(NETHANDLE id)
{
	std::lock_guard<std::mutex> lock(m_climtx);

	const_climapiter iter = m_clients.find(id);
	if (m_clients.end() != iter)
	{
		m_clients.erase(iter);

		return true;
	}

	return false;
}

void client_manager::pop_server_clients(NETHANDLE srvid)
{
	std::lock_guard<std::mutex> lock(m_climtx);
	for (const_climapiter iter = m_clients.begin(); m_clients.end() != iter;)
	{
		if (iter->second && (iter->second->get_server_id() == srvid))
		{
			m_clients.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

void client_manager::pop_all_clients()
{
	std::lock_guard<std::mutex> lock(m_climtx);

	for (const_climapiter iter = m_clients.begin(); iter != m_clients.end(); ++iter)
	{
		if (iter->second)
		{
			iter->second->close();
		}
	}

	m_clients.clear();
}

client_ptr client_manager::get_client(NETHANDLE id)
{
	std::lock_guard<std::mutex> lock(m_climtx);

	client_ptr cli = nullptr  ;
	const_climapiter iter = m_clients.find(id);
	if (m_clients.end() != iter)
	{
		cli = iter->second;
	}

	return cli;
}

client_manager& client_manager::get_instance() {
	static client_manager instance;
	return instance;
}