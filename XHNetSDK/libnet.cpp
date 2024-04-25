
#ifdef USE_BOOST
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include "libnet.h"
#include "libnet_error.h"
#include "io_context_pool.h"
#include "server_manager.h"
#include "client_manager.h"
#include "udp_session_manager.h"
#include "identifier_generator.h"


io_context_pool g_iocpool;
uint32_t g_deinittimes = 0;
int32_t g_initret = e_libnet_err_noninit;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
auto_lock::al_mutex g_initmtx;
#else
auto_lock::al_spin g_initmtx;
#endif

LIBNET_API int32_t XHNetSDK_Init(uint32_t ioccount,
							   uint32_t periocthread)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_initmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_initmtx);
#endif

	++g_deinittimes;

	if (e_libnet_err_noerror != g_initret)
	{
		if (!g_iocpool.is_init())
		{
			g_initret = g_iocpool.init(ioccount, periocthread);
		}

		if (e_libnet_err_noerror == g_initret)
		{
			g_initret = g_iocpool.run();
		}
	}	

	return g_initret;
}

LIBNET_API int32_t XHNetSDK_Deinit()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_initmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_initmtx);
#endif

	int32_t ret = e_libnet_err_noerror;

	if (0 == g_deinittimes)
	{
		ret = e_libnet_err_noninit;
	}
	else if (0 != --g_deinittimes)
	{
		ret = e_libnet_err_stillusing;
	}
	else
	{
		server_manager_singleton::get_mutable_instance().close_all_servers();
		client_manager_singleton::get_mutable_instance().pop_all_clients();
		udp_session_manager_singleton::get_mutable_instance().pop_all_udp_sessions();
		g_iocpool.close();
		g_initret = e_libnet_err_noninit;
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Listen(int8_t* localip,
								 uint16_t localport,
								 NETHANDLE* srvhandle,
								 accept_callback fnaccept,
								 read_callback fnread,
								 close_callback fnclose,
								 uint8_t autoread)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!localip || (0 == localport) || !srvhandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		*srvhandle = INVALID_NETHANDLE;

		boost::system::error_code ec;
		boost::asio::ip::address addr = boost::asio::ip::address::from_string(reinterpret_cast<char*>(localip), ec);
		if (ec)
		{
			ret = e_libnet_err_srvinvalidip;
		}
		else
		{
			boost::asio::ip::tcp::endpoint endpoint(addr, localport);

			try
			{
				server_ptr s = boost::make_shared<server>(boost::ref(g_iocpool.get_io_context()), boost::ref(endpoint), 
					fnaccept, fnread, fnclose, (0 != autoread) ? true : false);

				if (server_manager_singleton::get_mutable_instance().push_server(s))
				{
					ret = s->run();
					if (e_libnet_err_noerror == ret)
					{
						*srvhandle = s->get_id();
					}
					else
					{
						server_manager_singleton::get_mutable_instance().pop_server(s->get_id());
					}
				}
				else
				{
					s->close();
					ret = e_libnet_err_srvmanage;
				}
			}
			catch (const std::bad_alloc& /*e*/)
			{
				ret = e_libnet_err_srvcreate;
			}
			catch (...)
			{
				ret = e_libnet_err_srvcreate;
			}
	
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Unlisten(NETHANDLE srvhandle)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (INVALID_NETHANDLE == srvhandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		if (!server_manager_singleton::get_mutable_instance().pop_server(srvhandle))
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Connect(int8_t* remoteip,
	uint16_t remoteport,
	int8_t* localip,
	uint16_t locaport,
	NETHANDLE* clihandle,
	read_callback fnread,
	close_callback fnclose,
	connect_callback fnconnect,
	uint8_t blocked,
	uint32_t timeout,
	uint8_t autoread)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!remoteip || (0 == remoteport) || !clihandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		*clihandle = INVALID_NETHANDLE;
		client_ptr cli = client_manager_singleton::get_mutable_instance().malloc_client(g_iocpool.get_io_context(), INVALID_NETHANDLE, fnread, fnclose, (0 != autoread) ? true : false);
		if (cli)
		{
			if (client_manager_singleton::get_mutable_instance().push_client(cli))
			{
				ret = cli->connect(remoteip, remoteport, localip, locaport, (0 != blocked), fnconnect, timeout);
				if (e_libnet_err_noerror == ret)
				{
					*clihandle = cli->get_id();
				}
				else
				{
					client_manager_singleton::get_mutable_instance().pop_client(cli->get_id());
				}
			}
			else
			{
				cli->close();
				ret = e_libnet_err_climanage;
			}
		}
		else
		{
			ret = e_libnet_err_clicreate;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Disconnect(NETHANDLE clihandle)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (INVALID_NETHANDLE == clihandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		if (!client_manager_singleton::get_mutable_instance().pop_client(clihandle))
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Write(NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	uint8_t blocked)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!data || (0 == datasize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(clihandle);
		if (cli)
		{
			ret = cli->write(data, datasize, (0 != blocked) ? true : false);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Read(NETHANDLE clihandle,
	uint8_t* buffer,
	uint32_t* buffsize,
	uint8_t blocked,
	uint8_t certain)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!buffer || !buffsize || (0 == *buffsize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(clihandle);
		if (cli)
		{
			ret = cli->read(buffer, buffsize, (0 != blocked) ? true : false, (0 != certain) ? true : false);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_BuildUdp(int8_t* localip,
	uint16_t localport,
	void* bindaddr,
	NETHANDLE* udphandle,
	read_callback fnread,
	uint8_t autoread)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!udphandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		*udphandle = INVALID_NETHANDLE;

		try
		{
			udp_session_ptr s = boost::make_shared<udp_session>(boost::ref(g_iocpool.get_io_context()));

			if (udp_session_manager_singleton::get_mutable_instance().push_udp_session(s))
			{
				ret = s->init(localip, localport, bindaddr, fnread, (0 != autoread) ? true : false);
				if (e_libnet_err_noerror == ret)
				{
					*udphandle = s->get_id();
				}
				else
				{
					udp_session_manager_singleton::get_mutable_instance().pop_udp_session(s->get_id());
				}
			}
			else
			{
				s->close();
				ret = e_libnet_err_climanage;
			}
		}
		catch (const std::bad_alloc& /*e*/)
		{
			ret = e_libnet_err_srvcreate;
		}
		catch (...)
		{
			ret = e_libnet_err_srvcreate;
		}
		
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_DestoryUdp(NETHANDLE udphandle)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!udphandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		if (!udp_session_manager_singleton::get_mutable_instance().pop_udp_session(udphandle))
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Sendto(NETHANDLE udphandle,
	uint8_t* data,
	uint32_t datasize,
	void* remoteaddress)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if ((INVALID_NETHANDLE == udphandle) || !data || (0 == datasize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		udp_session_ptr s = udp_session_manager_singleton::get_mutable_instance().get_udp_session(udphandle);
		if (s)
		{
			ret = s->send_to(data, datasize, remoteaddress);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Recvfrom(NETHANDLE udphandle,
	uint8_t* buffer,
	uint32_t* buffsize,
	void* remoteaddress,
	uint8_t blocked)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if ((INVALID_NETHANDLE == udphandle) || !buffer || !buffsize || (0 == *buffsize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		udp_session_ptr s = udp_session_manager_singleton::get_mutable_instance().get_udp_session(udphandle);
		if (s)
		{
			ret = s->recv_from(buffer, buffsize, remoteaddress, ( 0 != blocked) ? true : false);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Multicast(NETHANDLE udphandle,
	uint8_t option,
	int8_t* multicastip,
	uint8_t value)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (INVALID_NETHANDLE == udphandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		udp_session_ptr s = udp_session_manager_singleton::get_mutable_instance().get_udp_session(udphandle);
		if (s)
		{
			ret = s->multicast(option, multicastip, value);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API NETHANDLE XHNetSDK_GenerateIdentifier()
{
	return generate_identifier(); 
}

#else

#include <memory>
#include "libnet.h"
#include "libnet_error.h"
#include "io_context_pool.h"
#include "server_manager.h"
#include "client_manager.h"
#include "udp_session_manager.h"
#include "identifier_generator.h"



io_context_pool g_iocpool;
uint32_t g_deinittimes = 0;
int32_t g_initret = e_libnet_err_noninit;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
auto_lock::al_mutex g_initmtx;
#else
auto_lock::al_spin g_initmtx;
#endif

LIBNET_API int32_t XHNetSDK_Init(uint32_t ioccount,
	uint32_t periocthread)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_initmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_initmtx);
#endif

	++g_deinittimes;

	if (e_libnet_err_noerror != g_initret)
	{
		if (!g_iocpool.is_init())
		{
			g_initret = g_iocpool.init(ioccount, periocthread);
		}

		if (e_libnet_err_noerror == g_initret)
		{
			g_initret = g_iocpool.run();
		}
	}

	return g_initret;
}


LIBNET_API int32_t XHNetSDK_Deinit()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_initmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_initmtx);
#endif

	int32_t ret = e_libnet_err_noerror;

	if (0 == g_deinittimes)
	{
		ret = e_libnet_err_noninit;
	}
	else if (0 != --g_deinittimes)
	{
		ret = e_libnet_err_stillusing;
	}
	else
	{
		server_manager::getInstance().close_all_servers();
		client_manager::getInstance().pop_all_clients();
		udp_session_manager::getInstance().pop_all_udp_sessions();
		g_iocpool.close();
		g_initret = e_libnet_err_noninit;
	}

	return ret;
}



LIBNET_API int32_t XHNetSDK_Listen(int8_t* localip,
								uint16_t localport,
								NETHANDLE* srvhandle,
								accept_callback fnaccept,
								read_callback fnread,
								close_callback fnclose,
								uint8_t autoread)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!localip || (0 == localport) || !srvhandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		*srvhandle = INVALID_NETHANDLE;


		std::error_code ec;
		asio::ip::address addr = asio::ip::address::from_string(reinterpret_cast<char*>(localip), ec);
		if (ec)
		{
			ret = e_libnet_err_srvinvalidip;
		}
		else
		{
			asio::ip::tcp::endpoint endpoint(addr, localport);

			try
			{


				auto s = std::make_shared<server>(std::ref(g_iocpool.get_io_context()), std::ref(endpoint),
					fnaccept, fnread, fnclose, (0 != autoread) ? true : false);

				if (server_manager::getInstance().push_server(s))
				{
					ret = s->run();
					if (e_libnet_err_noerror == ret)
					{
						*srvhandle = s->get_id();
					}
					else
					{
						server_manager::getInstance().pop_server(s->get_id());
					}
				}
				else
				{
					s->close();
					ret = e_libnet_err_srvmanage;
				}
			}
			catch (const std::bad_alloc& /*e*/)
			{
				ret = e_libnet_err_srvcreate;
			}
			catch (...)
			{
				ret = e_libnet_err_srvcreate;
			}

		}
	}

	return ret;
}




LIBNET_API int32_t XHNetSDK_Unlisten(NETHANDLE srvhandle)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (INVALID_NETHANDLE == srvhandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		if (!server_manager::getInstance().pop_server(srvhandle))
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Connect(int8_t* remoteip,
	uint16_t remoteport,
	int8_t* localip,
	uint16_t locaport,
	NETHANDLE* clihandle,
	read_callback fnread,
	close_callback fnclose,
	connect_callback fnconnect,
	uint8_t blocked,
	uint32_t timeout,
	uint8_t autoread)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!remoteip || (0 == remoteport) || !clihandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		*clihandle = INVALID_NETHANDLE;
		client_ptr cli = client_manager::getInstance().malloc_client(g_iocpool.get_io_context(), INVALID_NETHANDLE, fnread, fnclose, (0 != autoread) ? true : false);
		if (cli)
		{
			if (client_manager::getInstance().push_client(cli))
			{
				ret = cli->connect(remoteip, remoteport, localip, locaport, (0 != blocked), fnconnect, timeout);
				if (e_libnet_err_noerror == ret)
				{
					*clihandle = cli->get_id();
				}
				else
				{
					client_manager::getInstance().pop_client(cli->get_id());
				}
			}
			else
			{
				cli->close();
				ret = e_libnet_err_climanage;
			}
		}
		else
		{
			ret = e_libnet_err_clicreate;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Disconnect(NETHANDLE clihandle)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (INVALID_NETHANDLE == clihandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		if (!client_manager::getInstance().pop_client(clihandle))
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Write(NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	uint8_t blocked)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!data || (0 == datasize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		client_ptr cli = client_manager::getInstance().get_client(clihandle);
		if (cli)
		{
			ret = cli->write(data, datasize, (0 != blocked) ? true : false);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Read(NETHANDLE clihandle,
	uint8_t* buffer,
	uint32_t* buffsize,
	uint8_t blocked,
	uint8_t certain)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!buffer || !buffsize || (0 == *buffsize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		client_ptr cli = client_manager::getInstance().get_client(clihandle);
		if (cli)
		{
			ret = cli->read(buffer, buffsize, (0 != blocked) ? true : false, (0 != certain) ? true : false);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_BuildUdp(int8_t* localip,
	uint16_t localport,
	void* bindaddr,
	NETHANDLE* udphandle,
	read_callback fnread,
	uint8_t autoread)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!udphandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		*udphandle = INVALID_NETHANDLE;

		try
		{
			udp_session_ptr s = std::make_shared<udp_session>(std::ref(g_iocpool.get_io_context()));

			if (udp_session_manager::getInstance().push_udp_session(s))
			{
				ret = s->init(localip, localport, bindaddr, fnread, (0 != autoread) ? true : false);
				if (e_libnet_err_noerror == ret)
				{
					*udphandle = s->get_id();
				}
				else
				{
					udp_session_manager::getInstance().pop_udp_session(s->get_id());
				}
		}
			else
			{
				s->close();
				ret = e_libnet_err_climanage;
			}
	}
		catch (const std::bad_alloc& /*e*/)
		{
			ret = e_libnet_err_srvcreate;
		}
		catch (...)
		{
			ret = e_libnet_err_srvcreate;
		}

}

	return ret;
}

LIBNET_API int32_t XHNetSDK_DestoryUdp(NETHANDLE udphandle)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (!udphandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		if (!udp_session_manager::getInstance().pop_udp_session(udphandle))
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Sendto(NETHANDLE udphandle,
	uint8_t* data,
	uint32_t datasize,
	void* remoteaddress)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if ((INVALID_NETHANDLE == udphandle) || !data || (0 == datasize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		udp_session_ptr s = udp_session_manager::getInstance().get_udp_session(udphandle);
		if (s)
		{
			ret = s->send_to(data, datasize, remoteaddress);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Recvfrom(NETHANDLE udphandle,
	uint8_t* buffer,
	uint32_t* buffsize,
	void* remoteaddress,
	uint8_t blocked)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if ((INVALID_NETHANDLE == udphandle) || !buffer || !buffsize || (0 == *buffsize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		udp_session_ptr s = udp_session_manager::getInstance().get_udp_session(udphandle);
		if (s)
		{
			ret = s->recv_from(buffer, buffsize, remoteaddress, (0 != blocked) ? true : false);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API int32_t XHNetSDK_Multicast(NETHANDLE udphandle,
	uint8_t option,
	int8_t* multicastip,
	uint8_t value)
{
	int32_t ret = e_libnet_err_noerror;

	if (e_libnet_err_noerror != g_initret)
	{
		ret = e_libnet_err_noninit;
	}
	else if (INVALID_NETHANDLE == udphandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		udp_session_ptr s = udp_session_manager::getInstance().get_udp_session(udphandle);
		if (s)
		{
			ret = s->multicast(option, multicastip, value);
		}
		else
		{
			ret = e_libnet_err_invalidhandle;
		}
	}

	return ret;
}

LIBNET_API NETHANDLE XHNetSDK_GenerateIdentifier()
{
	return generate_identifier();
}


#endif

