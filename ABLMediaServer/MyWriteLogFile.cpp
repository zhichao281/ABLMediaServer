#ifdef OS_System_Windows
#include <mutex>
#include "Stdafx.h"
#include "WriteAVFile.h"
#include "MyWriteLogFile.h"

char                           ABL_szCurrentPath[512] = { 0 };
char                           ABL_szLogPath[512] = { 0 };
CWriteAVFile                   myLogFile;
std::mutex                     WriteLogFileLock_Boost;
char*                          ABL_writeBuffer ;
char*                          ABL_szLogText;
bool                           bInitLogFileFlag = false;
char                           ABL_BaseLogFileName[256] = { 0 };
int                            ABL_MaxLogFileCount = 10;//最大保留日志文件个数

//升序模式
class MyLogFileSortASC
{
public:
	string szFilePath;
	string szCreateTime;
	bool operator < (const MyLogFileSortASC& rhs)
	{
		return szFilePath < rhs.szFilePath;
	}
};

typedef list<MyLogFileSortASC> LogFileList;
LogFileList   logFileList;

//获取当前路径
bool GetCurrentPath(char *szCurPath)
{
	char    szPath[255] = { 0 };
	string  strTemp;
	int     nPos;

	GetModuleFileName(NULL, szPath, sizeof(szPath));
	strTemp = szPath;

	nPos = strTemp.rfind("\\", strlen(szPath));
	if (nPos >= 0)
	{
		memcpy(szCurPath, szPath, nPos + 1);
		return true;
	}
	else
		return false;
}

//创建日志路径
bool CreateLogDir(char* szPath)
{
	ABL_writeBuffer = new char[OneLineLogStringMaxLength];
	ABL_szLogText = new char[OneLineLogStringMaxLength];
	::CreateDirectory(szPath, NULL);

	return true;
}

/*
功能： 根据路径，日志名称，获取到最后一个日志
参数
char* szPath,
char* szLogFileName,
char* szOutFileName,
bool& bFileExist        文件是否存在
*/
unsigned long  GetLogFileByPathName(char* szPath, char* szLogFileName, char* szOutFileName, bool& bFileExist)
{
	LogFileVector logFileVector;
	bool bFindFlag = true;
	char tempFileFind[MAX_PATH];
	char szLogFile[64] = { 0 };
	int  nMaxCount = 1;
	unsigned long  nFileSize = 0;
	bFileExist = true;
	MyLogFileSortASC  logFileASC;

	//查找文件
	sprintf(tempFileFind, "%s\\%s", szPath, szLogFileName);

	WIN32_FIND_DATA fd = { 0 };
	int nFileCount = 0;
	HANDLE hFind = FindFirstFile(tempFileFind, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		strcpy(szLogFile, szLogFileName);
		szLogFile[strlen(szLogFile) - 5] = 0x00;
		sprintf(szOutFileName, "%s\\%s%010d.log", szPath, szLogFile, nMaxCount);
		bFileExist = false;
		return nFileSize;
	}
	memcpy(szLogFile, fd.cFileName + strlen(fd.cFileName) - 14, 10);
	logFileVector.push_back(atoi(szLogFile));

	logFileList.clear();
	logFileASC.szFilePath = fd.cFileName;
	logFileList.push_back(logFileASC);

	nFileCount++;
	while (bFindFlag)
	{
		bFindFlag = FindNextFile(hFind, &fd);
		if (bFindFlag)
		{
			memcpy(szLogFile, fd.cFileName + strlen(fd.cFileName) - 14, 10);
			logFileVector.push_back(atoi(szLogFile));
			nFileCount++;

			logFileASC.szFilePath = fd.cFileName;
			logFileList.push_back(logFileASC);
		}
	}
	FindClose(hFind);

	logFileVector.sort();
	nMaxCount = logFileVector.back();

	strcpy(szLogFile, szLogFileName);
	szLogFile[strlen(szLogFile) - 5] = 0x00;

	sprintf(szOutFileName, "%s\\%s%010d.log", szPath, szLogFile, nMaxCount);

	HANDLE hFileHandle = CreateFile(szOutFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFileHandle != INVALID_HANDLE_VALUE)
	{
		nFileSize = GetFileSize(hFileHandle, &nFileSize);
		CloseHandle(hFileHandle);

		//如果文件大于250兆，则再重新生成一个
		if (nFileSize >= MaxCsmFileByteCount)
		{
			sprintf(szOutFileName, "%s\\%s%010d.log", szPath, szLogFile, nMaxCount + 1);
			bFileExist = false;
		}
	}

	logFileList.sort();
	char szDeleteFile[256] = { 0 };
	while (logFileList.size() > ABL_MaxLogFileCount)
	{
		logFileASC = logFileList.front();
		logFileList.pop_front();

		sprintf(szDeleteFile, "%s\\%s", ABL_szLogPath, logFileASC.szFilePath.c_str());
		ABLDeleteFile(szDeleteFile);
	}

	return nFileSize;
}

#endif