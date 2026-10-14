#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IModule
{
public:
    virtual ~IModule() = default;

    virtual bool     Initialize() = 0;
    virtual void     GatherAll() = 0;

    // Gather only these device indices. An empty list means every device - which is what
    // a Debug=2 dump needs, and what a single-device module does regardless. Modules that
    // do not override this keep gathering everything, which is always correct, just not
    // always cheap: a machine here enumerates 37 network adapters and 13 CPU devices, all
    // of them read every tick for the sake of a skin showing one.
    virtual void     GatherDevices(const std::vector<uint32_t>& devices) { GatherAll(); }
    virtual double   GetValue(uint32_t metricId, uint32_t deviceIndex) = 0;
    virtual bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) = 0;

    // Which provider produced the last reading of this metric, for a Debug=2 dump.
    // nullptr when the metric is unsupported or nothing has gathered yet.
    virtual const wchar_t* GetSource(uint32_t metricId, uint32_t deviceIndex) { return nullptr; }
    virtual uint32_t GetDeviceCount() const = 0;

    // Only meaningful for Category::Ping — resolves a target host string to a
    // stable device-index slot (creating one on first sight). Every other
    // module inherits this default and is unaffected.
    virtual uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) { return 0; }
};
