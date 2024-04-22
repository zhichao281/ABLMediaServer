#pragma  once

#include <stdint.h>

//rtcp header
#pragma pack(push , 1)

typedef struct _rtcp_header
{
	uint8_t		report_count : 5;
	uint8_t 	padding_flag : 1;
	uint8_t 	version : 2;
	uint8_t		payload_type;
	uint16_t	packet_length;	//network byte order
}RTCP_HEADER;

//rtcp SR(sender report)/RR(receiver report)
typedef struct _rtcp_sr_sender_info
{
	uint32_t	ntptime_msw;
	uint32_t	ntptime_lsw;
	uint32_t	rtptime;
	uint32_t	sender_packet_count;
	uint32_t	sender_octet_count;
}RTCP_SR_SENDER_INFO;

typedef struct _rtcp_report_block
{
	uint32_t    ssrc_n;
	uint8_t		fraction_lost;
	uint8_t		cumulative_pakcet_lost[3];
	uint32_t	extended_highest_seq;
	uint32_t	interarrival_jitter;
	uint32_t	LSR;
	uint32_t	DLSR;

}RTCP_REPORT_BLOCK;

#pragma  pack(pop)