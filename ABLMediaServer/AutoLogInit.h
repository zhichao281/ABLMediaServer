#pragma once
#include "spdloghead.h"
#include <string>
enum  LogLevel
{
	Log_Debug = 0,   //用于调试
	Log_Title = 1,   //用于提示
	Log_Error = 2    //标识为错误
};


// 全局 logger_name，供日志宏使用
extern std::string g_logger_name;
// 日志自动初始化类
class AutoLogInit {
public:
	// 构造时初始化日志
	AutoLogInit();
};

// 推荐日志宏，自动使用当前 g_logger_name
#define XLOG_TRACE(...)  SPDLOG_LOGGER_CALL(spdlogptrbyname(g_logger_name).get(), spdlog::level::trace, __VA_ARGS__)
#define XLOG_DEBUG(...)  SPDLOG_LOGGER_CALL(spdlogptrbyname(g_logger_name).get(), spdlog::level::debug, __VA_ARGS__)
#define XLOG_INFO(...)   SPDLOG_LOGGER_CALL(spdlogptrbyname(g_logger_name).get(), spdlog::level::info, __VA_ARGS__)
#define XLOG_WARN(...)   SPDLOG_LOGGER_CALL(spdlogptrbyname(g_logger_name).get(), spdlog::level::warn, __VA_ARGS__)
#define XLOG_ERROR(...)  SPDLOG_LOGGER_CALL(spdlogptrbyname(g_logger_name).get(), spdlog::level::err, __VA_ARGS__)

// 可选：支持指定logger
#define XLOG_TRACE_BYNAME(logger_name, ...)  SPDLOG_LOGGER_CALL(spdlogptrbyname(logger_name).get(), spdlog::level::trace, __VA_ARGS__)
#define XLOG_DEBUG_BYNAME(logger_name, ...)  SPDLOG_LOGGER_CALL(spdlogptrbyname(logger_name).get(), spdlog::level::debug, __VA_ARGS__)
#define XLOG_INFO_BYNAME(logger_name, ...)   SPDLOG_LOGGER_CALL(spdlogptrbyname(logger_name).get(), spdlog::level::info, __VA_ARGS__)
#define XLOG_WARN_BYNAME(logger_name, ...)   SPDLOG_LOGGER_CALL(spdlogptrbyname(logger_name).get(), spdlog::level::warn, __VA_ARGS__)
#define XLOG_ERROR_BYNAME(logger_name, ...)  SPDLOG_LOGGER_CALL(spdlogptrbyname(logger_name).get(), spdlog::level::err, __VA_ARGS__)


#define WriteLog(level, fmt, ...) \
    do { \
        char buf[2048]; \
        snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
        if ((level) == Log_Debug) { XLOG_DEBUG("{}", buf); } \
        else if ((level) == Log_Title) { XLOG_INFO("{}", buf); } \
        else if ((level) == Log_Error) { XLOG_ERROR("{}", buf); } \
        else { XLOG_INFO("{}", buf); } \
    } while(0)

#ifdef _WIN32
#pragma comment(lib, "spdloghead.lib")
#endif