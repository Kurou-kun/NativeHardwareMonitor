#pragma once

// winsock2.h must be included before anything that drags in <windows.h>
// (Core/PollerThread.h does), otherwise <windows.h>'s default winsock.h
// clashes with winsock2.h (redefinition errors).
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <ipexport.h>
#include <icmpapi.h>

#include "Core/IProvider.h"
#include "Core/PollerThread.h"
#include "Types/Snapshot.h"

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class WinApiPingProvider : public IProvider
{
public:
    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;
    uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) override;

private:
    static constexpr size_t kHistorySize = 20;

    struct Target
    {
        ~Target();

        std::wstring host;
        bool         resolved = false;
        IPAddr       address  = 0;

        HANDLE                        icmpHandle = nullptr;
        uint32_t                      intervalMs = 1000;
        std::unique_ptr<PollerThread> poller;

        std::mutex       mutex;      // guards every field below
        double            rtt        = 9999.0;
        double            packetLoss = 0.0;
        double            minRtt     = 9999.0;
        double            maxRtt     = 9999.0;
        double            avgRtt     = 9999.0;
        double            jitter     = 0.0;
        double            ttl        = 0.0;   // last reply's TTL (hop-count hint)
        uint64_t          packetsSent     = 0;
        uint64_t          packetsReceived = 0;
        std::wstring      resolvedIp;         // dotted IPv4, empty until resolved
        std::wstring      status;             // "reachable" / "timeout" / "dns-fail"
        std::deque<bool>   history;    // true = reply received, capped at kHistorySize
        std::deque<double> rttHistory; // successful RTTs only, capped at kHistorySize
    };

    void PingOnce(Target* target);

    std::vector<std::unique_ptr<Target>> m_targets;
    mutable std::mutex                    m_targetsMutex; // guards m_targets itself (not Target internals)
};
