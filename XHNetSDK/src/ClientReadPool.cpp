/*
功能：
  实现对已经成功连接的tcp客户端进行读取、发送数据，支持同步 、异步发送，只支持异步读取

日期    2025-07-09
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "ClientReadPool.h"
#include "client_manager.h"
#include "ClientSendPool.h"
#include <fstream>
#include <sstream>


#ifdef USE_GHC
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif


extern CClientSendPool* clientSendPool;

CClientReadPool::CClientReadPool(int nThreadCount)
{
	nThreadProcessCount = 0;
	nTrueNetThreadPoolCount = nThreadCount / 2;
	if (nThreadCount > MaxNetHandleQueueCount)
		nTrueNetThreadPoolCount = MaxNetHandleQueueCount;

	if (nThreadCount <= 0)
		nTrueNetThreadPoolCount = 64;

	nGetCurrentThreadOrder = 0;
	unsigned long dwThread;
	bRunFlag.store(true);
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
#ifdef OS_System_Windows
		hProcessHandle[i] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else
		pthread_create(&hProcessHandle[i], NULL, OnProcessThread, (void*)this);
#endif
	}
}

int   CClientReadPool::GetThreadOrder()
{
	std::lock_guard<std::mutex> lock(threadLock);
	int nGet = nGetCurrentThreadOrder;
	nGetCurrentThreadOrder++;
	return nGet;
}

CClientReadPool::~CClientReadPool()
{

}

void  CClientReadPool::destroyReadPool()
{
	bRunFlag.store(false);
	int i;
	std::lock_guard<std::mutex> lock(threadLock);

	for (i = 0; i < nTrueNetThreadPoolCount; i++)
	{
#ifdef OS_System_Windows		
		epoll_close(epfd[i]);
#else
		close(epfd[i]);
#endif
		while (!bExitProcessThreadFlag[i].load())
			Sleep(50);

#ifdef  OS_System_Windows
		CloseHandle(hProcessHandle[i]);
#endif 
	}
}

void CClientReadPool::ProcessFunc()
{
	int nCurrentThreadID = GetThreadOrder();
	bExitProcessThreadFlag[nCurrentThreadID].store(false);
	int ret_num, i, ret_recv;
	unsigned char* szRecvData = new unsigned char[1024 * 1024 * 2];

	// 创建epoll句柄
	epfd[nCurrentThreadID] = epoll_create(1);

	while (bRunFlag.load())
	{
		ret_num = epoll_wait(epfd[nCurrentThreadID], events[nCurrentThreadID], MaxEventCount, 1000);
		if (ret_num <= 0) {
			Sleep(2);
			continue;
		}

		for (i = 0; i < ret_num; ++i)
		{
#ifdef USE_BOOST
			client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(events[nCurrentThreadID][i].data.u64);
#else
			client_ptr cli = client_manager::get_instance().get_client(events[nCurrentThreadID][i].data.u64);
#endif
			if (!cli)
				continue;

			ret_recv = ::recv(cli->m_Socket, (char*)szRecvData, 1024 * 1024 * 2, 0);
			if (ret_recv > 0)
			{
				if (ret_recv < 1024 * 1024 * 2)
					szRecvData[ret_recv] = 0x00;

				// 只在可能为HTTP请求时构造字符串
				if (ret_recv >= 4 &&
					(memcmp(szRecvData, "GET ", 4) == 0 || memcmp(szRecvData, "POST", 4) == 0))
				{
					std::string reqStr((char*)szRecvData, ret_recv);

					size_t line_end = reqStr.find("\r\n");
					std::string first_line = (line_end != std::string::npos) ? reqStr.substr(0, line_end) : reqStr;
					std::istringstream iss(first_line);
					std::string method, url, version;
					iss >> method >> url >> version;

					// 静态资源判断
					static const std::vector<std::string> mime_suffix = {
						".html", ".htm", ".css", ".js", ".json", ".png", ".jpg", ".jpeg", ".gif", ".txt", ".ico"
					};
					bool is_static = (url == "/");
					for (const auto& suffix : mime_suffix) {
						if (url.size() >= suffix.size() &&
							url.compare(url.size() - suffix.size(), suffix.size(), suffix) == 0) {
							is_static = true;
							break;
						}
					}
					if (is_static) {
						printf("handle_http_request: %s \r\n ", reqStr.c_str());
						handle_http_request(cli, reqStr);
						continue;
					}
				}
				// 非HTTP或非静态资源，走原有逻辑
				if (cli->m_fnread)
					cli->m_fnread(0, events[nCurrentThreadID][i].data.u64, szRecvData, static_cast<uint32_t>(ret_recv), (void*)&cli->tAddr4);
			}
			else
			{
#ifdef OS_System_Windows
				if (ret_recv == 0 || (ret_recv == SOCKET_ERROR && (WSAGetLastError() != EWOULDBLOCK && WSAGetLastError() != EAGAIN)))
#else 
				if (ret_recv == 0 || (ret_recv == -1 && (errno != EWOULDBLOCK && errno != EAGAIN)))
#endif
				{
					clientSendPool->DeleteFromTask(cli->nSendThreadOrder.load(), cli->get_id());
					DeleteFromTask(events[nCurrentThreadID][i].data.u64);
#ifdef USE_BOOST
					client_manager_singleton::get_mutable_instance().pop_client(cli->get_id());
#else
					client_manager::get_instance().pop_client(cli->get_id());
#endif
					if (cli->m_fnclose)
						cli->m_fnclose(cli->get_server_id(), cli->get_id());
				}
				// 继续下一个事件
			}
		}
	}
	delete[] szRecvData;
	szRecvData = nullptr;
	bExitProcessThreadFlag[nCurrentThreadID].store(true);
}

void* CClientReadPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
#endif

	CClientReadPool* pThread = (CClientReadPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //退出线程
#endif
	return  0;
}

bool CClientReadPool::InsertIntoTask(uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false;
	std::lock_guard<std::mutex> lock(threadLock);
	int                 nThreadThread = 0;
	int                 ret;
	static  int nAddCount = 0;

	nThreadThread = nThreadProcessCount.load() % nTrueNetThreadPoolCount;
	nThreadProcessCount.fetch_add(1);


#ifdef USE_BOOST
	client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(nClientID);
#else
	client_ptr cli = client_manager::get_instance().get_client(nClientID);
#endif
	if (cli)
	{
		struct epoll_event  event;//对event结构体进行属性填充
		event.events = EPOLLIN;
		event.data.u64 = cli->get_id();
		cli->nRecvThreadOrder.store(nThreadThread);
		ret = epoll_ctl(epfd[nThreadThread], EPOLL_CTL_ADD, cli->m_Socket, &event);
		if (ret == -1)
		{
			perror("epoll_ctl cfd");
			return false;
		}

		nAddCount++;
		//printf("=====================================  nAddCount = %d\r\n", nAddCount);
		return true;
	}

	return false;
}

//从线程池彻底移除 nClient 
bool CClientReadPool::DeleteFromTask(uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false;

	bool       bRet = false;
	struct epoll_event  event;//对event结构体进行属性填充

#ifdef USE_BOOST
	client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(nClientID);
#else
	client_ptr cli = client_manager::get_instance().get_client(nClientID);
#endif

	if (cli != NULL)
	{
		event.events = EPOLLIN;
		event.data.u64 = cli->get_id();
		epoll_ctl(epfd[cli->nRecvThreadOrder.load()], EPOLL_CTL_DEL, cli->m_Socket, &event);

		bRet = true;
	}
	else
		bRet = false;

	return bRet;
}

void CClientReadPool::handle_http_request(client_ptr cli,const std::string& request)
{
	// 只解析首行，避免多余字符串处理
	size_t line_end = request.find("\r\n");
	std::string first_line = (line_end != std::string::npos) ? request.substr(0, line_end) : request;
	std::istringstream iss(first_line);
	std::string method, url, version;
	iss >> method >> url >> version;

	// 只处理 GET/POST
	if (method != "GET" && method != "POST") {
		std::string not_allowed = "<h1>405 Method Not Allowed</h1>";
		std::ostringstream response;
		response << "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: " << not_allowed.size() << "\r\n\r\n" << not_allowed;
		cli->write((uint8_t*)response.str().data(), (uint32_t)response.str().size(), false);
		return;
	}

	// 路径和参数分离
	std::string file_url = url;
	std::string query;
	size_t pos = url.find('?');
	if (pos != std::string::npos) {
		file_url = url.substr(0, pos);
		query = url.substr(pos + 1);
	}

	// 默认首页
	if (file_url == "/") file_url = "webrtc_player.html";
	if (!file_url.empty() && file_url[0] == '/') file_url = file_url.substr(1);

	// 路径安全校验
	fs::path web_root_path = fs::absolute(m_web_root);
	fs::path file_path = web_root_path / fs::path(file_url);
	fs::path safe_path = fs::weakly_canonical(file_path);

	if (!fs::exists(safe_path) || safe_path.string().find(web_root_path.string()) != 0) {
		std::string not_found = "<h1>403 Forbidden</h1>";
		std::ostringstream response;
		response << "HTTP/1.1 403 Forbidden\r\nContent-Length: " << not_found.size() << "\r\n\r\n" << not_found;
		cli->write((uint8_t*)response.str().data(), (uint32_t)response.str().size(), false);
		return;
	}

	// MIME类型判断
	static const std::unordered_map<std::string, std::string> mime_map = {
		{".html", "text/html"}, {".htm", "text/html"}, {".css", "text/css"},
		{".js", "application/javascript"}, {".json", "application/json"},
		{".png", "image/png"}, {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
		{".gif", "image/gif"}, {".txt", "text/plain"}
	};
	std::string ext = safe_path.extension().string();
	std::string content_type = mime_map.count(ext) ? mime_map.at(ext) : "application/octet-stream";

	std::ifstream file(safe_path, std::ios::binary);
	std::ostringstream response;
	if (file) {
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		// 参数插入到 HTML
		if (!query.empty() && ext == ".html") {
			std::string js = "<script>var queryString='" + query + "';</script>";
			size_t head_pos = content.find("<head>");
			if (head_pos != std::string::npos) {
				content.insert(head_pos + 6, js);
			}
			else {
				content = js + content;
			}
		}
		response << "HTTP/1.1 200 OK\r\nContent-Type: " << content_type
			<< "\r\nContent-Length: " << content.size() << "\r\n\r\n" << content;
	}
	else {
		std::string not_found = "<h1>404 Not Found</h1>";
		response << "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: "
			<< not_found.size() << "\r\n\r\n" << not_found;
	}
	cli->write((uint8_t*)response.str().data(), (uint32_t)response.str().size(), false);
	return;
}
