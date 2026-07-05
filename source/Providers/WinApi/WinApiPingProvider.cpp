#include "Providers/WinApi/WinApiPingProvider.h"
#include "Types/PingMetric.h"
#include "Utils/Debug.h"

#include <algorithm>
#include <cmath>
#include <cwctype>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

WinApiPingProvider::Target::~Target()
{
    // Stop the poller before closing the handle it's using — PollerThread::Stop()
    // joins the thread, so PingOnce() cannot still be running once this returns.
    if (poller)
        poller->Stop();

    if (icmpHandle)
        IcmpCloseHandle(icmpHandle);
}

static std::wstring NormalizeHost(const std::wstring& input)
{
    std::wstring str = input;

    str.erase(str.begin(),
        std::find_if(str.begin(), str.end(), [](wchar_t c) { return !std::iswspace(c); }));

    str.erase(
        std::find_if(str.rbegin(), str.rend(), [](wchar_t c) { return !std::iswspace(c); }).base(),
        str.end());

    std::transform(str.begin(), str.end(), str.begin(), std::towlower);

    return str;
}

bool WinApiPingProvider::Initialize()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        LOG_STARTUP(L"WinApiPingProvider: WSAStartup failed");
        return false;
    }

    LOG_STARTUP(L"WinApiPingProvider: initialized");
    return true;
}

uint32_t WinApiPingProvider::GetDeviceCount() const
{
    std::lock_guard<std::mutex> lock(m_targetsMutex);
    return static_cast<uint32_t>(m_targets.size());
}

void WinApiPingProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    Target* target = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_targetsMutex);
        if (deviceIndex >= m_targets.size())
            return;
        target = m_targets[deviceIndex].get();
    }

    std::lock_guard<std::mutex> lock(target->mutex);
    snap.Set(static_cast<uint32_t>(PingMetric::Rtt),             target->rtt);
    snap.Set(static_cast<uint32_t>(PingMetric::PacketLoss),      target->packetLoss);
    snap.Set(static_cast<uint32_t>(PingMetric::MinRtt),          target->minRtt);
    snap.Set(static_cast<uint32_t>(PingMetric::MaxRtt),          target->maxRtt);
    snap.Set(static_cast<uint32_t>(PingMetric::AvgRtt),          target->avgRtt);
    snap.Set(static_cast<uint32_t>(PingMetric::Jitter),         target->jitter);
    snap.Set(static_cast<uint32_t>(PingMetric::Ttl),            target->ttl);
    snap.Set(static_cast<uint32_t>(PingMetric::PacketsSent),     static_cast<double>(target->packetsSent));
    snap.Set(static_cast<uint32_t>(PingMetric::PacketsReceived), static_cast<double>(target->packetsReceived));
}

bool WinApiPingProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    Target* target = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_targetsMutex);
        if (deviceIndex >= m_targets.size())
            return false;
        target = m_targets[deviceIndex].get();
    }

    std::lock_guard<std::mutex> lock(target->mutex);
    switch (static_cast<PingMetric>(metricId))
    {
    case PingMetric::ResolvedIp: out = target->resolvedIp; return true;
    case PingMetric::Status:     out = target->status;     return true;
    default:                     return false;
    }
}

static std::wstring FormatIPv4(IPAddr address)
{
    in_addr ia{};
    ia.s_addr = address;
    wchar_t buf[INET_ADDRSTRLEN] = {};
    if (InetNtopW(AF_INET, &ia, buf, INET_ADDRSTRLEN))
        return buf;
    return L"";
}

static bool ResolveIPv4(const std::wstring& host, IPAddr& out)
{
    ADDRINFOW hints{};
    hints.ai_family = AF_INET;

    PADDRINFOW result = nullptr;
    if (GetAddrInfoW(host.c_str(), nullptr, &hints, &result) != 0 || !result)
        return false;

    out = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr.s_addr;
    FreeAddrInfoW(result);
    return true;
}

void WinApiPingProvider::PingOnce(Target* target)
{
    // Retry DNS each poll until it succeeds — registration can run before the
    // network is up (skin loads at boot). Until resolved, each poll records a
    // miss below so the target reads as down (rtt 9999, loss → 100) instead of
    // a perfect 0 ms connection.
    if (!target->resolved)
        target->resolved = ResolveIPv4(target->host, target->address);

    bool   success = false;
    bool   sent    = false;
    double rttMs   = 9999.0;
    double ttl     = 0.0;

    if (target->resolved)
    {
        if (!target->icmpHandle)
        {
            target->icmpHandle = IcmpCreateFile();
            if (target->icmpHandle == INVALID_HANDLE_VALUE)
                target->icmpHandle = nullptr;
        }

        if (target->icmpHandle)
        {
            char sendData[32] = "NativeHardwareMonitor ping";
            BYTE replyBuffer[sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8];

            DWORD timeout = std::min(target->intervalMs, 1000u);

            sent = true;
            DWORD result = IcmpSendEcho(
                target->icmpHandle,
                target->address,
                sendData, static_cast<WORD>(sizeof(sendData)),
                nullptr,
                replyBuffer, sizeof(replyBuffer),
                timeout);

            if (result != 0)
            {
                auto* reply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer);
                if (reply->Status == IP_SUCCESS)
                {
                    success = true;
                    rttMs   = static_cast<double>(reply->RoundTripTime);
                    ttl     = static_cast<double>(reply->Options.Ttl);
                }
            }
        }
    }

    std::lock_guard<std::mutex> lock(target->mutex);

    target->history.push_back(success);
    if (target->history.size() > kHistorySize)
        target->history.pop_front();

    size_t misses = 0;
    for (bool hit : target->history)
        if (!hit) ++misses;

    target->rtt        = success ? rttMs : 9999.0;
    target->packetLoss = target->history.empty() ? 0.0
                        : (static_cast<double>(misses) / target->history.size()) * 100.0;

    if (sent)            ++target->packetsSent;
    if (success)
    {
        ++target->packetsReceived;
        target->ttl = ttl;

        target->rttHistory.push_back(rttMs);
        if (target->rttHistory.size() > kHistorySize)
            target->rttHistory.pop_front();
    }

    // Min/Max/Avg over the successful-RTT window; 9999 sentinel until we get one.
    if (target->rttHistory.empty())
    {
        target->minRtt = target->maxRtt = target->avgRtt = 9999.0;
        target->jitter = 0.0;
    }
    else
    {
        double lo = target->rttHistory.front();
        double hi = lo, sum = 0.0;
        for (double v : target->rttHistory)
        {
            lo = std::min(lo, v);
            hi = std::max(hi, v);
            sum += v;
        }
        target->minRtt = lo;
        target->maxRtt = hi;
        target->avgRtt = sum / target->rttHistory.size();

        // ponytail: jitter = mean absolute consecutive RTT difference (RFC-3550-ish).
        // Good enough for a status readout; swap for smoothed IPDV if a graph needs it.
        double diffSum = 0.0;
        for (size_t i = 1; i < target->rttHistory.size(); ++i)
            diffSum += std::abs(target->rttHistory[i] - target->rttHistory[i - 1]);
        target->jitter = target->rttHistory.size() > 1
                       ? diffSum / (target->rttHistory.size() - 1) : 0.0;
    }

    if (target->resolvedIp.empty() && target->resolved)
        target->resolvedIp = FormatIPv4(target->address);

    target->status = !target->resolved ? L"dns-fail"
                   : success            ? L"reachable"
                   :                      L"timeout";
}

uint32_t WinApiPingProvider::ResolveTarget(const std::wstring& host, uint32_t intervalMs)
{
    std::wstring key = NormalizeHost(host);
    uint32_t     interval = intervalMs > 0 ? intervalMs : 1000;

    std::lock_guard<std::mutex> lock(m_targetsMutex);

    for (size_t i = 0; i < m_targets.size(); ++i)
    {
        if (m_targets[i]->host != key)
            continue;

        if (interval < m_targets[i]->intervalMs)
        {
            m_targets[i]->intervalMs = interval;
            Target* t = m_targets[i].get();
            m_targets[i]->poller->Start(interval, [this, t]() { PingOnce(t); });
        }

        return static_cast<uint32_t>(i);
    }

    auto target = std::make_unique<Target>();
    target->host       = key;
    target->intervalMs = interval;

    target->resolved = ResolveIPv4(key, target->address);
    if (!target->resolved)
        LOG_STARTUP(L"WinApiPingProvider: failed to resolve host '%s' (will retry each poll)", key.c_str());

    uint32_t index = static_cast<uint32_t>(m_targets.size());

    Target* t = target.get();
    target->poller = std::make_unique<PollerThread>();
    target->poller->Start(target->intervalMs, [this, t]() { PingOnce(t); });

    m_targets.push_back(std::move(target));

    LOG_STARTUP(L"WinApiPingProvider: registered target '%s' -> slot %u (resolved=%d)",
        key.c_str(), index, t->resolved ? 1 : 0);

    return index;
}
