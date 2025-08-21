#pragma once

#include <memory>
#include <string>
#include <algorithm>
#include "ffmpegTypes.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/fmt.h> // 包含 fmt 库的头文件
namespace  spdlog
{
	class  SPDLOG
	{
	private:
		SPDLOG() = default;
		~SPDLOG() {
			spdlog::shutdown();   //logger使用完成后，要执行shutdown，否则不能循环创建同一类型的logger
		}
		;
		SPDLOG(const SPDLOG&) = delete;
		SPDLOG& operator=(const SPDLOG&) = delete;

	private:
		std::shared_ptr<spdlog::logger> m_logger_ptr;

		void setLogLevel(const std::string& level);

		void setLogLevel(level::level_enum log_level);

	public:
		static SPDLOG& getInstance();
		// 初始化一个默认日志文件log_file_path: 日志路径；logger name; 日志等级；单个日志文件最大大小；回滚日志文件个数；日志是否线程安全；
		//void init(std::string log_file_path, std::string logger_name, level::level_enum log_level= level::level_enum::info, size_t max_file_size = 1024 * 1024 * 10, size_t max_files=10, bool mt_security = true);
		
		void init(std::string log_file_path, std::string logger_name, std::string level = "I", size_t max_file_size = 1024 * 1024 * 10, size_t max_files = 10, bool mt_security = true);

		std::shared_ptr<spdlog::logger> logger() { return m_logger_ptr; }
	}; // SPDLOG class



} // Log namespace

#define  LOG_INIT( log_file_path, logger_name,  level,  max_file_size,  max_files,  mt_security)  spdlog::SPDLOG::getInstance().init(log_file_path, logger_name,  level,  max_file_size,  max_files,  mt_security)

#define  spdlogptr spdlog::SPDLOG::getInstance().logger()

