#include "Core/PollerThread.h"

// Stopping a poller never waits for its thread.
//
// Rainmeter calls FreeLibrary on the plugin the moment its last measure is gone, from
// its own thread, so a join here is either a stall (as long as a driver call takes) or
// the loader-lock deadlock 2.1.4 fixed. The thread is signalled and abandoned instead:
// it observes the stop event and exits on its own, and the module is pinned
// (HardwareCore::Instance) so the code it is still running stays mapped.
PollerThread::~PollerThread()
{
    Stop();
}

void PollerThread::Start(uint32_t intervalMs, Callback callback)
{
    Stop();

    auto ctx = std::make_shared<Context>();
    ctx->stopEvent  = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    ctx->intervalMs = intervalMs;
    ctx->callback   = std::move(callback);

    if (!ctx->stopEvent)
        return;

    auto* owned = new std::shared_ptr<Context>(ctx);   // the thread's own reference

    HANDLE thread = CreateThread(nullptr, 0, ThreadProc, owned, 0, nullptr);
    if (!thread)
    {
        delete owned;
        return;
    }

    CloseHandle(thread);   // nothing ever joins it, so the handle is not kept
    m_ctx = std::move(ctx);
}

void PollerThread::Stop()
{
    if (!m_ctx)
        return;

    SetEvent(m_ctx->stopEvent);
    m_ctx.reset();   // the thread holds the last reference and closes the event
}

DWORD WINAPI PollerThread::ThreadProc(LPVOID param)
{
    auto* owned = static_cast<std::shared_ptr<Context>*>(param);
    std::shared_ptr<Context> ctx = *owned;
    delete owned;

    while (WaitForSingleObject(ctx->stopEvent, ctx->intervalMs) == WAIT_TIMEOUT)
    {
        if (ctx->callback)
            ctx->callback();
    }

    return 0;
}
