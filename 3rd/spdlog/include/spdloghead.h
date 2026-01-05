#pragma once

// 导出/导入宏定义
#ifdef _WIN32
#  ifdef SPDLOGHEAD_EXPORTS
#    define SPDLOGHEAD_API __declspec(dllexport)
#  else
#    define SPDLOGHEAD_API __declspec(dllimport)
#  endif
#  define SPDLOGHEAD_APICALL __stdcall
#else
#  define SPDLOGHEAD_API __attribute__((visibility("default")))
#  define SPDLOGHEAD_APICALL
#endif

#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <cstddef>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/fmt.h>

namespace spdlog
{
	class SPDLOGHEAD_API SPDLOG
	{
	private:
		SPDLOG() = default;
		~SPDLOG() noexcept;

		SPDLOG(const SPDLOG&) = delete;
		SPDLOG& operator=(const SPDLOG&) = delete;

		void setLogLevel(const std::string& level, const std::string& logger_name);
		void setLogLevel(level::level_enum log_level, const std::string& logger_name);

	public:
		static SPDLOG& getInstance();

		// 初始化一个默认日志文件
		// log_file_path: 日志路径
		// logger_name: 日志器名称
		// level: 日志等级（字符串）
		// max_file_size: 单个日志文件最大大小
		// max_files: 回滚日志文件个数
		// mt_security: 日志是否线程安全
		void init(
			const std::string& log_file_path,
			const std::string& logger_name,
			const std::string& level = "I",
			size_t max_file_size = 1024 * 1024 * 5,
			size_t max_files = 5,
			bool mt_security = true);

		// 无参：返回共享logger，有参：返回指定logger
		std::shared_ptr<spdlog::logger> logger();
		std::shared_ptr<spdlog::logger> logger(const std::string& logger_name);

	private:
		std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;
		std::mutex m_mutex;
		std::atomic<bool> m_bInit{ false };
	};
} // namespace spdlog

// 全项目通用日志（共享 logger）
#define LOG_INIT(log_file_path, logger_name, level, max_file_size, max_files, mt_security) \
    spdlog::SPDLOG::getInstance().init(log_file_path, logger_name, level, max_file_size, max_files, mt_security)

// 共享日志时可直接用 spdlogptr
#define spdlogptr spdlog::SPDLOG::getInstance().logger()

#define spdlogptrbyname(logger_name) spdlog::SPDLOG::getInstance().logger(logger_name)

//spdlogptr("libA")->debug("调试信息: {}", 42);
//spdlogptr("libA")->critical("严重错误: {}", "崩溃");