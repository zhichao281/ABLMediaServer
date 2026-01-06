#include "spdloghead.h"
#include <iostream>
#include <cctype>

#ifdef USE_GHC
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace spdlog
{
	namespace
	{
		constexpr const char* kPattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t][%s %!:%#] %v";

		inline void configure_sink(const spdlog::sink_ptr& sink)
		{
			sink->set_level(spdlog::level::trace);
			sink->set_pattern(kPattern);
		}

		inline spdlog::level::level_enum parse_level(const std::string& levelStr, bool& ok)
		{
			ok = true;
			if (levelStr.empty())
			{
				ok = false;
				return spdlog::level::info;
			}

			const unsigned char c0 = static_cast<unsigned char>(levelStr[0]);
			const char L = static_cast<char>(std::toupper(c0));

			switch (L)
			{
			case 'T': return spdlog::level::trace;
			case 'D': return spdlog::level::debug;
			case 'I': return spdlog::level::info;
			case 'W': return spdlog::level::warn;
			case 'E': return spdlog::level::err;
			case 'C': return spdlog::level::critical;
			default:
				ok = false;
				return spdlog::level::info;
			}
		}
	}

	SPDLOG::~SPDLOG() noexcept
	{
		// 避免影响进程内其它模块对 spdlog 的使用：不调用 spdlog::shutdown()
		std::lock_guard<std::mutex> lock(m_mutex);
		for (const auto& kv : m_loggers)
		{
			// 仅 drop 自己注册过的 logger
			spdlog::drop(kv.first);
		}
		m_loggers.clear();
	}

	 SPDLOG& SPDLOG::getInstance()
	{
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
		const std::string& key = logger_name;

		// 快速路径：已存在直接返回
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_loggers.find(key) != m_loggers.end())
			{
				return;
			}
		}

		try
		{
			fs::create_directories(fs::absolute(log_file_path).parent_path());

			std::shared_ptr<spdlog::logger> newLogger;

			if (mt_security)
			{
				auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file_path, max_file_size, max_files);
				auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
				configure_sink(file_sink);
				configure_sink(console_sink);

				std::vector<spdlog::sink_ptr> sinks;
				sinks.reserve(2);
				sinks.emplace_back(std::move(console_sink));
				sinks.emplace_back(std::move(file_sink));

				newLogger = std::make_shared<spdlog::logger>(key, sinks.begin(), sinks.end());
			}
			else
			{
				auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(log_file_path, max_file_size, max_files);
				auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
				configure_sink(file_sink);
				configure_sink(console_sink);

				std::vector<spdlog::sink_ptr> sinks;
				sinks.reserve(2);
				sinks.emplace_back(std::move(console_sink));
				sinks.emplace_back(std::move(file_sink));

				newLogger = std::make_shared<spdlog::logger>(key, sinks.begin(), sinks.end());
			}

			// 注册 + 写入 map，必须在锁内，并二次确认避免并发重复注册
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				if (m_loggers.find(key) != m_loggers.end())
				{
					return;
				}

				spdlog::register_logger(newLogger);
				m_loggers.emplace(key, newLogger);
				m_bInit.store(true, std::memory_order_release);
			}

			// 放到锁外，避免递归加锁
			setLogLevel(level, key);
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			std::cout << "Log initialization failed: " << std::string(ex.what()) << std::endl;
		}
		catch (const std::exception& ex)
		{
			std::cout << "Log initialization failed: " << std::string(ex.what()) << std::endl;
		}
	}

	std::shared_ptr<spdlog::logger> SPDLOG::logger()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_loggers.empty())
		{
			return m_loggers.begin()->second;
		}
		return nullptr;
	}

	std::shared_ptr<spdlog::logger> SPDLOG::logger(const std::string& logger_name)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_loggers.find(logger_name);
		if (it != m_loggers.end())
		{
			return it->second;
		}
		return nullptr;
	}

	void SPDLOG::setLogLevel(const std::string& level, const std::string& logger_name)
	{
		auto log = logger(logger_name);
		if (!log) return;

		bool ok = false;
		const auto lv = parse_level(level, ok);
		if (!ok)
		{
			std::cout << "level set error " << level << std::endl;
		}

		log->set_level(lv);
		log->flush_on(lv);
	}

	void SPDLOG::setLogLevel(level::level_enum log_level, const std::string& logger_name)
	{
		auto log = logger(logger_name);
		if (!log) return;
		log->set_level(log_level);
		log->flush_on(log_level);
	}
}