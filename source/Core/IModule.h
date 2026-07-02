#pragma once

#include <cstdint>
#include <string>

class IModule
{
public:
    virtual ~IModule() = default;

    virtual bool     Initialize() = 0;
    virtual void     GatherAll() = 0;
    virtual double   GetValue(uint32_t metricId, uint32_t deviceIndex) = 0;
    virtual bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) = 0;
    virtual uint32_t GetDeviceCount() const = 0;
};
