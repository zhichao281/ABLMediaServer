// WriteAVFile.h: interface for the CWriteAVFile class.
//
//////////////////////////////////////////////////////////////////////
#ifdef OS_System_Windows
#pragma once
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define  MaxCsmFileByteCount    1024*1024*50  //50兆字节
#define  OneWriteDiskMaxLength  1024*1024*2   //每次积累2兆写入一次

class CWriteAVFile  
{
public:
	CWriteAVFile();
	virtual ~CWriteAVFile();

	bool    CreateAVFile(char* szFileName, bool bFileExist,unsigned long nFileSize);
	bool    WriteAVFile(char* szMediaData,int nLength, bool bFlastWriteFlag) ; //bWriteFlag true 则立刻写入，false缓存写
    void    CloseAVFile() ;

	CRITICAL_SECTION file_CriticalSection;
	unsigned long     nWriteByteCount ; //写入字节总数
	char    szFileName[255] ;
	bool    bOpenFlag ;        //打开文件的标志
	HANDLE  hWriteHandle ;     //文件句柄
	char*   szCacheAVBuffer ;  //音频视频缓冲
	int     nCacheAVLength ;   //写入缓冲的总长度
	char    m_szFileName[256]; //文件名字
	bool    bFileExist;
};

#endif
