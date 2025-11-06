#pragma once
#include "depacket.h"
#include "rtp_depacket.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class RtpDePacketManager {
public:
    static RtpDePacketManager& getInstance();

    int32_t Start(rtp_depacket_callback cb, void* userdata, uint32_t* h);
    int32_t Stop(uint32_t h);
    int32_t Input(uint32_t h, uint8_t* data, uint32_t datasize);
    int32_t SetPayload(uint32_t h, uint8_t payload, uint32_t streamtype);
    int32_t SetMediaOption(uint32_t h, int8_t* opt, int8_t* parm);

private:
    RtpDePacketManager();
    ~RtpDePacketManager();

    std::shared_ptr<depacket> getPtrByID(uint32_t nID);

    std::unordered_map<uint32_t, std::shared_ptr<depacket>> m_dePacketPtrMap;
    std::mutex m_dePacketPtrMapLock;
    uint32_t m_nBaseID = 1;
};