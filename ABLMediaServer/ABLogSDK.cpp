// ABLogSDK.cpp : Defines the exported functions for the DLL application.
//
#ifdef OS_System_Windows 

#include "stdafx.h"
#include "ABLogSDK.h"
#include "MyWriteLogFile.h"
#include "WriteAVFile.h"

extern char                           ABL_szCurrentPath[512];
extern char                           ABL_szLogPath[512] ;
extern CWriteAVFile                   myLogFile;
extern std::mutex                     WriteLogFileLock_Boost;
extern char*                          ABL_writeBuffer;
extern char*                          ABL_szLogText;
extern bool                           bInitLogFileFlag ;
extern char                           ABL_BaseLogFileName[256] ;
extern int                            ABL_MaxLogFileCount ;//最大保留日志文件个数

char                                  ABL_LogStringArrray[3][64] = { 0 };

bool StartLogFile(char* szSubPath,char* szBaseLogFile, int nMaxSaveLogFileCount)
{
	unsigned long dwFileSize = 0;

	if( (szSubPath == NULL) || strstr(szBaseLogFile, "*.log") == NULL || nMaxSaveLogFileCount <= 0)
		return false;

	strcpy(ABL_BaseLogFileName, szBaseLogFile);
	ABL_MaxLogFileCount = nMaxSaveLogFileCount;
	strcpy(ABL_LogStringArrray[0], "Log_Debug");
	strcpy(ABL_LogStringArrray[1], "Log_Title");
	strcpy(ABL_LogStringArrray[2], "Log_Error");

	char              szLogFile[256] = { 0 };
	bool              bFileExist;

	GetCurrentPath(ABL_szCurrentPath);
	sprintf(ABL_szLogPath, "%sLog", ABL_szCurrentPath);
	CreateLogDir(ABL_szLogPath);
	sprintf(ABL_szLogPath, "%sLog\\%s", ABL_szCurrentPath, szSubPath);
	CreateLogDir(ABL_szLogPath);
	dwFileSize = GetLogFileByPathName(ABL_szLogPath, ABL_BaseLogFileName, szLogFile, bFileExist);
	bInitLogFileFlag = myLogFile.CreateAVFile(szLogFile, bFileExist, dwFileSize);
	return bInitLogFileFlag;
}

bool WriteLog(LogLevel nLogLevel, char* szSQL, ...)
{
	std::lock_guard<std::mutex> lock(WriteLogFileLock_Boost);
	if (!bInitLogFileFlag || nLogLevel < 0 || nLogLevel > 2)
		return false;

	SYSTEMTIME        st;
	GetLocalTime(&st);

	sprintf(ABL_writeBuffer, "%04d-%02d-%02d_%02d:%02d:%02d:%03d [%d] %s ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, ::GetCurrentThreadId(), ABL_LogStringArrray[nLogLevel]);
	va_list list;
	va_start(list, szSQL);
	vsprintf(ABL_szLogText, szSQL, list);
	va_end(list);
	strcat(ABL_szLogText, "\r\n");
	strcat(ABL_writeBuffer, ABL_szLogText);

	if (strlen(ABL_writeBuffer) < 512 && strstr(ABL_writeBuffer, "%") == NULL)
		printf(ABL_writeBuffer);

	if (strlen(ABL_writeBuffer) < 4096)
		myLogFile.WriteAVFile(ABL_writeBuffer, strlen(ABL_writeBuffer), true);

	return true;
}

bool  StopLogFile()
{
	std::lock_guard<std::mutex> lock(WriteLogFileLock_Boost);
	if (!bInitLogFileFlag)
		return false;
	myLogFile.CloseAVFile();
	bInitLogFileFlag = false;
	return true;
}

#endif