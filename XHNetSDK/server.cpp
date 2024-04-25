
#ifdef USE_BOOST

#include <boost/make_shared.hpp>
#include "io_context_pool.h"
#include "server.h"
#include "server_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"
#include <malloc.h>

#if (defined _WIN32)
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern io_context_pool g_iocpool;

server::server(boost::asio::io_context &ioc,
	boost::asio::ip::tcp::endpoint& ep,
	accept_callback fnaccept,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
	: m_acceptor(ioc)
	, m_endpoint(ep)
	, m_fnaccept(fnaccept)
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_autoread(autoread)
	, m_id(generate_identifier())
{
}

server::~server(void)
{
	recycle_identifier(m_id);
#ifndef _WIN32
	malloc_trim(0);
#endif
}

int32_t server::run()
{
	boost::system::error_code ec;
	if (!m_acceptor.is_open())
	{
		//open
		m_acceptor.open(m_endpoint.address().is_v4() ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(), ec);
		if (ec)
		{
			return e_libnet_err_srvlistensocknotopen;
		}

		/* 屏蔽掉地址重用设置，否则绑定相同端口时不会提示端口重复绑定提示 //set option*/
		boost::asio::ip::tcp::acceptor::reuse_address reuse_address_option(true);
		m_acceptor.set_option(reuse_address_option, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensocksetopt;
		}

		boost::asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
		m_acceptor.set_option(send_buffer_size_option, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensocksetopt;
		}

		boost::asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
		m_acceptor.set_option(recv_buffer_size_option, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensocksetopt;
		}


		m_acceptor.bind(m_endpoint, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensockbind;
		}

		//listen
		m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistenstart;
		}
	}

	uint32_t num = (0 == (g_iocpool.get_thread_count() / 2) ? 1 : (g_iocpool.get_thread_count() / 2));
	for (uint32_t n = 0; n < num; ++n)
	{
		start_accept();
	}

	return e_libnet_err_noerror;
}

void server::close()
{
	disconnect_clients();

	if (m_acceptor.is_open())
	{
		boost::system::error_code ec;
		m_acceptor.close(ec);
	}
}

void server::start_accept()
{
	client_ptr c;
	while (m_acceptor.is_open())
	{
		c = client_manager_singleton::get_mutable_instance().malloc_client(g_iocpool.get_io_context(), get_id(), m_fnread, m_fnclose, m_autoread);
		if (c)
		{
			break;
		}
		else
		{
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
		}
	}

	m_acceptor.async_accept(c->socket(),
		boost::bind(&server::handle_accept,
			shared_from_this(), c,
			boost::asio::placeholders::error));
}

void server::handle_accept(client_ptr c, const boost::system::error_code& ec)
{
	if (!ec)
	{
		NETHANDLE cliid = c->get_id();

		if (m_fnaccept)
		{
			boost::system::error_code ec;
			boost::asio::ip::tcp::endpoint endpoint = c->socket().remote_endpoint(ec);
			if (ec || !client_manager_singleton::get_mutable_instance().push_client(c))
			{
				c->close();
				start_accept();
				return;
			}

			if (endpoint.address().is_v4())
			{
				in_addr addr_n;
				inet_pton(AF_INET, endpoint.address().to_string().c_str(), &addr_n);

				sockaddr_in tAddr = { 0 };
				tAddr.sin_family = AF_INET;
				tAddr.sin_addr = addr_n;
				tAddr.sin_port = htons(endpoint.port());

				m_fnaccept(get_id(), c->get_id(), &tAddr);
			}
			else
			{
				in6_addr addr_n;
				inet_pton(AF_INET6, endpoint.address().to_string().c_str(), &addr_n);

				sockaddr_in6 tAddr = { 0 };
				tAddr.sin6_family = AF_INET6;
				tAddr.sin6_addr = addr_n;
				tAddr.sin6_port = htons(endpoint.port());

				m_fnaccept(get_id(), c->get_id(), &tAddr);
			}
		}

		c = client_manager_singleton::get_mutable_instance().get_client(cliid);
		if (c)
		{
			c->run();
		}

		start_accept();
	}
	else
	{
		if (server_manager_singleton::get_mutable_instance().pop_server(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_id(), INVALID_NETHANDLE);
			}
		}
	}
}

#else

#include <functional>
#include <memory>
#include "io_context_pool.h"
#include "server.h"
#include "server_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"
#include <malloc.h>
#include <iostream>
#if (defined _WIN32)
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

extern io_context_pool g_iocpool;

server::server(asio::io_context& ioc,
	asio::ip::tcp::endpoint& ep,
	accept_callback fnaccept,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
	: m_acceptor(ioc)
	, m_endpoint(ep)
	, m_fnaccept(fnaccept)
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_autoread(autoread)
	, m_id(generate_identifier())
{
}

server::~server(void)
{
	
	recycle_identifier(m_id);

#ifndef _WIN32
	malloc_trim(0);
#endif


}

int32_t server::run()
{
	std::error_code ec;
	if (!m_acceptor.is_open())
	{
		//open
		m_acceptor.open(m_endpoint.address().is_v4() ? asio::ip::tcp::v4() : asio::ip::tcp::v6(), ec);
		if (ec)
		{
			return e_libnet_err_srvlistensocknotopen;
		}

		/* 屏蔽掉地址重用设置，否则绑定相同端口时不会提示端口重复绑定提示 //set option*/
		asio::ip::tcp::acceptor::reuse_address reuse_address_option(true);
		m_acceptor.set_option(reuse_address_option, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensocksetopt;
		}

		asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
		m_acceptor.set_option(send_buffer_size_option, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensocksetopt;
		}

		asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
		m_acceptor.set_option(recv_buffer_size_option, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensocksetopt;
		}


		m_acceptor.bind(m_endpoint, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistensockbind;
		}

		//listen
		m_acceptor.listen(asio::socket_base::max_connections, ec);
		if (ec)
		{
			close();
			return e_libnet_err_srvlistenstart;
		}
	}

	uint32_t num = (0 == (g_iocpool.get_thread_count() / 2) ? 1 : (g_iocpool.get_thread_count() / 2));
	for (uint32_t n = 0; n < num; ++n)
	{
		start_accept();
	}

	return e_libnet_err_noerror;
}

void server::close()
{
	disconnect_clients();

	if (m_acceptor.is_open())
	{
		asio::error_code ec;
		m_acceptor.close(ec);
	}
}

void server::start_accept()
{
	client_ptr c;
	while (m_acceptor.is_open())
	{
		c = client_manager::getInstance().malloc_client(g_iocpool.get_io_context(), get_id(), m_fnread, m_fnclose, m_autoread);
		if (c)
		{
			break;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		}
	}

	m_acceptor.async_accept(c->socket(),
		std::bind(&server::handle_accept,
			shared_from_this(), c,
			std::placeholders::_1));
}

void server::handle_accept(client_ptr c, std::error_code ec)
{
	if (!ec)
	{
		NETHANDLE cliid = c->get_id();

		if (m_fnaccept)
		{
			std::error_code ec;
			asio::ip::tcp::endpoint endpoint = c->socket().remote_endpoint(ec);
			if (ec || !client_manager::getInstance().push_client(c))
			{
				c->close();
				start_accept();
				return;
			}

			if (endpoint.address().is_v4())
			{
				in_addr addr_n;
				inet_pton(AF_INET, endpoint.address().to_string().c_str(), &addr_n);

				sockaddr_in tAddr = { 0 };
				tAddr.sin_family = AF_INET;
				tAddr.sin_addr = addr_n;
				tAddr.sin_port = htons(endpoint.port());

				m_fnaccept(get_id(), c->get_id(), &tAddr);
			}
			else
			{
				in6_addr addr_n;
				inet_pton(AF_INET6, endpoint.address().to_string().c_str(), &addr_n);

				sockaddr_in6 tAddr = { 0 };
				tAddr.sin6_family = AF_INET6;
				tAddr.sin6_addr = addr_n;
				tAddr.sin6_port = htons(endpoint.port());

				m_fnaccept(get_id(), c->get_id(), &tAddr);
			}
		}

		c = client_manager::getInstance().get_client(cliid);
		if (c)
		{
			c->run();
		}

		start_accept();
	}
	else
	{
		if (server_manager::getInstance().pop_server(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_id(), INVALID_NETHANDLE);
			}
		}
	}
}


#endif

