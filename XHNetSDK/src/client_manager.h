#ifndef _CLIENT_MANAGER_H_
#define _CLIENT_MANAGER_H_ 

#include "client.h"
#include "unordered_object_pool.h"



#ifdef USE_BOOST
#include <boost/unordered_map.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include "unordered_object_pool.h"
#else
#include <unordered_map>
#include <memory>
#include "./common/thread_pool.h"
#endif



#define CLIENT_POOL_OBJECT_COUNT 1000
#define CLIENT_POOL_MAX_KEEP_COUNT 100 

#ifdef USE_BOOST
typedef simple_pool::unordered_object_pool<client> client_pool;
typedef boost::shared_ptr<client_pool> client_pool_ptr;
#endif

class client_manager
{
public:
    client_manager(void);
    ~client_manager(void);

#ifndef USE_BOOST
    static client_manager& get_instance();
#endif

    client_ptr malloc_client(
        NETHANDLE srvid,
        read_callback fnread,
        close_callback fnclose,
        bool autoread,
        bool bSSLFlag,
        ClientType nCLientType,
        accept_callback fnaccept);
    void free_client(client* cli);
    bool push_client(client_ptr& cli);
    bool pop_client(NETHANDLE id);
    void pop_server_clients(NETHANDLE srvid);
    void pop_all_clients();
    client_ptr get_client(NETHANDLE id);


private:

	client_manager(const client_manager&) = delete;
	client_manager& operator=(const client_manager&) = delete;

private:
#ifdef USE_BOOST
    typedef boost::unordered_map<NETHANDLE, client_ptr>::iterator climapiter;
    typedef boost::unordered_map<NETHANDLE, client_ptr>::const_iterator const_climapiter;
    client_pool m_pool;
    boost::unordered_map<NETHANDLE, client_ptr> m_clients;
#else
    typedef std::unordered_map<NETHANDLE, client_ptr>::iterator climapiter;
    typedef std::unordered_map<NETHANDLE, client_ptr>::const_iterator const_climapiter;
    //netlib::ThreadPool m_pool; // 用线程池替代对象池
    std::unordered_map<NETHANDLE, client_ptr> m_clients;
#endif

    std::mutex          m_climtx;
    std::mutex          m_poolmtx;





};

#ifdef USE_BOOST

typedef boost::serialization::singleton<client_manager> client_manager_singleton;

#endif



#endif