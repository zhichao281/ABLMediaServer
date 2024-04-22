
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#define _WINSOCKAPI_
#include <windows.h>
#include <tchar.h>
#include <dbghelp.h>
#include <string>
#include <atlbase.h>
#include <atlstr.h>
#include <strsafe.h>
#include <stdexcept>
#include <shellapi.h>
#include <stdlib.h>

#ifdef UNICODE    
#     define tstring wstring    
#else       

#     define tstring string 
#endif

#pragma comment(lib, "dbghelp.lib")


namespace WisdomUtils 
{

	void simple_log(const std::wstring& log_msg);

	class CDumpCatch
	{
	public:
		CDumpCatch();
		~CDumpCatch();
	public:

		static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI TempSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
		static BOOL ReleaseDumpFile(const std::wstring& strPath, EXCEPTION_POINTERS *pException);
		static LONG WINAPI UnhandledExceptionFilterEx(struct _EXCEPTION_POINTERS *pException);
		static void MyPureCallHandler(void);
		static void MyInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);


		BOOL AddExceptionHandle();
		BOOL RemoveExceptionHandle();
		BOOL PreventSetUnhandledExceptionFilter();
		void SetInvalidHandle();
		void UnSetInvalidHandle();
	private:
		LPTOP_LEVEL_EXCEPTION_FILTER m_preFilter;
		_invalid_parameter_handler m_preIph;
		_purecall_handler m_prePch;    
	};
};
