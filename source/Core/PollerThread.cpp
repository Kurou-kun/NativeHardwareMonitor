#include "Core/PollerThread.h"

PollerThread::~PollerThread()
{
    Stop();
}

void PollerThread::Start(uint32_t intervalMs, Callback callback)
{
    Stop();

    m_intervalMs = intervalMs;
    m_callback   = std::move(callback);
    m_stopEvent  = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    m_thread     = CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
}

void PollerThread::Stop()
{
    if (m_stopEvent)
    {
        SetEvent(m_stopEvent);
    }

    if (m_thread)
    {
        WaitForSingleObject(m_thread, 5000);
        CloseHandle(m_thread);
        m_thread = nullptr;
    }

    if (m_stopEvent)
    {
        CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
    }
}

DWORD WINAPI PollerThread::ThreadProc(LPVOID param)
{
    static_cast<PollerThread*>(param)->Run();
    return 0;
}

void PollerThread::Run()
{
    while (WaitForSingleObject(m_stopEvent, m_intervalMs) == WAIT_TIMEOUT)
    {
        if (m_callback)
            m_callback();
    }
}
