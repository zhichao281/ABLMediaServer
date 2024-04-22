// WriteAVFile.cpp: implementation of the CWriteAVFile class.
//
//////////////////////////////////////////////////////////////////////
#ifdef OS_System_Windows
#include "stdafx.h"
#include "WriteAVFile.h"
#include "MyWriteLogFile.h"

extern char                           ABL_szCurrentPath[512]  ;
extern char                           ABL_szLogPath[256] ;
extern char                           ABL_BaseLogFileName[256];
extern unsigned long  GetLogFileByPathName(char* szPath, char* szLogFileName, char* szOutFileName, bool& bFileExist);

CWriteAVFile::CWriteAVFile()
{
	 bOpenFlag = false;        //打开文件的标志
	 hWriteHandle = NULL  ;     //文件句柄
	 szCacheAVBuffer = new char[OneWriteDiskMaxLength];
	 InitializeCriticalSection(&file_CriticalSection);
	 memset(m_szFileName, 0x00, sizeof(m_szFileName));
}

CWriteAVFile::~CWriteAVFile()
{
      CloseAVFile() ;
	  DeleteCriticalSection(&file_CriticalSection);
	  delete[] szCacheAVBuffer;
	  malloc_trim(0);
}

//创建文件
bool CWriteAVFile::CreateAVFile(char* szFileName, bool bFileExist, unsigned long nFileSize)
{
	unsigned long dwWrite ;

    if(bOpenFlag)
	  return false;

	if (bFileExist == false)
	{//创建文件
		hWriteHandle = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hWriteHandle == INVALID_HANDLE_VALUE)
			return false;
	}
	else
	{//原文件已经存在
		hWriteHandle = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hWriteHandle == INVALID_HANDLE_VALUE)
			return false;
		::SetFilePointer(hWriteHandle, NULL, NULL, FILE_END);
	}

	//统计字节总数，不能超过1G
	nWriteByteCount = nFileSize;

    nCacheAVLength = 0 ;
	bOpenFlag = true;
	if (strlen(m_szFileName) == 0)
	 strcpy(m_szFileName, szFileName);

	return true;
}

//写入媒体内容
bool  CWriteAVFile::WriteAVFile(char* szMediaData, int nLength, bool bFlastWriteFlag)
{
	unsigned long dwWrite ;
	unsigned long dwFileSize = 0;
     if(!bOpenFlag || nLength <= 0 || szMediaData == NULL)
         return false;

	 EnterCriticalSection(&file_CriticalSection);

	 if (bFlastWriteFlag)
	 {//立刻写入
		 WriteFile(hWriteHandle, szMediaData, nLength, &dwWrite, NULL);
		 nWriteByteCount += nLength;
	 }
	 else
	 {//先缓存再写入
		 if (OneWriteDiskMaxLength - nCacheAVLength < nLength)
		 {//缓冲剩余的空间不够保存该帧
			 if (nCacheAVLength > 0)
				 WriteFile(hWriteHandle, szCacheAVBuffer, nCacheAVLength, &dwWrite, NULL);

			 //总计字节数，不能超过1G
			 nWriteByteCount += nCacheAVLength;

			 nCacheAVLength = 0;
		 }

		 memcpy(szCacheAVBuffer + nCacheAVLength, szMediaData, nLength);
		 nCacheAVLength += nLength;
	 }
	 LeaveCriticalSection(&file_CriticalSection);

	 //如果超过1G字节，则自行关闭
	 if (nWriteByteCount >= MaxCsmFileByteCount)
	 {
		 CloseAVFile();

		 //获取日志名字
		 dwFileSize = GetLogFileByPathName(ABL_szLogPath, ABL_BaseLogFileName, m_szFileName, bFileExist);

		 CreateAVFile(m_szFileName, bFileExist, dwFileSize);//重新生成新文件
	 }

	 return true;
}

//关闭文件
void  CWriteAVFile::CloseAVFile() 
{
	unsigned long dwWrite ;

	if(bOpenFlag)
	{
       EnterCriticalSection(&file_CriticalSection);
		bOpenFlag = false;
	
	    if(nCacheAVLength > 0)
  		  WriteFile(hWriteHandle,szCacheAVBuffer,nCacheAVLength,&dwWrite,NULL) ;
	   
		nCacheAVLength = 0;
		CloseHandle(hWriteHandle) ;
 	   LeaveCriticalSection(&file_CriticalSection);

	}
}
#endif