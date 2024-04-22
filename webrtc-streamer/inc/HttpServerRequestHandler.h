/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** HttpServerRequestHandler.h
** 
** -------------------------------------------------------------------------*/

#pragma once

#include <list>
#include <map>
#include <functional>


#include "rtc_base/strings/json.h"
#include "../civetweb/include/CivetServer.h"


/* ---------------------------------------------------------------------------
**  http callback
** -------------------------------------------------------------------------*/
class HttpServerRequestHandler : public CivetServer
{
	public:
		typedef std::tuple<int, std::map<std::string,std::string>,Json::Value> httpFunctionReturn;
		typedef std::function<httpFunctionReturn(const struct mg_request_info *req_info, const Json::Value &)> httpFunction;
	
		HttpServerRequestHandler(std::map<std::string,httpFunction>& func, const std::vector<std::string>& options); 
		virtual ~HttpServerRequestHandler();

	private:
	
		std::vector<CivetHandler*> m_handlers;
};

#include <string>
#include "rtc_base/logging.h"
#include <vector>
#include <iostream>
#include <ctime>

#ifdef WEBRTC_LINUX
#include <sys/stat.h>
#include <stdio.h>
#include <sys/stat.h> 　
#include <sys/types.h>
#include <unistd.h>
#else
#include <direct.h>
#include <io.h>
#endif

#define MAX_LOG_FILE_SIZE (1024*1024)


class FileLog : public rtc::LogSink {
public:
	FileLog(const std::string& LogPath)
		:logfile_(NULL),
		log_path_(LogPath) {
		
#ifdef WEBRTC_LINUX
		if (access(LogPath.c_str(), F_OK) == -1)
		{
			int isCreate = mkdir(LogPath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
#else
		if (_access(LogPath.c_str(), 0) == -1)
		{
			int isCreate = _mkdir(LogPath.c_str());
#endif
			if (!isCreate)
				printf("create path:%s\n", LogPath.c_str());
			else
				printf("create path failed! error code : %s \n", isCreate, LogPath.c_str());
		}
		
	}

	virtual ~FileLog() {
		if (logfile_) {
			fclose(logfile_);
			logfile_ = NULL;
		}
	}

	inline void FileDate() {
		std::string timeString = getDatetime("yyyy-MM-dd_hhmmss");
		logfileName_ = log_path_ + "/webrtc_" + timeString + ".log";
	}

	inline size_t Size()
	{
		size_t size = 0;
		if (logfile_ != NULL) 
		{
#ifdef WEBRTC_LINUX
			struct stat fileStat;
			if (-1 == fstat(fileno(logfile_), &fileStat))
			{
				return 0;
			}
			// deal returns.
			return fileStat.st_size;
#else

			size = _filelength(_fileno(logfile_));
#endif
		

		}
		return size;
	}

	inline void Start(void)
	{
		if (NULL == logfile_) {
			FileDate();
			logfile_ = fopen(logfileName_.c_str(), "w");
		}
		else if (Size() > MAX_LOG_FILE_SIZE)
		{
			Close();
			FileDate();
			logfile_ = fopen(logfileName_.c_str(), "w");
		}
	}

	inline void Close(void)
	{
		if (logfile_) {
			fclose(logfile_);
			logfile_ = NULL;
		}
	}


	static std::string getDatetime(std::string strType)
	{
		time_t now = time(0);// 基于当前系统的当前日期/时间
		tm* ltm = localtime(&now);
		char szDateTime[128];
		char iyear[50], imonth[50], iday[50], ihour[50], imin[50], isec[50];
		sprintf(iyear, "%d", 1900 + ltm->tm_year);
		sprintf(imonth, "%02d", 1 + ltm->tm_mon);
		sprintf(iday, "%02d", ltm->tm_mday);
		sprintf(ihour, "%02d", ltm->tm_hour);
		sprintf(imin, "%02d", ltm->tm_min);
		sprintf(isec, "%02d", ltm->tm_sec);
		if (strType == "yyyy-MM-dd hh:mm:ss.zzz")
		{
			sprintf(szDateTime, "%s-%s-%s %s:%s:%s", iyear, imonth, iday, ihour, imin, isec);
		}
		if ("yyyy-MM-dd_hhmmss" == strType)
		{
			sprintf(szDateTime, "%s-%s-%s_%s%s%s", iyear, imonth, iday, ihour, imin, isec);
		}


		return std::string(szDateTime);
	}


	virtual void OnLogMessage(const std::string& message) 
	{

		Start();
		if (NULL == logfile_)
			return;		
		std::string timeString = getDatetime("yyyy-MM-dd hh:mm:ss.zzz");
		std::string msgString;
		msgString = timeString+"    " + message;
		if (fwrite(msgString.c_str(), 1, msgString.length(), logfile_) < 0) {
			Close();
		}
		else if (fflush(logfile_) < 0) {
			Close();
		}
	}

private:

	FILE* logfile_;
	const std::string log_path_;
	std::string logfileName_;



};