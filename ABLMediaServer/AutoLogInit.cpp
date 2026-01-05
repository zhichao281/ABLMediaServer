#include "AutoLogInit.h"

// 全局 logger_name，供日志宏使用
std::string g_logger_name = "ABLMediaServer";

AutoLogInit auto_log_init; 

AutoLogInit::AutoLogInit() 
{
	LOG_INIT(
		"log/ABLMediaServer.log",
		g_logger_name,
		"D",
		1 * 1024 * 1024,
		3,
		true
	);


}