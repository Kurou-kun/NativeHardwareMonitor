#pragma once

#include <windows.h>
#include <cstdint>
#include <functional>
#include <memory>

class PollerThread
{
public:
    using Callback = std::function<void()>;

    PollerThread() = default;
    ~PollerThread();

    void Start(uint32_t intervalMs, Callback callback);
    void Stop();   // signals the thread and returns; never joins - see the .cpp

private:
    // Owned jointly by this object and the thread, so the stop event outlives a
    // Stop() that happens while the thread is still finishing a gather.
    struct Context
    {
        HANDLE   stopEvent  = nullptr;
        uint32_t intervalMs = 0;
        Callback callback;

        ~Context() { if (stopEvent) CloseHandle(stopEvent); }
    };

    static DWORD WINAPI ThreadProc(LPVOID param);

    std::shared_ptr<Context> m_ctx;
};
