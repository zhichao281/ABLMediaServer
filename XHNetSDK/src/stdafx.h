// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#ifndef _Stdafx_H
#define _Stdafx_H

//定义当前操作系统为Windows 
#if (defined _WIN32 || defined _WIN64)
#define      OS_System_Windows        1
#endif

#ifdef  OS_System_Windows //windows 

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
#define  EPOLLHANDLE  HANDLE

// Windows 头文件: 
#include <windows.h>
#include <thread>
#include <mutex>

#include "libnet_error.h"
#include "XHNetSDK.h"
#include <WinSock2.h>
#include "MediaFifo.h"
#include "wepoll.h"
#include "ConnectCheckPool.h"
#include "ClientReadPool.h"
#include "ClientSendPool.h"

#else //Linux

#define  SOCKET        int 
#define  EPOLLHANDLE   int 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <dirent.h>
#include <sys/stat.h>

#include<sys/types.h> 
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h> 
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/epoll.h> 

#include <pthread.h>
#include <signal.h>
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <vector>
#include <math.h>
#include <iconv.h>
#include <malloc.h>
#include <stdint.h>

#include <limits.h>
#include <sys/resource.h>

unsigned long   GetTickCount();
int64_t         GetTickCount64() ;
void            Sleep(int mMicroSecond) ;

#include "libnet_error.h"
#include "XHNetSDK.h"
#include "MediaFifo.h"
#include "ConnectCheckPool.h"
#include "ClientReadPool.h"
#include "ClientSendPool.h"

#endif

#endif 
