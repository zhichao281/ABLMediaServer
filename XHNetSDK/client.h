#pragma  once
#ifdef USE_BOOST


#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "data_define.h"
#include "libnet.h"
#include "circular_buffer.h"

class client : public boost::enable_shared_from_this<client>
{
public:
	client(boost::asio::io_context& ioc,
		NETHANDLE srvhandle,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	~client();

	std::mutex     m_climtx;
	NETHANDLE get_id();
	NETHANDLE get_server_id() const;
	boost::asio::ip::tcp::socket& socket();

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
	bool m_onwriting;
	circular_buffer m_circularbuff;
	uint8_t* m_currwriteaddr;
	uint32_t m_currwritesize;
};
typedef boost::shared_ptr<client>  client_ptr;

inline boost::asio::ip::tcp::socket& client::socket()
{
	return m_socket;
}

inline NETHANDLE client::get_id()
{
	return m_id;
}

inline NETHANDLE client::get_server_id() const
{
	return m_srvid;
}


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
#include "circular_buffer.h"

class client : public std::enable_shared_from_this<client>
{
public:
	client(asio::io_context& ioc,
		NETHANDLE srvhandle,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	~client();
	auto_lock::al_spin m_climtx;
	NETHANDLE get_id();
	NETHANDLE get_server_id() const;
	asio::ip::tcp::socket& socket();

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
	void handle_write(std::error_code ec, size_t transize);
	void handle_read(std::error_code ec, size_t transize);
	void handle_connect(std::error_code ec);
	void handle_connect_timeout(std::error_code ec);
	bool write_packet();

private:
	NETHANDLE m_srvid;
	NETHANDLE m_id;
	asio::ip::tcp::socket m_socket;
	read_callback m_fnread;
	close_callback m_fnclose;
	connect_callback m_fnconnect;
	std::atomic<bool> m_closeflag;

	//connect
	asio::steady_timer m_timer;

	//read
	std::mutex m_readmtx;                //»¥³âËø	

	const bool m_autoread;
	uint8_t* m_readbuff;
	bool m_inreading;
	uint8_t* m_usrreadbuffer;

	//write
	std::mutex m_writemtx;                //»¥³âËø	
	std::mutex m_autowrmtx;                //»¥³âËø	

	bool m_onwriting;
	circular_buffer m_circularbuff;
	uint8_t* m_currwriteaddr;
	uint32_t m_currwritesize;

};
typedef std::shared_ptr<client>  client_ptr;

inline asio::ip::tcp::socket& client::socket()
{
	return m_socket;
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



