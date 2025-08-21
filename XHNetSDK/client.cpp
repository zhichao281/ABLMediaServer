#ifdef USE_BOOST
#include <boost/bind.hpp>
#include "client_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"
#include <malloc.h>

client::client(boost::asio::io_context& ioc,
	boost::asio::ssl::context& context,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread,
	bool bSSLFlag,
	ClientType nCLientType,
	accept_callback  fnaccept)
	: m_srvid(srvid)
	, m_id(generate_identifier())
	, m_socket(ioc)
	, m_socket_(ioc, context)
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_fnconnect(NULL)
	, m_closeflag(false)
	, m_timer(ioc)
	, m_autoread(autoread)
	, m_inreading(false)
	, m_usrreadbuffer(NULL)
	, m_onwriting(false)
	, m_currwriteaddr(NULL)
	, m_currwritesize(0)
	, resolver(ioc)
	, m_bSSLFlag(bSSLFlag)
	, m_nClientType(nCLientType)
	, m_fnaccept(fnaccept)
{
	m_onwriting.exchange(false);
	m_closeflag.exchange(false);
	m_connectflag.exchange(false);
	m_readbuff = new uint8_t[CLIENT_MAX_RECV_BUFF_SIZE];
}

client::~client(void)
{
	m_connectflag.exchange(false);
	m_closeflag.exchange(true);
	recycle_identifier(m_id);
	if (m_readbuff != NULL)
	{
		delete[] m_readbuff;
		m_readbuff = NULL;
	}

	//�ȵ��첽�������
	int nWaitCount = 0;
	while (m_onwriting.load())
	{//�ȴ�10�� 
		nWaitCount++;
		usleep(100000);
		if (nWaitCount >= 10 * 10)
			break;
	}
	m_circularbuff.FreeFifo();
#ifndef _WIN32
	malloc_trim(0);
#endif
}

void client::handle_handshake(const boost::system::error_code& error)
{
	if (!error)
	{//���ֳɹ��ٶ�ȡ
	   //struct timeval tv;
	   //tv.tv_sec = 5;
	   //tv.tv_usec = 0;

		m_connectflag.exchange(true);
		m_timer.cancel();
		if (m_nClientType == clientType_Connect)
		{//������������ 
			if (m_fnconnect)//���ֳɹ����ٻص����ӳɹ��������д�����쳣
				m_fnconnect(get_id(), 1, m_socket_.lowest_layer().local_endpoint().port());
		}
		else
		{//�����ⲿ����
			if (m_fnaccept)
				m_fnaccept(get_server_id(), get_id(), &tAddr4);
		}

		//int nRet1 = setsockopt(m_socket_.next_layer().native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(timeval)); //���÷��ͳ�ʱ
		//int nRet2 = setsockopt(m_socket_.next_layer().native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(timeval)); //���ý��ճ�ʱ 
		//printf("handle_handshake(),setsockopt nRet1 = %d ,nRet2 = %d \r\n", nRet1, nRet2);

		if (m_bSSLFlag.load() == true)
		{//SSL ��ȡ
			m_socket_.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
	else
	{
		printf("network information : %s \r\n ", error.message().c_str());
		m_timer.cancel();
		close();
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnclose)
				m_fnclose(get_server_id(), get_id());
		}
	}
}

int32_t client::run()
{
	m_connectflag.exchange(true);
	if (!m_autoread)
	{
		return e_libnet_err_climanualread;
	}

	if (m_bSSLFlag.load() == true)
	{//SSL ��ȡ
		m_socket_.async_handshake(boost::asio::ssl::stream_base::server,
			boost::bind(&client::handle_handshake, shared_from_this(),
				boost::asio::placeholders::error));
	}
	else
	{//��ͨ��ȡ
		int ul = 1;
		int nRet3 = ioctl(m_socket.native_handle(), FIONBIO, &ul); //����Ϊ������ģʽ 
		//printf("run() , ioctl nRet3 = %d \r\n",nRet3);
		m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
			boost::bind(&client::handle_read,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	return e_libnet_err_noerror;
}

int32_t client::connect(int8_t* remoteip,
	uint16_t remoteport,
	int8_t* localip,
	uint16_t localport,
	bool blocked,
	connect_callback fnconnect,
	uint32_t timeout)
{
	boost::system::error_code err;
	boost::asio::ip::address remoteaddr = boost::asio::ip::address::from_string(reinterpret_cast<char*>(remoteip), err);
	if (err)
	{
		return e_libnet_err_cliinvalidip;
	}

	boost::asio::ip::tcp::endpoint srvep(remoteaddr, remoteport);
	//���÷��ͣ����ճ�ʱ
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	if (m_bSSLFlag.load() == false)
	{//��SSL 
	//open socket
		if (!m_socket.is_open())
		{
			m_socket.open(remoteaddr.is_v4() ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(), err);
			if (err)
			{
				return e_libnet_err_cliopensock;
			}
		}

		//set callback function
		m_fnconnect = fnconnect;

		//bind local address
		if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))) || (localport > 0))
		{
			boost::asio::ip::address localaddr;
			if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))))
			{
				localaddr = boost::asio::ip::address::from_string(reinterpret_cast<char*>(localip), err);
				if (err)
				{
					close();
					return e_libnet_err_cliinvalidip;
				}
			}

			boost::asio::ip::tcp::endpoint localep(localaddr, localport);
			m_socket.bind(localep, err);
			if (err)
			{
				close();
				return e_libnet_err_clibind;
			}
		}

		//set option
		boost::asio::socket_base::reuse_address reuse_address_option(true);
		m_socket.set_option(reuse_address_option, err);
		if (err)
		{
			close();
			return e_libnet_err_clisetsockopt;
		}

		boost::asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
		m_socket.set_option(send_buffer_size_option, err);
		if (err)
		{
			close();
			return e_libnet_err_clisetsockopt;
		}

		boost::asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
		m_socket.set_option(recv_buffer_size_option, err);
		if (err)
		{
			close();
			return e_libnet_err_clisetsockopt;
		}

		int nRet1 = setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(timeval)); //���÷��ͳ�ʱ
		int nRet2 = setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(timeval)); //���ý��ճ�ʱ
		printf("connect() , setsockopt nRet1 = %d , nRet2 = %d \r\n", nRet1, nRet2);

		int ul = 1;
		int nRet3 = ioctl(m_socket.native_handle(), FIONBIO, &ul); //����Ϊ������ģʽ 
			//printf("connect() , ioctlsocket nRet3 = %d \r\n",nRet3);

		//���ùرղ�����
		boost::system::error_code ec;
		boost::asio::ip::tcp::no_delay no_delay_option(true);
		m_socket.set_option(no_delay_option, ec);

		//connect timeout
		if (timeout > 0)
		{
			m_timer.expires_from_now(boost::posix_time::milliseconds(3000));
			m_timer.async_wait(boost::bind(&client::handle_connect_timeout, shared_from_this(), boost::asio::placeholders::error));
		}

		//connect
		if (blocked)
		{
			m_socket.connect(srvep, err);
			m_timer.cancel();
			if (!err)
			{
				m_connectflag.exchange(true);
				run();
				return e_libnet_err_noerror;
			}
			else
			{
				close();
				return e_libnet_err_cliconnect;
			}
		}
		else //sync connect
		{
			m_socket.async_connect(srvep, boost::bind(&client::handle_connect, shared_from_this(), boost::asio::placeholders::error));
			return e_libnet_err_noerror;
		}
	}
	else
	{//SSL 
		char szPort[256] = { 0 };
		sprintf(szPort, "%d", remoteport);
		boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve((char*)remoteip, szPort);

		//set callback function
		m_fnconnect = fnconnect;

		//connect timeout
		if (timeout > 0)
		{
			m_timer.expires_from_now(boost::posix_time::milliseconds(3000));
			m_timer.async_wait(boost::bind(&client::handle_connect_timeout, shared_from_this(), boost::asio::placeholders::error));
		}
		boost::asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
		m_socket_.lowest_layer().set_option(send_buffer_size_option, err);

		boost::asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
		m_socket_.lowest_layer().set_option(recv_buffer_size_option, err);

		boost::asio::async_connect(m_socket_.lowest_layer(), endpoints,
			boost::bind(&client::handle_connect, shared_from_this(),
				boost::asio::placeholders::error));

		return e_libnet_err_noerror;
	}
}

void client::handle_write(const boost::system::error_code& ec, size_t transize)
{
	if (ec)
	{
		close();
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	//������Ϻ�ִ������������Ӧ�Ѿ����͵�buffer 
	if (transize > 0)
		m_circularbuff.pop_front();

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif
	m_onwriting.exchange(write_packet());
}

void client::handle_read(const boost::system::error_code& ec, size_t transize)
{
	if (ec)
	{
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	if (m_autoread)
	{
		if (!m_closeflag)
		{
			if (m_fnread)
			{
				m_fnread(get_server_id(), get_id(), m_readbuff, static_cast<uint32_t>(transize), NULL);
			}
			else
				return; //���ٶ�ȡ 
		}
		else
		{
			return; //���ٶ�ȡ 
		}

		if (m_bSSLFlag.load() == true)
		{//SSL ��ȡ 
			if (m_socket_.lowest_layer().is_open())
			{
				m_socket_.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
					boost::bind(&client::handle_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else
			{//�ر� 
				if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
				{
					if (m_fnclose)
					{
						m_fnclose(get_server_id(), get_id());
					}
				}
			}
		}
		else
		{//��ͨSOCKETd ��ȡ 
			if (m_socket.is_open())
			{
				m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
					boost::bind(&client::handle_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else
			{//�ر� 
				if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
				{
					if (m_fnclose)
					{
						m_fnclose(get_server_id(), get_id());
					}
				}
			}
		}
	}
	else
	{
		if (m_fnread)
		{
			m_fnread(get_server_id(), get_id(), m_usrreadbuffer, static_cast<uint32_t>(transize), NULL);
		}

		m_usrreadbuffer = NULL;
		m_inreading = false;
	}
}

void client::handle_connect(const boost::system::error_code& ec)
{
	if (ec)
	{
		printf("network information : %s \r\n ", ec.message().c_str());
		m_timer.cancel();
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnconnect)
			{
				m_fnconnect(get_id(), 0, 0);
			}
		}
	}
	else
	{
		if (m_bSSLFlag.load() == true)
		{//���ӳɹ���ִ��SSL���� 
			m_socket_.async_handshake(boost::asio::ssl::stream_base::client,
				boost::bind(&client::handle_handshake, shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{//��SSL������
			m_connectflag.exchange(true);
			m_timer.cancel();
			if (m_fnconnect)
			{
				m_fnconnect(get_id(), 1, htons(m_socket.local_endpoint().port()));
			}
			run();
		}
	}
}

int32_t client::read(uint8_t* buffer,
	uint32_t* buffsize,
	bool blocked,
	bool certain)
{
#ifdef LIBNET_MULTI_THREAD_RECV
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_readmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_readmtx);
#endif
#endif
	if (m_closeflag)
		return e_libnet_err_clisocknotopen;

	if (!buffer || !buffsize || (0 == *buffsize))
	{
		return  e_libnet_err_invalidparam;
	}

	if (m_autoread)
	{
		return e_libnet_err_cliautoread;
	}

	uint32_t readsize = 0;
	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		boost::system::error_code err;
		if (certain)
		{
			readsize = static_cast<uint32_t>(boost::asio::read(m_socket, boost::asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				client_manager_singleton::get_mutable_instance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				return e_libnet_err_noerror;
			}
		}
		else
		{
			readsize = static_cast<uint32_t>(m_socket.read_some(boost::asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
				{
					if (m_fnclose)
						m_fnclose(get_server_id(), get_id());
				}
				return e_libnet_err_clireaddata;
			}
			else
			{
				*buffsize = readsize;
				return e_libnet_err_noerror;
			}
		}
	}
	else
	{
		if (m_inreading)
		{
			return e_libnet_err_cliprereadnotfinish;
		}

		m_inreading = true;
		m_usrreadbuffer = buffer;

		if (certain)
		{
			boost::asio::async_read(m_socket, boost::asio::buffer(m_usrreadbuffer, *buffsize),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_usrreadbuffer, *buffsize),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}

		return e_libnet_err_noerror;
	}
}

int32_t client::write(uint8_t* data,
	uint32_t datasize,
	bool blocked)
{
#ifdef LIBNET_MULTI_THREAD_SEND
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_writemtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_writemtx);
#endif
#endif
	if (m_closeflag.load() || !m_connectflag.load())
		return e_libnet_err_clisocknotopen;

	int32_t ret = e_libnet_err_noerror;
	int32_t datasize2 = datasize;

	if (!data || (0 == datasize))
	{
		return e_libnet_err_invalidparam;
	}

	if (m_bSSLFlag.load() == true)
	{//SSL 
		if (!m_socket_.lowest_layer().is_open())
		{
			return e_libnet_err_clisocknotopen;
		}
	}
	else
	{//��ͨSOCKET
		if (!m_socket.is_open())
		{
			return e_libnet_err_clisocknotopen;
		}
	}

	if (blocked)
	{
		boost::system::error_code ec;
		unsigned long nSendPos = 0, nSendRet = 0;

		while (datasize2 > 0)
		{//�ĳ�ѭ������
			if (m_bSSLFlag.load() == true)
				nSendRet = boost::asio::write(m_socket_, boost::asio::buffer(data + nSendPos, datasize2), ec);
			else
				nSendRet = boost::asio::write(m_socket, boost::asio::buffer(data + nSendPos, datasize2), ec);

			if (!ec)
			{//����û�г���
				if (nSendRet > 0)
				{
					nSendPos += nSendRet;
					datasize2 -= nSendRet;
				}
			}
			else//���ͳ�����������ѭ�����������ѭ��
				break;
		}
		if (!ec)
		{
			return e_libnet_err_noerror;
		}
		else
		{
			close();

			if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
			{
				if (m_fnclose)
					m_fnclose(get_server_id(), get_id());
			}
			return e_libnet_err_cliwritedata;
		}
	}
	else
	{//�첽����
		//�����첽���ͻ�����
		if (m_circularbuff.pMediaBuffer == NULL)
			m_circularbuff.InitFifo(CLIENT_MAX_SEND_BUFF_SIZE);

		//��д�뻺���� 
		if (m_circularbuff.push(data, datasize) == false)
		{
			m_circularbuff.Reset();
			m_circularbuff.push(data, datasize);
			m_onwriting.exchange(false);
		}

		//���û�����ڷ��ͣ�ֱ�ӽ��з��� ��������д����ɵĻص������ٽ��з��� 
		if (!m_onwriting.load())
		{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
			auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
			auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif			
			m_onwriting.exchange(write_packet());
		}
	}

	return ret;
}

bool client::write_packet()
{
	if (m_closeflag.load())
	{
		m_onwriting.exchange(false);
		m_circularbuff.Reset();
		return false;
	}

	m_currwriteaddr = m_circularbuff.pop(&m_currwritesize);
	if (m_currwritesize > 0)
	{
		if (m_bSSLFlag.load() == true)
		{//SSL 
			boost::asio::async_write(m_socket_, boost::asio::buffer(m_currwriteaddr, m_currwritesize),
				boost::bind(&client::handle_write,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{//��SSL
			boost::asio::async_write(m_socket, boost::asio::buffer(m_currwriteaddr, m_currwritesize),
				boost::bind(&client::handle_write,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}

		return true;
	}

	return false;
}

void client::close()
{
	if (m_closeflag.load() == false)
	{
		m_closeflag.exchange(true);
		m_onwriting.exchange(false);
		m_fnread = NULL;

		if (m_bSSLFlag.load() == true)
		{//SSL ���ӹر�
			if (m_socket_.lowest_layer().is_open())
			{
				boost::system::error_code ec;
				m_socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				m_socket_.lowest_layer().close(ec);
				m_socket_.lowest_layer().release(ec);
			}
		}
		else
		{//��ͨ���ӹر�
			if (m_socket.is_open())
			{
				boost::system::error_code ec;
				m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				m_socket.close(ec);
				m_socket.release(ec);
			}
		}

	}
}

void client::handle_connect_timeout(const boost::system::error_code& ec)
{
	if (m_connectflag.load() == false)
	{
		client_manager_singleton::get_mutable_instance().pop_client(get_id());

		if (m_fnconnect)
		{//֪ͨ����ʧ��
			m_fnconnect(get_id(), 0, 0);
		}
	}
}
#else

#include "client_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"
#include <malloc.h>
#include <iostream>
#include <cstdio>
#include <thread>
client::client(asio::io_context& ioc,
	asio::ssl::context& context,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread,
	bool bSSLFlag,
	ClientType nCLientType,
	accept_callback  fnaccept)
	: m_srvid(srvid)
	, m_id(generate_identifier())
	, m_socket(ioc)
	, m_socket_(ioc, context)
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_fnconnect(NULL)
	, m_closeflag(false)
	, m_timer(ioc)
	, m_autoread(autoread)
	, m_inreading(false)
	, m_usrreadbuffer(NULL)
	, m_onwriting(false)
	, m_currwriteaddr(NULL)
	, m_currwritesize(0)
	, resolver(ioc)
	, m_bSSLFlag(bSSLFlag)
	, m_nClientType(nCLientType)
	, m_fnaccept(fnaccept)
{
	m_onwriting.exchange(false);
	m_closeflag.exchange(false);
	m_connectflag.exchange(false);
	m_readbuff = new uint8_t[CLIENT_MAX_RECV_BUFF_SIZE];
}

client::~client(void)
{
	m_connectflag.exchange(false);
	m_closeflag.exchange(true);
	recycle_identifier(m_id);
	if (m_readbuff != NULL)
	{
		delete[] m_readbuff;
		m_readbuff = NULL;
	}

	//�ȵ��첽�������
	int nWaitCount = 0;
	while (m_onwriting.load())
	{//�ȴ�10�� 
		nWaitCount++;
		std::this_thread::sleep_for(std::chrono::milliseconds(1 * 100));
		if (nWaitCount >= 10 * 10)
			break;
	}
	m_circularbuff.FreeFifo();

#ifndef _WIN32
	malloc_trim(0);
#endif
	
}


void client::handle_handshake(const std::error_code& error)
{
	if (!error)
	{//���ֳɹ��ٶ�ȡ
	   //struct timeval tv;
	   //tv.tv_sec = 5;
	   //tv.tv_usec = 0;

		m_connectflag.exchange(true);
		m_timer.cancel();
		if (m_nClientType == clientType_Connect)
		{//������������ 
			if (m_fnconnect)//���ֳɹ����ٻص����ӳɹ��������д�����쳣
				m_fnconnect(get_id(), 1, m_socket_.lowest_layer().local_endpoint().port());
		}
		else
		{//�����ⲿ����
			if (m_fnaccept)
				m_fnaccept(get_server_id(), get_id(), &tAddr4);
		}

		//int nRet1 = setsockopt(m_socket_.next_layer().native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(timeval)); //���÷��ͳ�ʱ
		//int nRet2 = setsockopt(m_socket_.next_layer().native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(timeval)); //���ý��ճ�ʱ 
		//printf("handle_handshake(),setsockopt nRet1 = %d ,nRet2 = %d \r\n", nRet1, nRet2);

		if (m_bSSLFlag.load() == true)
		{//SSL ��ȡ
			m_socket_.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
				std::bind(&client::handle_read,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2));
		

		}
	}
	else
	{
		printf("network information : %s \r\n ", error.message().c_str());
		m_timer.cancel();
		close();
		if (client_manager::getInstance().pop_client(get_id()))
		{
			if (m_fnclose)
				m_fnclose(get_server_id(), get_id());
		}
	}
}

int32_t client::run()
{
	m_connectflag.exchange(true);
	if (!m_autoread)
	{
		return e_libnet_err_climanualread;
	}

	if (m_bSSLFlag.load() == true)
	{//SSL ��ȡ
		m_socket_.async_handshake(asio::ssl::stream_base::server,
			std::bind(&client::handle_handshake, shared_from_this(),
				std::placeholders::_1));
	}
	else
	{//��ͨ��ȡ
		
#ifndef _WIN32
		int ul = 1;
		int nRet3 = ioctl(m_socket.native_handle(), FIONBIO, &ul); //����Ϊ������ģʽ 
	
#else
		u_long ul = 1; // 1 ��ʾ������ģʽ
		int nRet3 = ioctlsocket(m_socket.native_handle(), FIONBIO, &ul); // ����Ϊ������ģʽ
	
#endif
	
		//printf("run() , ioctl nRet3 = %d \r\n",nRet3);
		m_socket.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
			std::bind(&client::handle_read,
				shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}
	return e_libnet_err_noerror;
}




int32_t client::connect(int8_t* remoteip,
	uint16_t remoteport,
	int8_t* localip,
	uint16_t localport,
	bool blocked,
	connect_callback fnconnect,
	uint32_t timeout)
{
	std::error_code err;
	asio::ip::address remoteaddr = asio::ip::address::from_string(reinterpret_cast<char*>(remoteip), err);
	if (err)
	{
		return e_libnet_err_cliinvalidip;
	}

	asio::ip::tcp::endpoint srvep(remoteaddr, remoteport);
	//���÷��ͣ����ճ�ʱ
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	if (m_bSSLFlag.load() == false)
	{//��SSL 
	//open socket
		if (!m_socket.is_open())
		{
			m_socket.open(remoteaddr.is_v4() ? asio::ip::tcp::v4() : asio::ip::tcp::v6(), err);
			if (err)
			{
				return e_libnet_err_cliopensock;
			}
		}

		//set callback function
		m_fnconnect = fnconnect;

		//bind local address
		if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))) || (localport > 0))
		{
			asio::ip::address localaddr;
			if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))))
			{
				localaddr = asio::ip::address::from_string(reinterpret_cast<char*>(localip), err);
				if (err)
				{
					close();
					return e_libnet_err_cliinvalidip;
				}
			}

			asio::ip::tcp::endpoint localep(localaddr, localport);
			m_socket.bind(localep, err);
			if (err)
			{
				close();
				return e_libnet_err_clibind;
			}
		}

		//set option
		asio::socket_base::reuse_address reuse_address_option(true);
		m_socket.set_option(reuse_address_option, err);
		if (err)
		{
			close();
			return e_libnet_err_clisetsockopt;
		}

		asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
		m_socket.set_option(send_buffer_size_option, err);
		if (err)
		{
			close();
			return e_libnet_err_clisetsockopt;
		}

		asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
		m_socket.set_option(recv_buffer_size_option, err);
		if (err)
		{
			close();
			return e_libnet_err_clisetsockopt;
		}

		int nRet1 = setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(timeval)); //���÷��ͳ�ʱ
		int nRet2 = setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(timeval)); //���ý��ճ�ʱ
		printf("connect() , setsockopt nRet1 = %d , nRet2 = %d \r\n", nRet1, nRet2);

#ifndef _WIN32

		int ul = 1;
		int nRet3 = ioctl(m_socket.native_handle(), FIONBIO, &ul); //����Ϊ������ģʽ 

#else
		u_long ul = 1; // 1 ��ʾ������ģʽ
		int nRet3 = ioctlsocket(m_socket.native_handle(), FIONBIO, &ul); // ����Ϊ������ģʽ

#endif


		//���ùرղ�����
		std::error_code ec;
		asio::ip::tcp::no_delay no_delay_option(true);
		m_socket.set_option(no_delay_option, ec);

		//connect timeout
		if (timeout > 0)
		{
			m_timer.expires_from_now(std::chrono::milliseconds(3000));
			m_timer.async_wait(std::bind(&client::handle_connect_timeout, shared_from_this(), std::placeholders::_1));
		}

		//connect
		if (blocked)
		{
			m_socket.connect(srvep, err);
			m_timer.cancel();
			if (!err)
			{
				m_connectflag.exchange(true);
				run();
				return e_libnet_err_noerror;
			}
			else
			{
				close();
				return e_libnet_err_cliconnect;
			}
		}
		else //sync connect
		{
			m_socket.async_connect(srvep, std::bind(&client::handle_connect, shared_from_this(), std::placeholders::_1));
			return e_libnet_err_noerror;
		}
	}
	else
	{//SSL 
		char szPort[256] = { 0 };
		snprintf(szPort, sizeof(szPort), "%d", remoteport);
		asio::ip::tcp::resolver::results_type endpoints = resolver.resolve((char*)remoteip, szPort);

		//set callback function
		m_fnconnect = fnconnect;

		//connect timeout
		if (timeout > 0)
		{
			m_timer.expires_from_now(std::chrono::milliseconds(3000));
			m_timer.async_wait(std::bind(&client::handle_connect_timeout, shared_from_this(), std::placeholders::_1));
		}
		asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
		m_socket_.lowest_layer().set_option(send_buffer_size_option, err);

		asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
		m_socket_.lowest_layer().set_option(recv_buffer_size_option, err);

		asio::async_connect(m_socket_.lowest_layer(), endpoints,
			std::bind(&client::handle_connect, shared_from_this(),
				std::placeholders::_1));

		return e_libnet_err_noerror;
	}
}

void client::handle_write(const std::error_code& ec, size_t transize)
{
	if (ec)
	{
		close();
		if (client_manager::getInstance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	//������Ϻ�ִ������������Ӧ�Ѿ����͵�buffer 
	if (transize > 0)
		m_circularbuff.pop_front();


	std::unique_lock<std::mutex> _lock(m_autowrmtx);
	m_onwriting.exchange(write_packet());
}

void client::handle_read(const std::error_code& ec, size_t transize)
{
	if (ec)
	{
		if (client_manager::getInstance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	if (m_autoread)
	{
		if (!m_closeflag)
		{
			if (m_fnread)
			{
				m_fnread(get_server_id(), get_id(), m_readbuff, static_cast<uint32_t>(transize), NULL);
			}
			else
				return; //���ٶ�ȡ 
		}
		else
		{
			return; //���ٶ�ȡ 
		}

		if (m_bSSLFlag.load() == true)
		{//SSL ��ȡ 
			if (m_socket_.lowest_layer().is_open())
			{
				m_socket_.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
					std::bind(&client::handle_read,
						shared_from_this(),
						std::placeholders::_1,
						std::placeholders::_2));
			}
			else
			{//�ر� 
				if (client_manager::getInstance().pop_client(get_id()))
				{
					if (m_fnclose)
					{
						m_fnclose(get_server_id(), get_id());
					}
				}
			}
		}
		else
		{//��ͨSOCKETd ��ȡ 
			if (m_socket.is_open())
			{
				m_socket.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
					std::bind(&client::handle_read,
						shared_from_this(),
						std::placeholders::_1,
						std::placeholders::_2));
			}
			else
			{//�ر� 
				if (client_manager::getInstance().pop_client(get_id()))
				{
					if (m_fnclose)
					{
						m_fnclose(get_server_id(), get_id());
					}
				}
			}
		}
	}
	else
	{
		if (m_fnread)
		{
			m_fnread(get_server_id(), get_id(), m_usrreadbuffer, static_cast<uint32_t>(transize), NULL);
		}

		m_usrreadbuffer = NULL;
		m_inreading = false;
	}
}

void client::handle_connect(const std::error_code& ec)
{
	if (ec)
	{
		printf("network information : %s \r\n ", ec.message().c_str());
		m_timer.cancel();
		if (client_manager::getInstance().pop_client(get_id()))
		{
			if (m_fnconnect)
			{
				m_fnconnect(get_id(), 0, 0);
			}
		}
	}
	else
	{
		if (m_bSSLFlag.load() == true)
		{//���ӳɹ���ִ��SSL���� 
			m_socket_.async_handshake(asio::ssl::stream_base::client,
				std::bind(&client::handle_handshake, shared_from_this(),
					std::placeholders::_1));
		}
		else
		{//��SSL������
			m_connectflag.exchange(true);
			m_timer.cancel();
			if (m_fnconnect)
			{
				m_fnconnect(get_id(), 1, htons(m_socket.local_endpoint().port()));
			}
			run();
		}
	}
}


int32_t client::read(uint8_t* buffer,
	uint32_t* buffsize,
	bool blocked,
	bool certain)
{
	std::unique_lock<std::mutex> _lock(m_readmtx);
	if (m_closeflag)
		return e_libnet_err_clisocknotopen;
	if (!buffer || !buffsize || (0 == *buffsize))
	{
		return  e_libnet_err_invalidparam;
	}

	if (m_autoread)
	{
		return e_libnet_err_cliautoread;
	}

	uint32_t readsize = 0;
	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		std::error_code err;
		if (certain)
		{
			readsize = static_cast<uint32_t>(asio::read(m_socket, asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				//client_manager_singleton::get_mutable_instance().pop_client(get_id());
				client_manager::getInstance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				return e_libnet_err_noerror;
			}
		}
		else
		{
			readsize = static_cast<uint32_t>(m_socket.read_some(asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				//if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
				if (client_manager::getInstance().pop_client(get_id()))
				{
					if (m_fnclose)
						m_fnclose(get_server_id(), get_id());
				}
				return e_libnet_err_clireaddata;
			}
			else
			{
				*buffsize = readsize;
				return e_libnet_err_noerror;
			}
		}
	}
	else
	{
		if (m_inreading)
		{
			return e_libnet_err_cliprereadnotfinish;
		}

		m_inreading = true;
		m_usrreadbuffer = buffer;

		if (certain)
		{
			asio::async_read(m_socket,
				asio::buffer(m_usrreadbuffer, *buffsize),
				[this](std::error_code ec, std::size_t length)
				{
					handle_read(ec, length);
				});
		}
		else
		{
			auto self(shared_from_this());
			m_socket.async_read_some(asio::buffer(m_usrreadbuffer, *buffsize),
				[this, self](std::error_code ec, std::size_t length)
				{
					handle_read(ec, length);				
				});
		}

		return e_libnet_err_noerror;
	}
}


int32_t client::write(uint8_t* data,
	uint32_t datasize,
	bool blocked)
{
	std::unique_lock<std::mutex> _lock(m_autowrmtx);
	if (m_closeflag.load() || !m_connectflag.load())
		return e_libnet_err_clisocknotopen;

	int32_t ret = e_libnet_err_noerror;
	int32_t datasize2 = datasize;

	if (!data || (0 == datasize))
	{
		return e_libnet_err_invalidparam;
	}

	if (m_bSSLFlag.load() == true)
	{//SSL 
		if (!m_socket_.lowest_layer().is_open())
		{
			return e_libnet_err_clisocknotopen;
		}
	}
	else
	{//��ͨSOCKET
		if (!m_socket.is_open())
		{
			return e_libnet_err_clisocknotopen;
		}
	}

	if (blocked)
	{
		std::error_code ec;
		unsigned long nSendPos = 0, nSendRet = 0;

		while (datasize2 > 0)
		{//�ĳ�ѭ������
			if (m_bSSLFlag.load() == true)
				nSendRet = asio::write(m_socket_, asio::buffer(data + nSendPos, datasize2), ec);
			else
				nSendRet = asio::write(m_socket, asio::buffer(data + nSendPos, datasize2), ec);

			if (!ec)
			{//����û�г���
				if (nSendRet > 0)
				{
					nSendPos += nSendRet;
					datasize2 -= nSendRet;
				}
			}
			else//���ͳ�����������ѭ�����������ѭ��
				break;
		}
		if (!ec)
		{
			return e_libnet_err_noerror;
		}
		else
		{
			close();
			if (client_manager::getInstance().pop_client(get_id()))
			{
				if (m_fnclose)
					m_fnclose(get_server_id(), get_id());
			}
			return e_libnet_err_cliwritedata;
		}
	}
	else
	{//�첽����
		//�����첽���ͻ�����
		if (m_circularbuff.pMediaBuffer == NULL)
			m_circularbuff.InitFifo(CLIENT_MAX_SEND_BUFF_SIZE);

		//��д�뻺���� 
		if (m_circularbuff.push(data, datasize) == false)
		{
			m_circularbuff.Reset();
			m_circularbuff.push(data, datasize);
			m_onwriting.exchange(false);
		}

		//���û�����ڷ��ͣ�ֱ�ӽ��з��� ��������д����ɵĻص������ٽ��з��� 
		if (!m_onwriting.load())
		{
			m_onwriting.exchange(write_packet());
		}
	}

	return ret;
}


bool client::write_packet()
{
	if (m_closeflag.load())
	{
		m_onwriting.exchange(false);
		m_circularbuff.Reset();
		return false;
	}

	m_currwriteaddr = m_circularbuff.pop(&m_currwritesize);
	if (m_currwritesize > 0)
	{
		if (m_bSSLFlag.load() == true)
		{//SSL 
			asio::async_write(m_socket_, asio::buffer(m_currwriteaddr, m_currwritesize),
				std::bind(&client::handle_write,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2));
		}
		else
		{//��SSL
			asio::async_write(m_socket, asio::buffer(m_currwriteaddr, m_currwritesize),
				std::bind(&client::handle_write,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2));
		}

		return true;
	}

	return false;
}

void client::close()
{
	if (m_closeflag.load() == false)
	{
		m_closeflag.exchange(true);
		m_onwriting.exchange(false);
		m_fnread = NULL;

		if (m_bSSLFlag.load() == true)
		{//SSL ���ӹر�
			if (m_socket_.lowest_layer().is_open())
			{
				std::error_code ec;
				m_socket_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
				m_socket_.lowest_layer().close(ec);
				m_socket_.lowest_layer().release(ec);
			}
		}
		else
		{//��ͨ���ӹر�
			if (m_socket.is_open())
			{
				std::error_code ec;
				m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
				m_socket.close(ec);
				m_socket.release(ec);
			}
		}

	}
}

void client::handle_connect_timeout(const std::error_code& ec1)
{
	if (m_connectflag.load() == false)
	{
		client_manager::getInstance().pop_client(get_id());

		if (m_fnconnect)
		{//֪ͨ����ʧ��
			m_fnconnect(get_id(), 0, 0);
		}
	}
}

#endif

