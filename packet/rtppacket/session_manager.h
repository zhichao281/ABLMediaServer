#ifndef _RTP_PACKET_DEPACKET_SESSION_MANAGER_H_
#define _RTP_PACKET_DEPACKET_SESSION_MANAGER_H_


#include <stdint.h>
#include <unordered_map>
#include <mutex>

#include "session.h"


class rtp_session_manager
{
public:
	rtp_session_ptr malloc(rtp_packet_callback cb, void* userdata);
	void free(rtp_session_ptr p);

	bool push(rtp_session_ptr s);
	bool pop(uint32_t h);
	rtp_session_ptr get(uint32_t h);

public:
	// 获取单实例对象
	static rtp_session_manager& getInstance();
private:
	// 禁止外部构造
	rtp_session_manager() = default;
	// 禁止外部析构
	~rtp_session_manager() = default;
	// 禁止外部复制构造
	rtp_session_manager(const rtp_session_manager&) = delete;
	// 禁止外部赋值操作
	rtp_session_manager& operator=(const rtp_session_manager&) = delete;

private:
	std::unordered_map<uint32_t, rtp_session_ptr> m_sessionmap;

	std::mutex m_sesmapmtx;


};


#endif