#ifndef _RTP_PACKET_DEPACKET_SESSION_H_
#define _RTP_PACKET_DEPACKET_SESSION_H_

#include <unordered_map>
#include <memory>
#include "packet.h"
#include "rtp_packet.h"

class rtp_session_packet
{
public:
	rtp_session_packet(rtp_packet_callback cb, void* userdata);
	~rtp_session_packet();

	uint32_t get_id() const;

	int32_t handle(_rtp_packet_input* in);

	int32_t set_option(_rtp_packet_sessionopt* opt);

	int32_t reset_option(_rtp_packet_sessionopt* opt);

private:
	const uint32_t m_id;
	std::unordered_map<uint32_t, rtp_packet_ptr> m_pktmap;
	const rtp_packet_callback m_cb;
	const void* m_userdata;

};

typedef std::shared_ptr<rtp_session_packet> rtp_session_ptr;

#endif