// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#ifndef _Stdafx_H
#define _Stdafx_H

#define   RTP_PAYLOAD_MAX_SIZE    1320 

//定义当前操作系统为Windows 
#if (defined _WIN32 || defined _WIN64)
#define      OS_System_Windows        1
#endif

#ifdef  OS_System_Windows //windows 



#define WIN32_LEAN_AND_MEAN   // 从 Windows 头中排除极少使用的资料
// Windows 头文件: 
#include <WinSock2.h>

#include <windows.h>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

#else //Linux 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#include <memory>
#include <unordered_map>

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/epoll.h> 

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

#endif 

#endif 
