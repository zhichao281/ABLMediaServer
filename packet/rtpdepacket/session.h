#ifndef _RTP_PACKET_DEPACKET_SESSION_H_
#define _RTP_PACKET_DEPACKET_SESSION_H_

#include <unordered_map>
#include <memory>
#include "depacket.h"
#include "rtp_depacket.h"

class rtp_session
{
public:
	rtp_session(rtp_depacket_callback cb, void* userdata);
	~rtp_session();

	uint32_t get_id() const { return m_id; }
	int32_t handle(uint8_t* data, uint32_t datasize);
	void set_payload(uint8_t payload, uint32_t streamtype);
	void set_mediaoption(const std::string& opt, const std::string& param);

private:
	bool rtp_check(uint8_t* data, uint32_t datasize, uint32_t& ssrc);
	bool is_rtcp(uint8_t payload);
	rtp_depacket_ptr& get_depacket(uint32_t ssrc);

private:
	const uint32_t m_id;
	const rtp_depacket_callback m_cb;
	const void* m_userdata;
	std::unordered_map<uint32_t, rtp_depacket_ptr> m_depacketMap;
	std::unordered_map<uint8_t, uint32_t> m_payloadMap;
	std::unordered_map<std::string, std::string> m_mediaoptMap;
};
typedef std::shared_ptr<rtp_session> rtp_session_ptr;

#endif