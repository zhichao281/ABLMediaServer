#pragma once

#include <memory>
#include <string>
#include <algorithm>
#include "ffmpegTypes.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/fmt.h> // ���� fmt ���ͷ�ļ�
namespace  spdlog
{
	class  SPDLOG
	{
	private:
		SPDLOG() = default;
		~SPDLOG() {
			spdlog::shutdown();   //loggerʹ����ɺ�Ҫִ��shutdown��������ѭ������ͬһ���͵�logger
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
		// ��ʼ��һ��Ĭ����־�ļ�log_file_path: ��־·����logger name; ��־�ȼ���������־�ļ�����С���ع���־�ļ���������־�Ƿ��̰߳�ȫ��
		//void init(std::string log_file_path, std::string logger_name, level::level_enum log_level= level::level_enum::info, size_t max_file_size = 1024 * 1024 * 10, size_t max_files=10, bool mt_security = true);
		
		void init(std::string log_file_path, std::string logger_name, std::string level = "I", size_t max_file_size = 1024 * 1024 * 10, size_t max_files = 10, bool mt_security = true);

		std::shared_ptr<spdlog::logger> logger() { return m_logger_ptr; }
	}; // SPDLOG class



} // Log namespace

#define  LOG_INIT( log_file_path, logger_name,  level,  max_file_size,  max_files,  mt_security)  spdlog::SPDLOG::getInstance().init(log_file_path, logger_name,  level,  max_file_size,  max_files,  mt_security)

#define  spdlogptr spdlog::SPDLOG::getInstance().logger()

