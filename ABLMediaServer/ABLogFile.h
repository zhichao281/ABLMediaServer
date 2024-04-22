#ifndef _ABLogFile_H
#define _ABLogFile_H

//Log 信息级别
enum  LogLevel
{
	Log_Debug = 0,   //用于调试
	Log_Title = 1,   //用于提示
	Log_Error = 2    //标识为错误
};

int   showAllFiles( const char * dir_name,bool& bExitingFlag,int& fileSize);
void* DeleteLogFileThread(void* lpVoid);
bool  InitLogFile() ;
bool  WriteLog(LogLevel nLogLevel,const char* ms, ... );
bool  ExitLogFile() ;

#endif

