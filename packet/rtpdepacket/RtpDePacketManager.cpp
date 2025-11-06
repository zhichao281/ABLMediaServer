#include "RtpDePacketManager.h"

RtpDePacketManager& RtpDePacketManager::getInstance() {
    static RtpDePacketManager instance;
    return instance;
}

RtpDePacketManager::RtpDePacketManager() {}
RtpDePacketManager::~RtpDePacketManager() {}

std::shared_ptr<depacket> RtpDePacketManager::getPtrByID(uint32_t nID) {
    std::unique_lock<std::mutex> lock(m_dePacketPtrMapLock);
    auto it = m_dePacketPtrMap.find(nID);
    if (it != m_dePacketPtrMap.end())
        return it->second;
    return nullptr;
}

int32_t RtpDePacketManager::Start(rtp_depacket_callback cb, void* userdata, uint32_t* h) {
    if (cb == nullptr || h == nullptr)
        return e_rtpdepkt_err_invalidparam;

    auto ptr = std::make_shared<depacket>();
    if (ptr == nullptr)
        return e_rtpdepkt_err_mallocsessionfail;

    {
        std::unique_lock<std::mutex> lock(m_dePacketPtrMapLock);
        *h = m_nBaseID;
        m_dePacketPtrMap[m_nBaseID] = ptr;
        ptr->m_ID = m_nBaseID;
        ptr->m_userdata = userdata;
        ptr->m_cb = cb;

        ptr->cb.data = ptr->m_buff;
        ptr->cb.handle = m_nBaseID;
        ptr->cb.userdata = userdata;

        m_nBaseID++;
    }

    return e_rtpdepkt_err_noerror;
}

int32_t RtpDePacketManager::Stop(uint32_t h) {
    std::unique_lock<std::mutex> lock(m_dePacketPtrMapLock);
    auto it = m_dePacketPtrMap.find(h);
    if (it != m_dePacketPtrMap.end()) {
        m_dePacketPtrMap.erase(it);
        return e_rtpdepkt_err_noerror;
    }
    return e_rtpdepkt_err_notfindsession;
}

int32_t RtpDePacketManager::Input(uint32_t h, uint8_t* data, uint32_t datasize) {
    auto ptr = getPtrByID(h);
    if (ptr == nullptr)
        return e_rtpdepkt_err_notfindsession;

    if (ptr->streamtype == e_rtpdepkt_st_unknow)
        return e_rtpdepkt_err_nosetparam;

    return ptr->handle(data, datasize);
}

int32_t RtpDePacketManager::SetPayload(uint32_t h, uint8_t payload, uint32_t streamtype) {
    auto ptr = getPtrByID(h);
    if (ptr == nullptr)
        return e_rtpdepkt_err_notfindsession;

    ptr->payload = payload;
    ptr->streamtype = streamtype;
    ptr->cb.payload = payload;
    ptr->cb.streamtype = streamtype;

    return e_rtpdepkt_err_noerror;
}

int32_t RtpDePacketManager::SetMediaOption(uint32_t h, int8_t* opt, int8_t* parm) {
    return e_rtpdepkt_err_noerror;
}