#pragma once

#include <windows.h>
#include <cstdint>
#include <functional>

class PollerThread
{
public:
    using Callback = std::function<void()>;

    PollerThread() = default;
    ~PollerThread();

    void Start(uint32_t intervalMs, Callback callback);
    void Stop();

private:
    static DWORD WINAPI ThreadProc(LPVOID param);
    void Run();

    HANDLE   m_thread    = nullptr;
    HANDLE   m_stopEvent = nullptr;
    uint32_t m_intervalMs = 0;
    Callback m_callback;
};
