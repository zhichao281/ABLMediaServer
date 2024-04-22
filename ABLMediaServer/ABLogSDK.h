/*
项目： 
Author 罗家兄弟
Date   2019-08-15
QQ     79941308
E-mail 79941308@qq.com
        
	   20019-08-15 1.0.2 版本
 
*/
#pragma  once 
#ifdef OS_System_Windows 


//Log 信息级别
enum  LogLevel
{
	Log_Debug = 0,   //用于调试
	Log_Title = 1,   //用于提示
	Log_Error = 2    //标识为错误
};

/*
 函数功能：
     初始日志库
 参数:
    char* szSubPath                子路径 ，即在当前路径下面的log路径里面 ，创建一个子路径 szSubPath  
    char* szBaseLogFile            日志文件名字，一定要使用这样的格式  "rtspURLServer_00*.log"  ，保存出来的日志文件为 "rtspURLServer_0000000000001.log" 、"rtspURLServer_0000000000002.log”、"rtspURLServer_0000000000003.log"
	int   nMaxSaveLogFileCount     日志文件最大保存数量，超过这个数量会自动回滚，即删除最老的日志文件。
说明：
    日志保存路径自动保存在日志dll所在路径的log路径的子路径szSubPath 里面。
*/
bool  StartLogFile(char* szSubPath,char* szBaseLogFile,int nMaxSaveLogFileCount);

/*
函数功能：
   写日志
参数:
    int   nLevel,      日志级别 
	                   enum  LogLevel
						{
							Log_Debug = 0,   //用于调试
							Log_Title = 1,   //用于提示
							Log_Error = 2    //标识为错误
						};
	char* szSQL, ...   日志内容，和 sprintf 格式一致
*/
bool  WriteLog(LogLevel nLogLevel, char* szSQL, ...);

/*
函数功能：
  关闭日志
参数:
  无参数
*/
bool  StopLogFile();



#endif