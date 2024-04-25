
#ifdef USE_BOOST
#include <boost/bind.hpp>
#include "udp_session.h"
#include "udp_session_manager.h"
#include "identifier_generator.h"
#include <malloc.h>

#if (defined _WIN32 || defined _WIN64)
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

udp_session::udp_session(boost::asio::io_context& ioc)
	: m_socket(ioc)
	, m_fnread(NULL)
	, m_start(false)
	, m_autoread(false)
	, m_hasconnected(false)
	, m_userreadbuffer(NULL)
	, m_inreading(false)
	, m_id(generate_identifier())
{
	m_readbuff = new uint8_t[UDP_SESSION_MAX_BUFF_SIZE];
}

udp_session::~udp_session(void)
{
	recycle_identifier(m_id);
	if (m_readbuff != NULL)
	{
		delete[] m_readbuff;
		m_readbuff = NULL;
	}
#ifndef _WIN32
	malloc_trim(0);
#endif
}

int32_t udp_session::init(const int8_t* localip,
	uint16_t localport,
	void* bindaddr,
	read_callback fnread,
	bool autoread)
{
#ifdef LIBNET_NOT_ALLOW_AUTO_ADDRESS
	if ((!localip || (0 == strcmp(reinterpret_cast<const char*>(localip), ""))) && (0 == localport))
	{
		return e_libnet_err_invalidparam;
	}
#endif 

	m_fnread = fnread;
	m_autoread = autoread;

	int32_t ret = bind_address(localip, localport, bindaddr);

	if (e_libnet_err_noerror == ret)
	{
		if (m_autoread)
		{
			start_read();
		}

		m_start = true;
	}

	boost::system::error_code ec;
	boost::asio::socket_base::receive_buffer_size recv_buffer_size_option(4 * 1024 * 1024);
	m_socket.set_option(recv_buffer_size_option, ec);//设置接收缓冲区

	boost::asio::socket_base::send_buffer_size    SendSize_option(4 * 1024 * 1024); //定义发送缓冲区大小
	m_socket.set_option(SendSize_option, ec); //设置发送缓存区大小

	return ret;
}

int32_t udp_session::connect(const int8_t* remoteip,
	uint16_t remoteport)
{
	if (!remoteip || (0 == strcmp(reinterpret_cast<const char*>(remoteip), "") || (0 == remoteport)))
	{
		return e_libnet_err_invalidparam;
	}
	boost::system::error_code ec;
	boost::asio::ip::address addr = boost::asio::ip::address::from_string(reinterpret_cast<const char*>(remoteip), ec);
	if (ec)
	{
		close();
		return e_libnet_err_cliinvalidip;
	}

	m_endpoint.address(addr);
	m_endpoint.port(remoteport);

	m_socket.connect(m_endpoint, ec);
	if (ec)
	{
		close();
		return e_libnet_err_cliconnect;
	}
	else
	{
		m_hasconnected = true;
		return e_libnet_err_noerror;
	}
}

int32_t udp_session::recv_from(uint8_t* buffer,
	uint32_t* buffsize,
	void* remoteaddr,
	bool blocked)
{
#ifdef LIBNET_MULTI_THREAD_RECV_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_readmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_readmtx);
#endif
#endif

	if (!buffer || !buffsize || (0 == *buffsize))
	{
		return e_libnet_err_invalidparam;
	}

	if (m_autoread)
	{
		return e_libnet_err_cliautoread;
	}

	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	boost::system::error_code ec;
	if (blocked)
	{
		size_t readsize = m_socket.receive_from(boost::asio::buffer(buffsize, *buffsize), m_remoteep, 0, ec);
		if (ec || (0 == readsize))
		{
			*buffsize = 0;
			//close();
			udp_session_manager_singleton::get_mutable_instance().pop_udp_session(get_id());
			return e_libnet_err_clireaddata;
		}
		else
		{
			*buffsize = static_cast<uint32_t>(readsize);
			if (remoteaddr)
			{
				if (m_remoteep.address().is_v4())
				{
					in_addr addr_n;
					inet_pton(AF_INET, m_remoteep.address().to_string().c_str(), &addr_n);

					(static_cast<sockaddr_in*>(remoteaddr))->sin_family = AF_INET;
					(static_cast<sockaddr_in*>(remoteaddr))->sin_addr = addr_n;
					(static_cast<sockaddr_in*>(remoteaddr))->sin_port = htons(m_remoteep.port());
				}
				else
				{
					in6_addr addr_n;
					inet_pton(AF_INET6, m_remoteep.address().to_string().c_str(), &addr_n);

					(static_cast<sockaddr_in6*>(remoteaddr))->sin6_family = AF_INET6;
					(static_cast<sockaddr_in6*>(remoteaddr))->sin6_addr = addr_n;
					(static_cast<sockaddr_in6*>(remoteaddr))->sin6_port = htons(m_remoteep.port());
				}
			}

			return e_libnet_err_noerror;
		}
	}
	else
	{
		if (m_inreading)
		{
			return e_libnet_err_cliprereadnotfinish;
		}

		m_inreading = true;
		m_userreadbuffer = buffer;

		m_socket.async_receive_from(boost::asio::buffer(m_userreadbuffer, *buffsize), m_remoteep,
			boost::bind(&udp_session::handle_read,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		return e_libnet_err_noerror;
	}
}

int32_t udp_session::send_to(uint8_t* data,
	uint32_t datasize,
	void* remoteaddr)
{
#ifdef LIBNET_MULTI_THREAD_SEND_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_writemtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_writemtx);
#endif
#endif

	if (!data || (0 == datasize) || (!m_hasconnected && !remoteaddr))
	{
		return e_libnet_err_invalidparam;
	}

	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	boost::system::error_code ec;
	if (m_hasconnected)
	{
		m_socket.send(boost::asio::buffer(data, datasize), 0, ec);
#ifndef WIN32
		std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif // !WIN32

		
	}
	else
	{
		sockaddr_in* addr = static_cast<sockaddr_in*>(remoteaddr);

		m_writeep.address(boost::asio::ip::address::from_string(inet_ntoa(addr->sin_addr)));
		m_writeep.port(ntohs(addr->sin_port));
		m_socket.send_to(boost::asio::buffer(data, datasize), m_writeep, 0, ec);
#ifndef WIN32
		std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif // !WIN32
	}

	if (ec)
	{
		udp_session_manager_singleton::get_mutable_instance().pop_udp_session(get_id());
		return e_libnet_err_cliwritedata;
	}
	else
	{
		return e_libnet_err_noerror;
	}
}

int32_t	udp_session::close()
{
	m_start = false;
	m_fnread = NULL;
	if (m_socket.is_open())
	{
		boost::system::error_code ec;
		m_socket.close(ec);
	}

	return e_libnet_err_noerror;
}

int32_t udp_session::bind_address(const int8_t* localip,
	uint16_t localport,
	void* bindaddr)
{
	boost::system::error_code ec;

	boost::asio::ip::address addr;
	if (localip && (0 != strcmp(reinterpret_cast<const char*>(localip), "")))
	{
		addr = boost::asio::ip::address::from_string(reinterpret_cast<const char*>(localip), ec);
		if (ec)
		{
			return e_libnet_err_srvinvalidip;
		}
	}

	if (!m_socket.is_open())
	{
		m_socket.open(boost::asio::ip::udp::v4(), ec);
		if (ec)
		{
			return e_libnet_err_cliopensock;
		}
	}

	m_endpoint = boost::asio::ip::udp::endpoint(addr, localport);
	m_socket.bind(m_endpoint, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clibind;
	}

	m_endpoint = m_socket.local_endpoint(ec);
	if (ec)
	{
		close();
		return e_libnet_err_clibind;
	}

	//set option
	boost::asio::socket_base::reuse_address reuse_address_option(true);
	m_socket.set_option(reuse_address_option, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	boost::asio::socket_base::send_buffer_size send_buffer_size_option(UDP_SESSION_SEND_BUFF_SIZE);
	m_socket.set_option(send_buffer_size_option, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	boost::asio::socket_base::receive_buffer_size recv_buffer_size_option(UDP_SESSION_RECV_BUFF_SIZE);
	m_socket.set_option(recv_buffer_size_option, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}


	if (bindaddr)
	{
		if (m_remoteep.address().is_v4())
		{
			in_addr addr_n;
			inet_pton(AF_INET, m_remoteep.address().to_string().c_str(), &addr_n);

			(static_cast<sockaddr_in*>(bindaddr))->sin_family = AF_INET;
			(static_cast<sockaddr_in*>(bindaddr))->sin_addr = addr_n;
			(static_cast<sockaddr_in*>(bindaddr))->sin_port = htons(m_remoteep.port());
		}
		else
		{
			in6_addr addr_n;
			inet_pton(AF_INET6, m_remoteep.address().to_string().c_str(), &addr_n);

			(static_cast<sockaddr_in6*>(bindaddr))->sin6_family = AF_INET6;
			(static_cast<sockaddr_in6*>(bindaddr))->sin6_addr = addr_n;
			(static_cast<sockaddr_in6*>(bindaddr))->sin6_port = htons(m_remoteep.port());
		}
	}

	memset((char*)&tAddrV4, 0x00, sizeof(tAddrV4));
	memset((char*)&tAddrV6, 0x00, sizeof(tAddrV6));

	return e_libnet_err_noerror;
}

void udp_session::start_read()
{
	if (m_socket.is_open())
	{
		m_socket.async_receive_from(boost::asio::buffer(m_readbuff, UDP_SESSION_MAX_BUFF_SIZE),
			m_remoteep,
			boost::bind(&udp_session::handle_read,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void udp_session::handle_read(const boost::system::error_code& ec,
	size_t transize)
{
	if (!m_socket.is_open())
	{
		return;
	}

	if (!ec || ec == boost::asio::error::message_size)
	{
		if (m_start)
		{
			if (!m_hasconnected)
			{
				if (m_remoteep.address().is_v4())
				{
					inet_pton(AF_INET, m_remoteep.address().to_string().c_str(), &addr_nV4);

					tAddrV4.sin_family = AF_INET;
					tAddrV4.sin_addr = addr_nV4;
					tAddrV4.sin_port = htons(m_remoteep.port());

					if (m_fnread)
					{
						m_fnread(INVALID_NETHANDLE, get_id(), m_autoread ? m_readbuff : m_userreadbuffer, static_cast<uint32_t>(transize), &tAddrV4);
					}
				}
				else
				{
					inet_pton(AF_INET6, m_remoteep.address().to_string().c_str(), &addr_nV6);

					tAddrV6.sin6_family = AF_INET6;
					tAddrV6.sin6_addr = addr_nV6;
					tAddrV6.sin6_port = htons(m_remoteep.port());

					if (m_fnread)
					{
						m_fnread(INVALID_NETHANDLE, get_id(), m_autoread ? m_readbuff : m_userreadbuffer, static_cast<uint32_t>(transize), &tAddrV6);
					}
				}
			}
			else
			{
				if (m_fnread)
				{
					m_fnread(INVALID_NETHANDLE, get_id(), m_autoread ? m_readbuff : m_userreadbuffer, static_cast<uint32_t>(transize), NULL);
				}
			}

			if (m_autoread)
			{
				start_read();
			}
			else
			{
				m_userreadbuffer = NULL;
				m_inreading = false;
			}
		}
	}
	else
	{// 有时候会触发 10061 这个错误，造成udp关闭，不需要在这里调用关闭
		//close();
		//udp_session_manager_singleton::get_mutable_instance().pop_udp_session(get_id());
	}
}

int32_t udp_session::multicast(uint8_t option,
	const int8_t* ip,
	uint8_t value)
{
	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	boost::system::error_code ec;

	switch (option)
	{
	case MULTICAST_OPERATION_JOIN_GROUP:
	{
		if (!ip || (0 == strcmp(reinterpret_cast<const char*>(ip), "")))
		{
			return e_libnet_err_invalidparam;
		}

		boost::asio::ip::address addr = boost::asio::ip::address::from_string(reinterpret_cast<const char*>(ip), ec);
		if (ec)
		{
			return e_libnet_err_cliinvalidip;
		}

		boost::asio::ip::multicast::join_group join_group_option(addr);
		m_socket.set_option(join_group_option, ec);
		if (ec)
		{
			return e_libnet_err_clijoinmulticastgroup;
		}
	}
	break;
	case MULTICAST_OPERATION_LEAVE_GROUP:
	{
		if (!ip || (0 == strcmp(reinterpret_cast<const char*>(ip), "")))
		{
			return e_libnet_err_invalidparam;
		}

		boost::asio::ip::address addr = boost::asio::ip::address::from_string(reinterpret_cast<const char*>(ip), ec);
		if (ec)
		{
			return e_libnet_err_cliinvalidip;
		}

		boost::asio::ip::multicast::leave_group leave_group_option(addr);
		m_socket.set_option(leave_group_option, ec);
		if (ec)
		{
			return e_libnet_err_clileavemulticastgroup;
		}
	}
	break;
	case MULTICAST_OPERATION_LOOPBACK:
	{
		boost::asio::ip::multicast::enable_loopback enable_loopback_option(0 == value);
		m_socket.set_option(enable_loopback_option, ec);
		if (ec)
		{
			return e_libnet_err_clisetmulticastopt;
		}
	}
	break;
	case MULTICAST_OPERATION_HOPS:
	{
		boost::asio::ip::multicast::hops hop_option(value);
		m_socket.set_option(hop_option, ec);
		if (ec)
		{
			return e_libnet_err_clisetmulticastopt;
		}
	}
	break;
	case MULTICAST_OPERATION_OUTBOUND_INTERFACE:
	{
		if (!ip || (0 == strcmp(reinterpret_cast<const char*>(ip), "")))
		{
			return e_libnet_err_invalidparam;
		}

		boost::asio::ip::address addr = boost::asio::ip::address::from_string(reinterpret_cast<const char*>(ip), ec);
		if (ec || (!addr.is_v4()))
		{
			return e_libnet_err_cliinvalidip;
		}

		boost::asio::ip::multicast::outbound_interface outbound_interface_option(addr.to_v4());
		m_socket.set_option(outbound_interface_option, ec);
		if (ec)
		{
			return e_libnet_err_clisetmulticastopt;
		}
	}
	break;
	default:
		return e_libnet_err_cliunsupportmulticastopt;
		break;
	}

	return e_libnet_err_noerror;
}
#else


#include <malloc.h>
#include <functional>
#include "udp_session.h"
#include "udp_session_manager.h"
#include "identifier_generator.h"
#include <iostream>

#if (defined _WIN32 || defined _WIN64)
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

udp_session::udp_session(asio::io_context& ioc)
	: m_socket(ioc)
	, m_fnread(NULL)
	, m_start(false)
	, m_autoread(false)
	, m_hasconnected(false)
	, m_userreadbuffer(NULL)
	, m_inreading(false)
	, m_id(generate_identifier())
{
	m_readbuff = new uint8_t[UDP_SESSION_MAX_BUFF_SIZE];
}

udp_session::~udp_session(void)
{
	recycle_identifier(m_id);
	if (m_readbuff != NULL)
	{
		delete[] m_readbuff;
		m_readbuff = NULL;
	}
#ifndef _WIN32
	malloc_trim(0);
#endif
}

int32_t udp_session::init(const int8_t* localip,
	uint16_t localport,
	void* bindaddr,
	read_callback fnread,
	bool autoread)
{
#ifdef LIBNET_NOT_ALLOW_AUTO_ADDRESS
	if ((!localip || (0 == strcmp(reinterpret_cast<const char*>(localip), ""))) && (0 == localport))
	{
		return e_libnet_err_invalidparam;
	}
#endif 

	m_fnread = fnread;
	m_autoread = autoread;

	int32_t ret = bind_address(localip, localport, bindaddr);

	if (e_libnet_err_noerror == ret)
	{
		if (m_autoread)
		{
			start_read();
		}

		m_start = true;
	}

	asio::error_code ec;
	asio::socket_base::receive_buffer_size recv_buffer_size_option(4 * 1024 * 1024);
	m_socket.set_option(recv_buffer_size_option, ec);//设置接收缓冲区

	asio::socket_base::send_buffer_size    SendSize_option(4 * 1024 * 1024); //定义发送缓冲区大小
	m_socket.set_option(SendSize_option, ec); //设置发送缓存区大小

	return ret;
}

int32_t udp_session::connect(const int8_t* remoteip,
	uint16_t remoteport)
{
	if (!remoteip || (0 == strcmp(reinterpret_cast<const char*>(remoteip), "") || (0 == remoteport)))
	{
		return e_libnet_err_invalidparam;
	}
	asio::error_code ec;
	asio::ip::address addr = asio::ip::address::from_string(reinterpret_cast<const char*>(remoteip), ec);
	if (ec)
	{
		close();
		return e_libnet_err_cliinvalidip;
	}

	m_endpoint.address(addr);
	m_endpoint.port(remoteport);

	m_socket.connect(m_endpoint, ec);
	if (ec)
	{
		close();
		return e_libnet_err_cliconnect;
	}
	else
	{
		m_hasconnected = true;
		return e_libnet_err_noerror;
	}
}

int32_t udp_session::recv_from(uint8_t* buffer,
	uint32_t* buffsize,
	void* remoteaddr,
	bool blocked)
{
#ifdef LIBNET_MULTI_THREAD_RECV_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_readmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_readmtx);
#endif
#endif

	if (!buffer || !buffsize || (0 == *buffsize))
	{
		return e_libnet_err_invalidparam;
	}

	if (m_autoread)
	{
		return e_libnet_err_cliautoread;
	}

	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	asio::error_code ec;
	if (blocked)
	{
		size_t readsize = m_socket.receive_from(asio::buffer(buffsize, *buffsize), m_remoteep, 0, ec);
		if (ec || (0 == readsize))
		{
			*buffsize = 0;
			//close();
			udp_session_manager::getInstance().pop_udp_session(get_id());
			return e_libnet_err_clireaddata;
		}
		else
		{
			*buffsize = static_cast<uint32_t>(readsize);
			if (remoteaddr)
			{
				if (m_remoteep.address().is_v4())
				{
					in_addr addr_n;
					inet_pton(AF_INET, m_remoteep.address().to_string().c_str(), &addr_n);

					(static_cast<sockaddr_in*>(remoteaddr))->sin_family = AF_INET;
					(static_cast<sockaddr_in*>(remoteaddr))->sin_addr = addr_n;
					(static_cast<sockaddr_in*>(remoteaddr))->sin_port = htons(m_remoteep.port());
				}
				else
				{
					in6_addr addr_n;
					inet_pton(AF_INET6, m_remoteep.address().to_string().c_str(), &addr_n);

					(static_cast<sockaddr_in6*>(remoteaddr))->sin6_family = AF_INET6;
					(static_cast<sockaddr_in6*>(remoteaddr))->sin6_addr = addr_n;
					(static_cast<sockaddr_in6*>(remoteaddr))->sin6_port = htons(m_remoteep.port());
				}
			}

			return e_libnet_err_noerror;
		}
	}
	else
	{
		if (m_inreading)
		{
			return e_libnet_err_cliprereadnotfinish;
		}

		m_inreading = true;
		m_userreadbuffer = buffer;

		m_socket.async_receive_from(
			asio::buffer(m_userreadbuffer, *buffsize), m_remoteep,
			[this](std::error_code ec, std::size_t length)
			{
				handle_read(ec, length);
			});
		return e_libnet_err_noerror;
	}
}

int32_t udp_session::send_to(uint8_t* data,
	uint32_t datasize,
	void* remoteaddr)
{
#ifdef LIBNET_MULTI_THREAD_SEND_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_writemtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_writemtx);
#endif
#endif

	if (!data || (0 == datasize) || (!m_hasconnected && !remoteaddr))
	{
		return e_libnet_err_invalidparam;
	}

	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	asio::error_code ec;
	if (m_hasconnected)
	{
		m_socket.send(asio::buffer(data, datasize), 0, ec);
#ifndef WIN32
		std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif // !WIN32

	}
	else
	{
		sockaddr_in* addr = static_cast<sockaddr_in*>(remoteaddr);

		m_writeep.address(asio::ip::address::from_string(inet_ntoa(addr->sin_addr)));
		m_writeep.port(ntohs(addr->sin_port));
		m_socket.send_to(asio::buffer(data, datasize), m_writeep, 0, ec);
#ifndef WIN32
		std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif // !WIN32
	}

	if (ec)
	{
		udp_session_manager::getInstance().pop_udp_session(get_id());
		return e_libnet_err_cliwritedata;
	}
	else
	{
		return e_libnet_err_noerror;
	}
}

int32_t	udp_session::close()
{
	m_start = false;
	m_fnread = NULL;
	if (m_socket.is_open())
	{
		asio::error_code ec;
		m_socket.close(ec);
	}

	return e_libnet_err_noerror;
}

int32_t udp_session::bind_address(const int8_t* localip,
	uint16_t localport,
	void* bindaddr)
{
	asio::error_code ec;

	asio::ip::address addr;
	if (localip && (0 != strcmp(reinterpret_cast<const char*>(localip), "")))
	{
		addr = asio::ip::address::from_string(reinterpret_cast<const char*>(localip), ec);
		if (ec)
		{
			return e_libnet_err_srvinvalidip;
		}
	}

	if (!m_socket.is_open())
	{
		m_socket.open(asio::ip::udp::v4(), ec);
		if (ec)
		{
			return e_libnet_err_cliopensock;
		}
	}

	m_endpoint = asio::ip::udp::endpoint(addr, localport);
	m_socket.bind(m_endpoint, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clibind;
	}

	m_endpoint = m_socket.local_endpoint(ec);
	if (ec)
	{
		close();
		return e_libnet_err_clibind;
	}

	//set option
	asio::socket_base::reuse_address reuse_address_option(true);
	m_socket.set_option(reuse_address_option, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	asio::socket_base::send_buffer_size send_buffer_size_option(UDP_SESSION_SEND_BUFF_SIZE);
	m_socket.set_option(send_buffer_size_option, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	asio::socket_base::receive_buffer_size recv_buffer_size_option(UDP_SESSION_RECV_BUFF_SIZE);
	m_socket.set_option(recv_buffer_size_option, ec);
	if (ec)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}


	if (bindaddr)
	{
		if (m_remoteep.address().is_v4())
		{
			in_addr addr_n;
			inet_pton(AF_INET, m_remoteep.address().to_string().c_str(), &addr_n);

			(static_cast<sockaddr_in*>(bindaddr))->sin_family = AF_INET;
			(static_cast<sockaddr_in*>(bindaddr))->sin_addr = addr_n;
			(static_cast<sockaddr_in*>(bindaddr))->sin_port = htons(m_remoteep.port());
		}
		else
		{
			in6_addr addr_n;
			inet_pton(AF_INET6, m_remoteep.address().to_string().c_str(), &addr_n);

			(static_cast<sockaddr_in6*>(bindaddr))->sin6_family = AF_INET6;
			(static_cast<sockaddr_in6*>(bindaddr))->sin6_addr = addr_n;
			(static_cast<sockaddr_in6*>(bindaddr))->sin6_port = htons(m_remoteep.port());
		}
	}

	memset((char*)&tAddrV4, 0x00, sizeof(tAddrV4));
	memset((char*)&tAddrV6, 0x00, sizeof(tAddrV6));

	return e_libnet_err_noerror;
}

void udp_session::start_read()
{
	if (m_socket.is_open())
	{
		
		m_socket.async_receive_from(
			asio::buffer(m_readbuff, UDP_SESSION_MAX_BUFF_SIZE), m_remoteep,
			[this](std::error_code ec, std::size_t length)
			{
				handle_read(ec, length);
			});

	}
}

void udp_session::handle_read(const asio::error_code& ec,
	size_t transize)
{
	if (!m_socket.is_open())
	{
		return;
	}

	if (!ec || ec == asio::error::message_size)
	{
		if (m_start)
		{
			if (!m_hasconnected)
			{
				if (m_remoteep.address().is_v4())
				{
					inet_pton(AF_INET, m_remoteep.address().to_string().c_str(), &addr_nV4);

					tAddrV4.sin_family = AF_INET;
					tAddrV4.sin_addr = addr_nV4;
					tAddrV4.sin_port = htons(m_remoteep.port());

					if (m_fnread)
					{
						m_fnread(INVALID_NETHANDLE, get_id(), m_autoread ? m_readbuff : m_userreadbuffer, static_cast<uint32_t>(transize), &tAddrV4);
					}
				}
				else
				{
					inet_pton(AF_INET6, m_remoteep.address().to_string().c_str(), &addr_nV6);

					tAddrV6.sin6_family = AF_INET6;
					tAddrV6.sin6_addr = addr_nV6;
					tAddrV6.sin6_port = htons(m_remoteep.port());

					if (m_fnread)
					{
						m_fnread(INVALID_NETHANDLE, get_id(), m_autoread ? m_readbuff : m_userreadbuffer, static_cast<uint32_t>(transize), &tAddrV6);
					}
				}
			}
			else
			{
				if (m_fnread)
				{
					m_fnread(INVALID_NETHANDLE, get_id(), m_autoread ? m_readbuff : m_userreadbuffer, static_cast<uint32_t>(transize), NULL);
				}
			}

			if (m_autoread)
			{
				start_read();
			}
			else
			{
				m_userreadbuffer = NULL;
				m_inreading = false;
			}
		}
	}
	else
	{// 有时候会触发 10061 这个错误，造成udp关闭，不需要在这里调用关闭
		//close();
		//udp_session_manager_singleton::get_mutable_instance().pop_udp_session(get_id());
	}
}

int32_t udp_session::multicast(uint8_t option,
	const int8_t* ip,
	uint8_t value)
{
	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	asio::error_code ec;

	switch (option)
	{
	case MULTICAST_OPERATION_JOIN_GROUP:
	{
		if (!ip || (0 == strcmp(reinterpret_cast<const char*>(ip), "")))
		{
			return e_libnet_err_invalidparam;
		}

		asio::ip::address addr = asio::ip::address::from_string(reinterpret_cast<const char*>(ip), ec);
		if (ec)
		{
			return e_libnet_err_cliinvalidip;
		}

		asio::ip::multicast::join_group join_group_option(addr);
		m_socket.set_option(join_group_option, ec);
		if (ec)
		{
			return e_libnet_err_clijoinmulticastgroup;
		}
	}
	break;
	case MULTICAST_OPERATION_LEAVE_GROUP:
	{
		if (!ip || (0 == strcmp(reinterpret_cast<const char*>(ip), "")))
		{
			return e_libnet_err_invalidparam;
		}

		asio::ip::address addr = asio::ip::address::from_string(reinterpret_cast<const char*>(ip), ec);
		if (ec)
		{
			return e_libnet_err_cliinvalidip;
		}

		asio::ip::multicast::leave_group leave_group_option(addr);
		m_socket.set_option(leave_group_option, ec);
		if (ec)
		{
			return e_libnet_err_clileavemulticastgroup;
		}
	}
	break;
	case MULTICAST_OPERATION_LOOPBACK:
	{
		asio::ip::multicast::enable_loopback enable_loopback_option(0 == value);
		m_socket.set_option(enable_loopback_option, ec);
		if (ec)
		{
			return e_libnet_err_clisetmulticastopt;
		}
	}
	break;
	case MULTICAST_OPERATION_HOPS:
	{
		asio::ip::multicast::hops hop_option(value);
		m_socket.set_option(hop_option, ec);
		if (ec)
		{
			return e_libnet_err_clisetmulticastopt;
		}
	}
	break;
	case MULTICAST_OPERATION_OUTBOUND_INTERFACE:
	{
		if (!ip || (0 == strcmp(reinterpret_cast<const char*>(ip), "")))
		{
			return e_libnet_err_invalidparam;
		}

		asio::ip::address addr = asio::ip::address::from_string(reinterpret_cast<const char*>(ip), ec);
		if (ec || (!addr.is_v4()))
		{
			return e_libnet_err_cliinvalidip;
		}

		asio::ip::multicast::outbound_interface outbound_interface_option(addr.to_v4());
		m_socket.set_option(outbound_interface_option, ec);
		if (ec)
		{
			return e_libnet_err_clisetmulticastopt;
		}
	}
	break;
	default:
		return e_libnet_err_cliunsupportmulticastopt;
		break;
	}

	return e_libnet_err_noerror;
}
#endif

