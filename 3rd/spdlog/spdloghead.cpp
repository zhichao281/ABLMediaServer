#include "spdloghead.h"
#include <iostream>

#ifdef USE_GHC
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#endif

#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace spdlog
{
	SPDLOG::~SPDLOG() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_loggers.clear();
		spdlog::shutdown();
	}

	SPDLOG& SPDLOG::getInstance() {
		static SPDLOG instance;
		return instance;
	}

	void SPDLOG::init(
		const std::string& log_file_path,
		const std::string& logger_name,
		const std::string& level,
		size_t max_file_size,
		size_t max_files,
		bool mt_security)
	{
		std::string key =logger_name;

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_loggers.find(key) != m_loggers.end()) {
				return; // 已初始化
			}
		}

		try
		{
			fs::path fs_log_file_path = fs::absolute(log_file_path).parent_path();
			if (!fs::exists(fs_log_file_path)) {
				fs::create_directories(fs_log_file_path);
			}

			std::shared_ptr<spdlog::logger> logger;
			if (mt_security) {
				auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file_path, max_file_size, max_files);
				auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
				file_sink->set_level(spdlog::level::trace);
				file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");
				console_sink->set_level(spdlog::level::trace);
				console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");

				std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
				logger = std::make_shared<spdlog::logger>(key, sinks.begin(), sinks.end());
			}
			else {
				auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(log_file_path, max_file_size, max_files);
				auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
				file_sink->set_level(spdlog::level::trace);
				file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");
				console_sink->set_level(spdlog::level::trace);
				console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v");

				std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
				logger = std::make_shared<spdlog::logger>(key, sinks.begin(), sinks.end());
			}

			// 先注册logger和更新成员变量（加锁）
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				spdlog::register_logger(logger);
				m_loggers[key] = logger;
				m_bInit = true;
			
			}

			// setLogLevel放到锁外，避免递归加锁
			setLogLevel(level, key);
		}
		catch (const spdlog::spdlog_ex& ex) {
			std::cout << "Log initialization failed: " << std::string(ex.what()) << std::endl;
		}
	}

	// 无参：返回共享logger（或唯一logger），有参：返回指定logger
	std::shared_ptr<spdlog::logger> SPDLOG::logger()
	{
		std::lock_guard<std::mutex> lock(m_mutex);		
		// 非共享时返回第一个logger（兼容用法）
		if (!m_loggers.empty()) return m_loggers.begin()->second;
		return nullptr;
	}

	std::shared_ptr<spdlog::logger> SPDLOG::logger(const std::string& logger_name)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_loggers.find(logger_name);
		if (it != m_loggers.end()) {
			return it->second;
		}
		
		return nullptr;
	}

	void SPDLOG::setLogLevel(const std::string& level, const std::string& logger_name)
	{
		auto log = logger(logger_name);
		if (!log) return;
		char L = toupper(level[0]);
		if (L == 'T') {
			log->set_level(spdlog::level::trace);
			log->flush_on(spdlog::level::trace);
		}
		else if (L == 'D') {
			log->set_level(spdlog::level::debug);
			log->flush_on(spdlog::level::debug);
		}
		else if (L == 'I') {
			log->set_level(spdlog::level::info);
			log->flush_on(spdlog::level::info);
		}
		else if (L == 'W') {
			log->set_level(spdlog::level::warn);
			log->flush_on(spdlog::level::warn);
		}
		else if (L == 'E') {
			log->set_level(spdlog::level::err);
			log->flush_on(spdlog::level::err);
		}
		else if (L == 'C') {
			log->set_level(spdlog::level::critical);
			log->flush_on(spdlog::level::critical);
		}
		else {
			std::cout << "level set error " << level << std::endl;
		}
	}

	void SPDLOG::setLogLevel(level::level_enum log_level, const std::string& logger_name)
	{
		auto log = logger(logger_name);
		if (!log) return;
		log->set_level(log_level);
		log->flush_on(log_level);
	}
}