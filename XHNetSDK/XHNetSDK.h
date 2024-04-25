#pragma once

#include <stdint.h>

#if (defined _WIN32 || defined _WIN64)
#define LIBNET_CALLMETHOD _stdcall
#ifdef LIBNET_STATIC
#define LIBNET_API
#else
#ifdef LIBNET_EXPORTS
#define LIBNET_API  __declspec(dllexport)
#else
#define LIBNET_API  __declspec(dllimport)
#endif
#endif 
#else
#define LIBNET_CALLMETHOD
#define LIBNET_API __attribute__((visibility("default")))
#endif 

#ifndef NETHANDLE
#if (defined(__x86_64__) || defined(_M_X64) || defined(_amd64))
#define NETHANDLE uint64_t
#else
#define NETHANDLE uint32_t
#endif
#endif

#ifndef INVALID_NETHANDLE
#define  INVALID_NETHANDLE  0
#endif


#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (LIBNET_CALLMETHOD	*accept_callback)(NETHANDLE srvhandle,
		NETHANDLE clihandle,
		void* address);


	typedef void(LIBNET_CALLMETHOD	*connect_callback)(NETHANDLE clihandle,
		uint8_t result, uint16_t nLocalPort);


	typedef void (LIBNET_CALLMETHOD *read_callback)(NETHANDLE srvhandle,
		NETHANDLE clihandle,
		uint8_t* data,
		uint32_t datasize,
		void* address);


	typedef void (LIBNET_CALLMETHOD	*close_callback)(NETHANDLE srvhandle,
		NETHANDLE clihandle);


	LIBNET_API int32_t XHNetSDK_Init(uint32_t ioccount,
		uint32_t periocthread);

	LIBNET_API int32_t XHNetSDK_Deinit();

	LIBNET_API int32_t XHNetSDK_Listen(int8_t* localip,
		uint16_t localport,
		NETHANDLE* srvhandle,
		accept_callback fnaccept,
		read_callback fnread,
		close_callback fnclose,
		uint8_t autoread);

	LIBNET_API int32_t XHNetSDK_Unlisten(NETHANDLE srvhandle);

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
		uint8_t autoread);

	LIBNET_API int32_t XHNetSDK_Disconnect(NETHANDLE clihandle);

	LIBNET_API int32_t XHNetSDK_Write(NETHANDLE clihandle,
		uint8_t* data,
		uint32_t datasize,
		uint8_t blocked);

	LIBNET_API int32_t XHNetSDK_Read(NETHANDLE clihandle,
		uint8_t* buffer,
		uint32_t* buffsize,
		uint8_t blocked,
		uint8_t certain);

	LIBNET_API int32_t XHNetSDK_BuildUdp(int8_t* localip,
		uint16_t localport,
		void* bindaddr,
		NETHANDLE* udphandle,
		read_callback fnread,
		uint8_t autoread);

	LIBNET_API int32_t XHNetSDK_DestoryUdp(NETHANDLE udphandle);

	LIBNET_API int32_t XHNetSDK_Sendto(NETHANDLE udphandle,
		uint8_t* data,
		uint32_t datasize,
		void* remoteaddress);

	LIBNET_API int32_t XHNetSDK_Recvfrom(NETHANDLE udphandle,
		uint8_t* buffer,
		uint32_t* buffsize,
		void* remoteaddress,
		uint8_t blocked);

	LIBNET_API int32_t XHNetSDK_Multicast(NETHANDLE udphandle,
		uint8_t option,
		int8_t* multicastip,
		uint8_t value);

	LIBNET_API NETHANDLE XHNetSDK_GenerateIdentifier();

#ifdef __cplusplus
}
#endif
