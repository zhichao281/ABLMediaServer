#pragma once

#include <stdint.h>
#include <cstring>
uint32_t generate_identifier_rtppacket();

void recycle_identifier_rtppacket(uint32_t id);

int32_t get_mediatype_rtppacket(int32_t st);

