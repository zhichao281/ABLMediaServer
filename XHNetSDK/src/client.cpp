/*
功能：
    实现对于tcp方式的客户端封装，包括连接远程服务器产生的客户端，或者外部设备连接进来的客户端 

	日期    2025-02-09
	作者    罗家兄弟
	QQ      79941308
	E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#ifdef USE_BOOST
#include <boost/bind.hpp>
#else
#include <functional>
#endif
#include "client_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"
#include "ClientSendPool.h"
#include "ClientReadPool.h"
#include "ConnectCheckPool.h"

extern CClientReadPool*     clientReadPool;
extern CConnectCheckPool*   connectCheckPool ;
extern CClientSendPool*     clientSendPool;

client::client(NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread,
	bool bSSLFlag,
	ClientType nCLientType,
	accept_callback  fnaccept)
	: m_srvid(srvid)
	, m_id(generate_identifier())
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_fnconnect(NULL)
	, m_closeflag(true)
	, m_autoread(autoread)
	, m_inreading(false)
	, m_usrreadbuffer(NULL)
	, m_onwriting(false)

	, m_currwriteaddr(NULL)
	, m_currwritesize(0)
	, m_nClientType(nCLientType)
	, m_fnaccept(fnaccept)
{
	nPopLength = 0;
	nSendPos = 0;
	nSendLength = 0;
	pPopData = NULL;
	nRecvThreadOrder = nSendThreadOrder = 0;
	m_connectflag.exchange(false);
	nCreateTime = ::GetTickCount64();
}

client::~client(void)
{
	close();
	m_circularbuff.FreeFifo();
#ifndef OS_System_Windows 
   malloc_trim(0); 
#endif
}

int32_t client::run()
{
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
	//set callback function
	m_fnconnect = fnconnect;

	int     nSetRet;
	int32_t ret = e_libnet_err_noerror;
	u_long      iMode = 1;
	m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_Socket < 0)
	{
		perror("socket");
		return e_libnet_err_invalidhandle;
	}
	m_closeflag.store(false);

	memset((char*)&tAddr4, 0x00, sizeof(tAddr4));
	tAddr4.sin_family = AF_INET;
	tAddr4.sin_port = htons(remoteport);
 	tAddr4.sin_addr.s_addr = inet_addr((char*)remoteip);

	//设置异步
#ifdef OS_System_Windows
	int nRet2 = ioctlsocket(m_Socket, FIONBIO, &iMode);
#else 	
    ::fcntl(m_Socket, F_SETFL, O_NONBLOCK);
#endif 

	//设置地址、端口允许重用
	int32_t   nReUse = 1;
	setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&nReUse, static_cast<socklen_t>(sizeof(nReUse)));
#ifndef OS_System_Windows
	setsockopt(m_Socket, SOL_SOCKET, SO_REUSEPORT, (char*)&nReUse, static_cast<socklen_t>(sizeof(nReUse)));
#endif 

	//设置不允许延时关闭
	struct linger sLinger;
	sLinger.l_onoff = 0;
	sLinger.l_linger = 1;
	nSetRet = setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *)&sLinger, sizeof(linger));

	//设置发送、接收缓冲区 
	int  size = 1024 * 1024 * 2;
	nSetRet = setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
	nSetRet = setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));

	//设置发送、接收超时 
	struct timeval timeOut = { 3,0 };
	socklen_t      timeOutLength = sizeof(timeOut);
	nSetRet = setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeOut, timeOutLength);
	nSetRet = setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, timeOutLength);
 
	return e_libnet_err_noerror;
}


int32_t client::read(uint8_t* buffer,
	uint32_t* buffsize,
	bool blocked,
	bool certain)
{
	int32_t ret = e_libnet_err_noerror;

	return ret;
}

int32_t client::write(uint8_t* data,
	uint32_t datasize,
	bool blocked)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	int32_t ret = e_libnet_err_clisocknotopen;
	int     nSendRet = 0;
	int     nSendPos = 0;
	int     nSize = datasize;
	bool    ec = false;

	if (m_connectflag.load() == true)
	{
		if (blocked)
		{//同步发送
			ec = false;
			while (nSize > 0)
			{
				nSendRet = send(m_Socket, (const char*)data + nSendPos, nSize, 0);
				if (nSendRet > 0)
				{
					nSendPos += nSendRet;
					nSize   -= nSendRet;
				}
				else
				{
					ec = true;
					break;
				}
			}

			if (!ec)
			{
				return e_libnet_err_noerror;
			}
			else
			{
 				client_manager_singleton::get_mutable_instance().pop_client(get_id());
 				return e_libnet_err_cliwritedata;
			}

			return  e_libnet_err_noerror;
 		}
		else
		{//异步发送 
		 //分配异步发送缓冲区
			if (m_circularbuff.pMediaBuffer == NULL)
				m_circularbuff.InitFifo(CLIENT_MAX_SEND_BUFF_SIZE);

			//先写入缓冲区 
			if (m_circularbuff.push(data, datasize) == false)
			{
				m_circularbuff.Reset();
				m_circularbuff.push(data, datasize);
				m_onwriting.exchange(false);
			}

			clientSendPool->notify_one(nSendThreadOrder.load());
			return  e_libnet_err_noerror;
		}
	}

	return ret;
}

void client::close()
{
	if(m_closeflag.load() == false)
	{
#ifdef OS_System_Windows
 	  closesocket(m_Socket);
#else 
	  ::shutdown(m_Socket, SHUT_RDWR);
  	  ::close(m_Socket) ;
#endif
	  m_closeflag.exchange(true);
	  m_connectflag.exchange(false);
	}
}

void client::SetConnect(bool bFlag)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_closeflag.exchange(false);//尚未关闭
	m_connectflag.exchange(bFlag);
}

bool client::GetConnect()
{
	return  m_connectflag.load();
}
