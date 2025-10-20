#ifndef _CLIENT_H_
#define _CLIENT_H_ 

#ifdef USE_BOOST
#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#else
#include <atomic>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


#endif

#include "data_define.h"
#include "XHNetSDK.h"
#include "MediaFifo.h"
#include "auto_lock.h"

#ifndef SOCKET
#define  SOCKET   int 
#endif

//客户端类型
enum ClientType
{
	clientType_Connect = 1,  //主动向外连接服务器产生的client 
	clientType_Accept  = 2   //外部连接本地端口产生的client 
};

#ifdef USE_BOOST
class client : public boost::enable_shared_from_this<client>
#else
class client : public std::enable_shared_from_this<client>
#endif
{
public:
	client(NETHANDLE srvhandle,
		read_callback fnread,
		close_callback fnclose,
		bool autoread,
		bool bSSLFlag,
		ClientType nCLientType,
		accept_callback  fnaccept);
		
	~client();
	void               SetConnect(bool bFlag);
	bool               GetConnect();

	CMediaFifo         m_circularbuff;
	unsigned char*     pPopData;
	int                nPopLength;
	int                nSendPos;
	int                nSendLength;


	std::mutex         m_mutex;
 
	uint64_t           nCreateTime;

	int                m_nClientType;
	accept_callback    m_fnaccept;
	sockaddr_in        tAddr4;
	sockaddr_in6       tAddr6;
 
	std::mutex     m_climtx;
	NETHANDLE      get_id();
	NETHANDLE      get_server_id() const;
	SOCKET         m_Socket;
	NETHANDLE      m_id;

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

	read_callback m_fnread = NULL ;
	close_callback m_fnclose = NULL;
	connect_callback m_fnconnect = NULL;


private:
	NETHANDLE m_srvid;


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

	uint8_t*  m_currwriteaddr;
	int       m_currwritesize;



public:
#ifdef USE_BOOST
	boost::atomic<bool> m_connectflag;
	boost::atomic<bool> m_closeflag;
	boost::atomic<bool> m_onwriting;
	boost::atomic_uint64_t   nRecvThreadOrder;
	boost::atomic_uint64_t   nSendThreadOrder;
#else
	std::atomic_bool m_connectflag;
	std::atomic_bool m_closeflag;
	std::atomic_bool m_onwriting;
	std::atomic<uint64_t>    nRecvThreadOrder;
	std::atomic<uint64_t>    nSendThreadOrder;
#endif


};

#ifdef USE_BOOST
typedef boost::shared_ptr<client>  client_ptr;
#else
typedef std::shared_ptr<client>  client_ptr;
#endif

inline NETHANDLE client::get_id()
{
	return m_id;
}

inline NETHANDLE client::get_server_id() const
{
	return m_srvid;
}

#endif