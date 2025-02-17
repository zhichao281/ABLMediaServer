#pragma  once 

#ifdef USE_BOOST
#ifndef _SERVER_H_
#define _SERVER_H_

#include <boost/function.hpp>
#include "client_manager.h"
#include "auto_lock.h"
#include <boost/asio/ssl.hpp>

class server : public boost::enable_shared_from_this<server>
{
public:
	server(boost::asio::io_context& ioc,
		boost::asio::ip::tcp::endpoint& ep,
		accept_callback fnaccept,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	~server();

	int32_t run();
	void close();
	NETHANDLE get_id();
	boost::atomic_bool sslFlag;

	void init_SSL();
private:
	void start_accept();
	void handle_accept(client_ptr c, const boost::system::error_code& error);
	void disconnect_clients();

private:
	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::ip::tcp::endpoint m_endpoint;
	accept_callback m_fnaccept;
	read_callback m_fnread;
	close_callback m_fnclose;
	const bool m_autoread;
	NETHANDLE m_id;
	boost::asio::ssl::context m_context;
};
typedef boost::shared_ptr<server>  server_ptr;

inline NETHANDLE server::get_id()
{
	return m_id;
}

inline void server::disconnect_clients()
{
	client_manager_singleton::get_mutable_instance().pop_server_clients(get_id());
}

#endif

#else
#include <functional>
#include <memory>
#include "client_manager.h"
#include "auto_lock.h"
#include <ssl.hpp>
class server : public std::enable_shared_from_this<server>
{
public:
	server(asio::io_context& ioc,
		asio::ip::tcp::endpoint& ep,
		accept_callback fnaccept,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	~server();

	int32_t run();
	void close();
	NETHANDLE get_id();
	std::atomic<bool> sslFlag;

	void init_SSL();
private:
	void start_accept();
	void handle_accept(client_ptr c, std::error_code error);
	void disconnect_clients();

private:
	asio::ip::tcp::acceptor m_acceptor;
	asio::ip::tcp::endpoint m_endpoint;
	accept_callback m_fnaccept;
	read_callback m_fnread;
	close_callback m_fnclose;
	const bool m_autoread;
	NETHANDLE m_id;
	asio::ssl::context m_context;
};

typedef std::shared_ptr<server>  server_ptr;

inline NETHANDLE server::get_id()
{
	return m_id;
}

inline void server::disconnect_clients()
{
	client_manager::getInstance().pop_server_clients(get_id());
}


#endif
