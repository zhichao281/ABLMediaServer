// XHNetSDK.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <cstring> 
#include <memory>
#include "XHNetSDK.h"
#include "libnet_error.h"
#include "client_manager.h"
#include "identifier_generator.h"
#include "ListenPool.h"
#include "UdpPool.h"
#include "ClientSendPool.h"

CConnectCheckPool*        connectCheckPool = NULL ;
CClientReadPool*          clientReadPool = NULL;
CListenPool*              listenPoolPool = NULL;
CUdpPool*                 udpPool        = NULL;
CClientSendPool*          clientSendPool = NULL;

#ifndef OS_System_Windows
unsigned long GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

int64_t GetTickCount64()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

//延时
void  Sleep(int mMicroSecond)
{
	if (mMicroSecond > 0)
		usleep(mMicroSecond * 1000);
	else
		usleep(5 * 1000);
}

#endif

LIBNET_API int32_t XHNetSDK_Init(uint32_t ioccount,
	uint32_t periocthread)
{
	int32_t ret = e_libnet_err_noerror;
#ifdef OS_System_Windows
	WSADATA wsaData;
	ret = WSAStartup(0x0202, &wsaData);
#else
	struct sigaction act, old_act;
 	std::memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	::sigaction(SIGPIPE, &act, &old_act);	
#endif
	listenPoolPool = new CListenPool(std::thread::hardware_concurrency());
	connectCheckPool = new CConnectCheckPool(1);
	clientReadPool = new CClientReadPool(std::thread::hardware_concurrency());
	udpPool = new CUdpPool(std::thread::hardware_concurrency());
	clientSendPool = new CClientSendPool(std::thread::hardware_concurrency());

	return ret;
}

LIBNET_API int32_t XHNetSDK_Deinit()
{
	int32_t ret = e_libnet_err_noerror;
	printf(" ---------------- NetSDK_Deinit ---------------\r\n");

#ifdef USE_BOOST
	client_manager_singleton::get_mutable_instance().pop_all_clients();
#else
	client_manager::get_instance().pop_all_clients();
#endif

#ifdef OS_System_Windows
	ret = ::WSACleanup();
#endif
    printf(" ---------------- pop_all_clients ---------------\r\n");
	connectCheckPool->destoryPool();
	delete connectCheckPool;
	connectCheckPool = NULL;
    printf(" ---------------- delete connectCheckPool ---------------\r\n");

	clientReadPool->destroyReadPool();
	delete clientReadPool;
    printf(" ---------------- delete clientReadPool ---------------\r\n");

	listenPoolPool->destroyListenPool();
	delete listenPoolPool;
	printf(" ---------------- delete destroyListenPool ---------------\r\n");

	udpPool->destoryUdpPool();
	delete udpPool;
    printf(" ---------------- delete udpPool ---------------\r\n");
	
	clientSendPool->destroySendPool();
	delete clientSendPool;
    printf(" ---------------- delete clientSendPool ---------------\r\n");

	printf(" ---------------- Success ---------------\r\n");

	return ret;
}

LIBNET_API int32_t XHNetSDK_Listen(
	int8_t* localip,
	uint16_t localport,
	NETHANDLE* srvhandle,
	accept_callback fnaccept,
	read_callback fnread,
	close_callback fnclose,
	uint8_t autoread,
	bool bSSLFlag
)
{
	return listenPoolPool->Listen(localip, localport,srvhandle, fnaccept, fnread, fnclose, autoread,bSSLFlag);
}

LIBNET_API int32_t XHNetSDK_Unlisten(NETHANDLE srvhandle)
{
	int32_t ret = e_libnet_err_noerror;
	listenPoolPool->Unlisten(srvhandle);
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
	uint8_t autoread,
	bool bSSLFlag)
{
	int32_t ret = e_libnet_err_noerror;

    if (!remoteip || (0 == remoteport) || !clihandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
 		*clihandle = INVALID_NETHANDLE;
#ifdef USE_BOOST
		client_ptr cli = client_manager_singleton::get_mutable_instance().malloc_client(INVALID_NETHANDLE, fnread, fnclose, (0 != autoread) ? true : false, bSSLFlag, clientType_Connect, NULL);
#else
		client_ptr cli = client_manager::get_instance().malloc_client(INVALID_NETHANDLE, fnread, fnclose, (0 != autoread) ? true : false, bSSLFlag, clientType_Connect, NULL);
#endif
		if (cli)
		{
				ret = cli->connect(remoteip, remoteport, localip, locaport, (0 != blocked), fnconnect, timeout);
				if (e_libnet_err_noerror == ret)
				{
					*clihandle = cli->get_id();
#ifdef USE_BOOST
					if (client_manager_singleton::get_mutable_instance().push_client(cli))
					{
 						connectCheckPool->InsertIntoTask(cli->get_id());
  					}
					else
					{
						cli->close();
						ret = e_libnet_err_climanage;
					}
 				}
				else
				{
					client_manager_singleton::get_mutable_instance().pop_client(cli->get_id());
				}
#else
					if (client_manager::get_instance().push_client(cli))
					{
 						connectCheckPool->InsertIntoTask(cli->get_id());
  					}
					else
					{
						cli->close();
						ret = e_libnet_err_climanage;
					}
 				}
				else
				{
					client_manager::get_instance().pop_client(cli->get_id());
				}
#endif
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

    if (INVALID_NETHANDLE == clihandle)
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
		//先执行从线程池里面移除
		if (true)
		{
#ifdef USE_BOOST
			client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(clihandle);
#else
			client_ptr cli = client_manager::get_instance().get_client(clihandle);
#endif
			if (cli != NULL)
			{
				cli->close();
				clientSendPool->DeleteFromTask(cli->nSendThreadOrder.load(), cli->get_id());
				clientReadPool->DeleteFromTask(cli->get_id()); 
 			}
		}
 
#ifdef USE_BOOST
		if (!client_manager_singleton::get_mutable_instance().pop_client(clihandle))
#else
		if (!client_manager::get_instance().pop_client(clihandle))
#endif
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

    if (!data || (0 == datasize))
	{
		ret = e_libnet_err_invalidparam;
	}
	else
	{
#ifdef USE_BOOST
		client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(clihandle);
#else
		client_ptr cli = client_manager::get_instance().get_client(clihandle);
#endif
		if (cli)
		{
			if (cli->GetConnect() == true)
				ret = cli->write(data, datasize, (0 != blocked) ? true : false);
			else
			{
 				return e_libnet_err_clisocknotopen;
		    }
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

	ret = udpPool->BuildUdp(localip, localport, bindaddr, udphandle, fnread, autoread);

	return ret;
}

LIBNET_API int32_t XHNetSDK_DestoryUdp(NETHANDLE udphandle)
{
	int32_t ret = e_libnet_err_noerror;

	ret = udpPool->DestoryUdp(udphandle);

	return ret;
}

LIBNET_API int32_t XHNetSDK_Sendto(NETHANDLE udphandle,
	uint8_t* data,
	uint32_t datasize,
	void* remoteaddress)
{
	int32_t ret = e_libnet_err_noerror;

	ret = udpPool->Sendto(udphandle, data, datasize, remoteaddress);

	return ret;
}

LIBNET_API int32_t XHNetSDK_Recvfrom(NETHANDLE udphandle,
	uint8_t* buffer,
	uint32_t* buffsize,
	void* remoteaddress,
	uint8_t blocked)
{
	int32_t ret = e_libnet_err_noerror;

	return ret;
}

LIBNET_API int32_t XHNetSDK_Multicast(NETHANDLE udphandle,
	uint8_t option,
	int8_t* multicastip,
	uint8_t value)
{
	int32_t ret = e_libnet_err_noerror;


	return ret;
}

LIBNET_API NETHANDLE XHNetSDK_GenerateIdentifier()
{
	return generate_identifier();
}

