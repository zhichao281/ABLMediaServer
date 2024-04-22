#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <syslog.h>

#include <pthread.h>

#include "ABLogFile.h"

#define  LogMaxByteCount                   1024*1024*25                         //25兆
#define  BaseLogName                          "ABLMediaServer_"                //基础log名称 
#define  LogFilePath                             "./log/ABLMediaServer"          //Log 路径
#define  RetainMaxFileCount                10                                         //最大保留文件个数
#define  DeleteLogFileTimerSecond      700                                      //多长时间执行一次删除日志文件 单位 秒

#ifdef OS_System_Windows
extern BOOL GBK2UTF8(char *szGbk, char *szUtf8, int Len);
#else
extern int GB2312ToUTF8(char* szSrc, size_t iSrcLen, char* szDst, size_t iDstLen);
#endif

using namespace std;
typedef    vector<unsigned long> FileNameNumber ;
FileNameNumber                   fileNameNumber ;
bool                             bInitLogFlag = false ; 
pthread_mutex_t                  ABL_LogFileLock;
int                              ABL_nFileByteCount = 0 ;//当前操作文件的大小
unsigned long                    ABL_nFileNumber; //当前文件序号
FILE*                            ABL_fLogFile=NULL ; 
char                             ABL_wzLog[48000] = {0};
char                             ABL_LogBuffer[48000] = {0};
char                             ABL_PrintBuffer[48000] = {0};
pthread_t                        pThread_deleteLogFile ;
char                             szLogLevel[3][64]={"Log_Debug","Log_Title","Log_Error"};

int showAllFiles( const char * dir_name,bool& bExitingFlag,int& fileSize)
{
	umask(0);
	mkdir("./log",777);
	umask(0);
	mkdir(LogFilePath,777);

    fileNameNumber.clear() ;
	
	fileSize = 0 ;
	// check the parameter !
	if( NULL == dir_name )
	{
		cout<<" dir_name is null ! "<<endl;
		return -1;
	}
 
	// check if dir_name is a valid dir
	struct stat s;
	lstat( dir_name , &s );
	if( ! S_ISDIR( s.st_mode ) )
	{
		cout<<"dir_name is not a valid directory !"<<endl;
		return -1;
	}
	
	struct dirent * filename;    // return value for readdir()
 	DIR * dir;                   // return value for opendir()
	dir = opendir( dir_name );
	if( NULL == dir )
	{
		cout<<"Can not open dir "<<dir_name<<endl;
		return -1;
	}
	cout<<"Successfully opened the dir !"<<endl;
	
    int nPos ;
	char  szFileNumber[64] ;
	/* read all the files in the dir ~ */
	while( ( filename = readdir(dir) ) != NULL )
	{
		// get rid of "." and ".."
		if( strcmp( filename->d_name , "." ) == 0 || 
			strcmp( filename->d_name , "..") == 0    )
			continue;
		if(strstr(filename ->d_name,BaseLogName) != NULL)
		{
           string strFileName = filename ->d_name ;
		   nPos = strFileName.find("_",0) ;
		   if(nPos >= 0)
		   {
			  memset(szFileNumber,0x00,sizeof(szFileNumber));
			  memcpy(szFileNumber,filename->d_name+nPos+1,strlen(filename->d_name)-nPos-5) ;
			  fileNameNumber.push_back(atoi(szFileNumber));
			  cout<<szFileNumber<< "  " <<endl ;
		   }
		}
	}
	closedir(dir);
	
	if(fileNameNumber.size() == 0)
	{
		bExitingFlag = false ;
		return 1 ;
	}else
	{
  	  sort(fileNameNumber.begin(),fileNameNumber.end(),greater<int>());
	  cout<< "vector Siize = " << fileNameNumber.size() <<  "  " << fileNameNumber[0] << endl ;
 	  //printf("当前文件 XHRTSPClient_%010d \r\n",fileNameNumber[0]) ;
	  char szFileName[64] = {0};
	  sprintf(szFileName,"%s/%s%010d.log",LogFilePath,BaseLogName,fileNameNumber[0]);

	  if(fileNameNumber.size() > RetainMaxFileCount)
	  {//删除老日志文件
	     char szDelFile[256] = {0};
		 for(int i= RetainMaxFileCount;i<fileNameNumber.size();i++)
		 {
	        sprintf(szDelFile,"%s/%s%010d.log",LogFilePath,BaseLogName,fileNameNumber[i]);
			unlink(szDelFile) ;
		 }
	  }
	 
	  struct stat statbuf;
      if(stat(szFileName,&statbuf)==0)
	  {
	     cout<< "File Siize = " << statbuf.st_size << endl ;
         fileSize = statbuf.st_size ;
		 
         if( statbuf.st_size >= LogMaxByteCount )
		 {//文件大于120兆
	        bExitingFlag = false ;
			return fileNameNumber[0] + 1;
		 }else
		 {
	        bExitingFlag = true  ;
			return fileNameNumber[0] ;
		 }
	  }else
	  {
	      bExitingFlag = false ;
		  return fileNameNumber[0];
	  }
	}
} 

void* DeleteLogFileThread(void* lpVoid)
{
	int nSleepCount = 0;
	bool bExiting ;
	char szFileName[256] ={0};
    int  nFileByteCount ;
	
	while(bInitLogFlag)
	{
		if(nSleepCount >=  DeleteLogFileTimerSecond)
		{
			nSleepCount = 0;
	        showAllFiles( LogFilePath ,bExiting,nFileByteCount);
		}
		
		nSleepCount ++ ;
		sleep(5) ;
	}
}

bool  InitLogFile() 
{
	if(bInitLogFlag == false) 
	{
	    pthread_mutex_init(&ABL_LogFileLock,NULL);	
		
 		pthread_mutex_lock(&ABL_LogFileLock);
		bool bExiting ;
		char szFileName[256] ={0};
		ABL_nFileNumber = showAllFiles( LogFilePath ,bExiting,ABL_nFileByteCount);
		sprintf(szFileName,"%s/%s%010d.log",LogFilePath,BaseLogName,ABL_nFileNumber);
		if(bExiting)
		{//文件已经存在
			ABL_fLogFile = fopen(szFileName,"a");
		}else
		{
			ABL_fLogFile = fopen(szFileName,"w");
		}
		
		bInitLogFlag = true ;
		pthread_create(&pThread_deleteLogFile,NULL,DeleteLogFileThread,(void*)NULL) ;
		
	    pthread_mutex_unlock(&ABL_LogFileLock);
		
		return true ;
	}else
		return false ;
}

bool WriteLog(LogLevel nLogLevel,const char* ms, ... )
{
	if(bInitLogFlag == true && nLogLevel >= 0 && nLogLevel <= 2 ) 
	{
 		pthread_mutex_lock(&ABL_LogFileLock);
		
		//先检查原来的大小
		if(ABL_nFileByteCount >= LogMaxByteCount && ABL_fLogFile != NULL )
		{
			fclose(ABL_fLogFile) ;
			
			char szFileName[256] ={0};
			ABL_nFileNumber ++ ; //文件序号递增一个
 		    sprintf(szFileName,"%s/%s%010d.log",LogFilePath,BaseLogName,ABL_nFileNumber);
	 		ABL_fLogFile = fopen(szFileName,"w");
			ABL_nFileByteCount = 0; //大小重新开始计算
		}
		
		va_list args;
		va_start(args, ms);
		vsprintf( ABL_wzLog ,ms,args);
		va_end(args);
	 
		time_t now;
		time(&now);
		struct tm *local;
		local = localtime(&now);
 
		sprintf(ABL_LogBuffer,"%04d-%02d-%02d %02d:%02d:%02d %s %s\n", local->tm_year+1900, local->tm_mon+1,
					local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,szLogLevel[nLogLevel],
					ABL_wzLog);
 		
		memset(ABL_PrintBuffer,0x00,sizeof(ABL_PrintBuffer));
		GB2312ToUTF8(ABL_LogBuffer, strlen(ABL_LogBuffer), ABL_PrintBuffer, sizeof(ABL_PrintBuffer));
		if (strlen(ABL_PrintBuffer) < 1024 && strstr(ABL_PrintBuffer, "%") == NULL)
			printf(ABL_PrintBuffer);
		
	     if(ABL_fLogFile != NULL)				
		 {
		   fwrite(ABL_LogBuffer,1,strlen(ABL_LogBuffer),ABL_fLogFile);
		   fflush(ABL_fLogFile);
		   
		   ABL_nFileByteCount += strlen(ABL_LogBuffer) ;
		 }
	    pthread_mutex_unlock(&ABL_LogFileLock);
		return true ;
	}else
		return false ;	
 }

bool  ExitLogFile() 
{
	if(bInitLogFlag == true) 
	{
 		pthread_mutex_lock(&ABL_LogFileLock);
	    bInitLogFlag = false ;
		
		if(ABL_fLogFile != NULL)
		{
			fclose(ABL_fLogFile) ;
			ABL_fLogFile = NULL ;
		}
		
	    pthread_mutex_unlock(&ABL_LogFileLock);
		return true ;
	}else
		return false ;
}
 