#ifndef _CLIENT_MANAGER_H_
#define _CLIENT_MANAGER_H_ 

#ifdef USE_BOOST
#include <boost/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#else
#include <unordered_map>
#include <memory>
#endif

#include "client.h"
#include "unordered_object_pool.h"

#define CLIENT_POOL_OBJECT_COUNT 1000
#define CLIENT_POOL_MAX_KEEP_COUNT 100 

typedef simple_pool::unordered_object_pool<client> client_pool;
#ifdef USE_BOOST
typedef boost::shared_ptr<client_pool> client_pool_ptr;
#else
typedef std::shared_ptr<client_pool> client_pool_ptr;
#endif

class client_manager
{
public:
    client_manager(void);
    ~client_manager(void);

    static client_manager& get_instance();

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
#ifdef USE_BOOST
    typedef boost::unordered_map<NETHANDLE, client_ptr>::iterator climapiter;
    typedef boost::unordered_map<NETHANDLE, client_ptr>::const_iterator const_climapiter;
#else
    typedef std::unordered_map<NETHANDLE, client_ptr>::iterator climapiter;
    typedef std::unordered_map<NETHANDLE, client_ptr>::const_iterator const_climapiter;
#endif

private:
    client_pool m_pool;
#ifdef USE_BOOST
    boost::unordered_map<NETHANDLE, client_ptr> m_clients;
#else
    std::unordered_map<NETHANDLE, client_ptr> m_clients;
#endif
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
    auto_lock::al_mutex m_poolmtx;
    auto_lock::al_mutex m_climtx;
#else
    auto_lock::al_spin m_poolmtx;
    std::mutex          m_climtx;
#endif

    // ½ûÖ¹¿½±´ºÍ¸³Öµ
    client_manager(const client_manager&) = delete;
    client_manager& operator=(const client_manager&) = delete;
};

#ifdef USE_BOOST
typedef boost::serialization::singleton<client_manager> client_manager_singleton;


#else


#endif



#endif