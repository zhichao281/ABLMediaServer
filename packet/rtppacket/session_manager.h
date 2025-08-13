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
	// ��ȡ��ʵ������
	static rtp_session_manager& getInstance();
private:
	// ��ֹ�ⲿ����
	rtp_session_manager() = default;
	// ��ֹ�ⲿ����
	~rtp_session_manager() = default;
	// ��ֹ�ⲿ���ƹ���
	rtp_session_manager(const rtp_session_manager&) = delete;
	// ��ֹ�ⲿ��ֵ����
	rtp_session_manager& operator=(const rtp_session_manager&) = delete;

private:
	std::unordered_map<uint32_t, rtp_session_ptr> m_sessionmap;

	std::mutex m_sesmapmtx;


};


#endif