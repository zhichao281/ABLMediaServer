#pragma once
#include <unordered_set>
#include <mutex>
#include <cstdint>

class IdentifierManager {
public:
    static IdentifierManager& getInstance();

    uint32_t Generate();
    void Recycle(uint32_t id);

private:
    IdentifierManager() = default;
    ~IdentifierManager() = default;
    IdentifierManager(const IdentifierManager&) = delete;
    IdentifierManager& operator=(const IdentifierManager&) = delete;

    std::unordered_set<uint32_t> m_identifier_set;
    std::mutex m_mutex;
    uint32_t m_next_id = 1;
};