#pragma once


#ifdef USE_BOOST

#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "libnet.h"
#include "libnet_error.h"
#include "data_define.h"
#include "circular_buffer.h"

class udp_session : public boost::enable_shared_from_this<udp_session>
{
public:
	udp_session(boost::asio::io_context& ioc);
	~udp_session(void);

	NETHANDLE get_id();
	in_addr      addr_nV4;
	in6_addr     addr_nV6;
	sockaddr_in  tAddrV4;
	sockaddr_in6 tAddrV6;

	int32_t init(const int8_t* localip,
		uint16_t localport,
		void* bindaddr,
		read_callback fnread,
		bool autoread);
	int32_t connect(const int8_t* remoteip,
		uint16_t remoteport);
	int32_t recv_from(uint8_t* buffer,
		uint32_t* buffsize,
		void* remoteaddr,
		bool blocked);
	int32_t send_to(uint8_t* data,
		uint32_t datasize,
		void* remoteaddr);
	int32_t	close();
	int32_t multicast(uint8_t option,
		const int8_t* ip,
		uint8_t value);

private:
	int32_t bind_address(const int8_t* localip,
		uint16_t localport,
		void* bindaddr);
	void start_read();
	void handle_read(const boost::system::error_code& ec,
		size_t transize);

private:
	enum MULTICAST_OPERATION
	{
		MULTICAST_OPERATION_JOIN_GROUP = 0,
		MULTICAST_OPERATION_LEAVE_GROUP,
		MULTICAST_OPERATION_LOOPBACK,
		MULTICAST_OPERATION_HOPS,
		MULTICAST_OPERATION_OUTBOUND_INTERFACE
	};

private:
	boost::atomic<bool> m_start;
	bool m_autoread;
	bool m_hasconnected;
	boost::asio::ip::udp::socket m_socket;
	boost::asio::ip::udp::endpoint m_endpoint;
	boost::asio::ip::udp::endpoint m_remoteep;
	boost::asio::ip::udp::endpoint m_writeep;
	read_callback m_fnread;
	NETHANDLE m_id;


#ifdef LIBNET_MULTI_THREAD_SEND_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_writemtx;
#else
	auto_lock::al_spin m_writemtx;
#endif
#endif

#ifdef LIBNET_MULTI_THREAD_RECV_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_readmtx;
#else
	auto_lock::al_spin m_readmtx;
#endif
#endif


	uint8_t* m_readbuff;

	bool m_inreading;
	uint8_t* m_userreadbuffer;
};

typedef boost::shared_ptr<udp_session> udp_session_ptr;

inline NETHANDLE udp_session::get_id()
{
	return m_id;
}

#else

#include <memory>
#include <atomic>
#include <asio.hpp>
#include "libnet.h"
#include "libnet_error.h"
#include "data_define.h"
#include "circular_buffer.h"

class udp_session : public std::enable_shared_from_this<udp_session>
{
public:
	udp_session(asio::io_context& ioc);
	~udp_session(void);

	NETHANDLE get_id();
	in_addr      addr_nV4;
	in6_addr     addr_nV6;
	sockaddr_in  tAddrV4;
	sockaddr_in6 tAddrV6;

	int32_t init(const int8_t* localip,
		uint16_t localport,
		void* bindaddr,
		read_callback fnread,
		bool autoread);
	int32_t connect(const int8_t* remoteip,
		uint16_t remoteport);
	int32_t recv_from(uint8_t* buffer,
		uint32_t* buffsize,
		void* remoteaddr,
		bool blocked);
	int32_t send_to(uint8_t* data,
		uint32_t datasize,
		void* remoteaddr);
	int32_t	close();
	int32_t multicast(uint8_t option,
		const int8_t* ip,
		uint8_t value);

private:
	int32_t bind_address(const int8_t* localip,
		uint16_t localport,
		void* bindaddr);
	void start_read();
	void handle_read(const asio::error_code& ec,
		size_t transize);

private:
	enum MULTICAST_OPERATION
	{
		MULTICAST_OPERATION_JOIN_GROUP = 0,
		MULTICAST_OPERATION_LEAVE_GROUP,
		MULTICAST_OPERATION_LOOPBACK,
		MULTICAST_OPERATION_HOPS,
		MULTICAST_OPERATION_OUTBOUND_INTERFACE
	};

private:
	std::atomic<bool> m_start;
	bool m_autoread;
	bool m_hasconnected;
	asio::ip::udp::socket m_socket;
	asio::ip::udp::endpoint m_endpoint;
	asio::ip::udp::endpoint m_remoteep;
	asio::ip::udp::endpoint m_writeep;
	read_callback m_fnread;
	NETHANDLE m_id;


#ifdef LIBNET_MULTI_THREAD_SEND_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_writemtx;
#else
	auto_lock::al_spin m_writemtx;
#endif
#endif

#ifdef LIBNET_MULTI_THREAD_RECV_UDP
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_readmtx;
#else
	auto_lock::al_spin m_readmtx;
#endif
#endif


	uint8_t* m_readbuff;

	bool m_inreading;
	uint8_t* m_userreadbuffer;
};

typedef std::shared_ptr<udp_session> udp_session_ptr;

inline NETHANDLE udp_session::get_id()
{
	return m_id;
}

#endif




