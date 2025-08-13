#ifndef _RTP_PACKET_DEPACKET_COMMON_H_
#define _RTP_PACKET_DEPACKET_COMMON_H_

#include <stdint.h>

uint32_t generate_identifier_rtppacket();

void recycle_identifier_rtppacket(uint32_t id);

int32_t get_mediatype(int32_t st);

#endif 