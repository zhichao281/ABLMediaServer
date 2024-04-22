#pragma once

#if (defined _WIN32 || defined _WIN64)




#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <shellapi.h>  // must come after windows.h
// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#pragma comment(lib,"version.lib")
#pragma comment(lib,"Imm32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"dbghelp.lib")
#pragma comment(lib,"dnsapi.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"msimg32.lib")
#pragma comment(lib,"odbc32.lib")
#pragma comment(lib,"odbccp32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"usp10.lib")
#pragma comment(lib,"uuid.lib")
#pragma comment(lib,"wininet.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"winspool.lib")
#pragma comment(lib,"delayimp.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"crypt32.lib")
#pragma comment(lib,"iphlpapi.lib")
#pragma comment(lib,"secur32.lib")
#pragma comment(lib,"dmoguids.lib")
#pragma comment(lib,"wmcodecdspuuid.lib")
#pragma comment(lib,"amstrmid.lib")
#pragma comment(lib,"msdmo.lib")
#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"oleaut32.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dwmapi.lib")


#ifdef _DEBUG



#ifdef _WIN64
#pragma comment(lib, "./webrtc/lib/x64/debug/webrtc.lib")

#else

#pragma comment(lib, "./webrtc/lib/debug/webrtc.lib")


#endif //WIN64


#else



#ifdef _WIN64
#pragma comment(lib, "./webrtc/lib/x64/release/webrtc.lib")

#else

#pragma comment(lib, "./webrtc/lib/release/webrtc.lib")

#endif //WIN64




#endif //_DEBUG

#endif