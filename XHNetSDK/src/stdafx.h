// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#ifndef _Stdafx_H
#define _Stdafx_H

//���嵱ǰ����ϵͳΪWindows 
#if (defined _WIN32 || defined _WIN64)
#define      OS_System_Windows        1
#endif

#ifdef  OS_System_Windows //windows 

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ���ų�����ʹ�õ�����
#define  EPOLLHANDLE  HANDLE

// Windows ͷ�ļ�: 
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
