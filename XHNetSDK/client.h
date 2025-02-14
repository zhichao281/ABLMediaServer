#pragma  once
#ifdef USE_BOOST
#ifndef _CLIENT_H_
#define _CLIENT_H_ 

#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "data_define.h"
#include "libnet.h"
#include "circular_buffer.h"
#include "AsyncBuffer.h"
#include "MediaFifo.h"
#include <boost/asio/ssl.hpp>

#include "auto_lock.h"

//客户端类型
enum ClientType
{
	clientType_Connect = 1,  //主动向外连接服务器产生的client 
	clientType_Accept = 2   //外部连接本地端口产生的client 
};

class client : public boost::enable_shared_from_this<client>
{
public:
	client(boost::asio::io_context& ioc,
		boost::asio::ssl::context& context,
		NETHANDLE srvhandle,
		read_callback fnread,
		close_callback fnclose,
		bool autoread,
		bool bSSLFlag,
		ClientType nCLientType,
		accept_callback  fnaccept);
	~client();

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif

	boost::atomic_bool m_connectflag;
	int               m_nClientType;
	accept_callback   m_fnaccept;
	sockaddr_in       tAddr4;
	sockaddr_in6      tAddr6;

	boost::asio::ip::tcp::resolver resolver;
	boost::atomic_bool m_bSSLFlag;
	void handle_handshake(const boost::system::error_code& error);

	auto_lock::al_spin m_climtx;
	NETHANDLE get_id();
	NETHANDLE get_server_id() const;
	boost::asio::ip::tcp::socket& socket();
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::lowest_layer_type& socket_();

	int32_t run();
	int32_t connect(int8_t* remoteip,
		uint16_t remoteport,
		int8_t* localip,
		uint16_t localport,
		bool blocked,
		connect_callback fnconnect,
		uint32_t timeout);
	int32_t write(uint8_t* data,
		uint32_t datasize,
		bool blocked);
	int32_t read(uint8_t* buffer,
		uint32_t* buffsize,
		bool blocked,
		bool certain);
	void close();

private:
	void handle_write(const boost::system::error_code& ec, size_t transize);
	void handle_read(const boost::system::error_code& ec, size_t transize);
	void handle_connect(const boost::system::error_code& ec);
	void handle_connect_timeout(const boost::system::error_code& ec);
	bool write_packet();

private:
	NETHANDLE m_srvid;
	NETHANDLE m_id;
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket_;
	read_callback m_fnread;
	close_callback m_fnclose;
	connect_callback m_fnconnect;
	boost::atomic_bool m_closeflag;

	//connect
	boost::asio::deadline_timer m_timer;

	//read
#ifdef LIBNET_MULTI_THREAD_RECV
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_readmtx;
#else
	auto_lock::al_spin m_readmtx;
#endif
#endif
	const bool m_autoread;
	uint8_t* m_readbuff;
	bool m_inreading;
	uint8_t* m_usrreadbuffer;

	//write
#ifdef LIBNET_MULTI_THREAD_SEND
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_writemtx;
#else
	auto_lock::al_spin m_writemtx;
#endif
#endif

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_autowrmtx;
#else
	auto_lock::al_spin m_autowrmtx;
#endif
	boost::atomic_bool m_onwriting;
	CMediaFifo m_circularbuff;
	uint8_t* m_currwriteaddr;
	int       m_currwritesize;
};
typedef boost::shared_ptr<client>  client_ptr;

inline boost::asio::ip::tcp::socket& client::socket()
{
	return m_socket;
}

inline boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::lowest_layer_type& client::socket_()
{
	return m_socket_.lowest_layer();
}

inline NETHANDLE client::get_id()
{
	return m_id;
}

inline NETHANDLE client::get_server_id() const
{
	return m_srvid;
}

#endif

#else


#ifdef _WIN32
#define _WIN32_WINNT 0x0A00 
#endif

#include <memory>
#include <atomic>
#include <asio.hpp>
# include "asio/deadline_timer.hpp"
#include "data_define.h"
#include "libnet.h"
#include "AsyncBuffer.h"
#include "MediaFifo.h"
#include "circular_buffer.h"
#include <asio/ssl.hpp>

//客户端类型
enum ClientType
{
	clientType_Connect = 1,  //主动向外连接服务器产生的client 
	clientType_Accept = 2   //外部连接本地端口产生的client 
};


class client : public std::enable_shared_from_this<client>
{
public:
	client(asio::io_context& ioc,
		asio::ssl::context& context,
		NETHANDLE srvhandle,
		read_callback fnread,
		close_callback fnclose,
		bool autoread,
		bool bSSLFlag,
		ClientType nCLientType,
		accept_callback  fnaccept);

	~client();

	std::atomic<bool> m_connectflag;
	int               m_nClientType;
	accept_callback   m_fnaccept;
	sockaddr_in       tAddr4;
	sockaddr_in6      tAddr6;

	asio::ip::tcp::resolver resolver;
	std::atomic<bool> m_bSSLFlag;
	void handle_handshake(const std::error_code& error);

	auto_lock::al_spin m_climtx;
	NETHANDLE get_id();
	NETHANDLE get_server_id() const;
	asio::ip::tcp::socket& socket();
	asio::ssl::stream<asio::ip::tcp::socket>::lowest_layer_type& socket_();

	int32_t run();
	int32_t connect(int8_t* remoteip,
		uint16_t remoteport,
		int8_t* localip,
		uint16_t localport,
		bool blocked,
		connect_callback fnconnect,
		uint32_t timeout);
	int32_t write(uint8_t* data,
		uint32_t datasize,
		bool blocked);
	int32_t read(uint8_t* buffer,
		uint32_t* buffsize,
		bool blocked,
		bool certain);
	void close();

private:
	void handle_write(const std::error_code& ec, size_t transize);
	void handle_read(const std::error_code& ec, size_t transize);
	void handle_connect(const std::error_code& ec);
	void handle_connect_timeout(const std::error_code& ec);
	bool write_packet();

private:
	NETHANDLE m_srvid;
	NETHANDLE m_id;
	asio::ip::tcp::socket m_socket;
	asio::ssl::stream<asio::ip::tcp::socket> m_socket_;
	read_callback m_fnread;
	close_callback m_fnclose;
	connect_callback m_fnconnect;
	std::atomic<bool> m_closeflag;

	//connect
	asio::steady_timer m_timer;

	//read
	std::mutex m_readmtx;                //互斥锁	

	const bool m_autoread;
	uint8_t* m_readbuff;
	bool m_inreading;
	uint8_t* m_usrreadbuffer;

	//write
	std::mutex m_writemtx;                //互斥锁	
	std::mutex m_autowrmtx;                //互斥锁	

	std::atomic<bool> m_onwriting;
	CMediaFifo m_circularbuff;
	uint8_t* m_currwriteaddr;
	int       m_currwritesize;

};
typedef std::shared_ptr<client>  client_ptr;

inline asio::ip::tcp::socket& client::socket()
{
	return m_socket;
}


inline asio::ssl::stream<asio::ip::tcp::socket>::lowest_layer_type& client::socket_()
{
	return m_socket_.lowest_layer();
}


inline NETHANDLE client::get_id()
{
	return m_id;
}

inline NETHANDLE client::get_server_id() const
{
	return m_srvid;
}


#endif



