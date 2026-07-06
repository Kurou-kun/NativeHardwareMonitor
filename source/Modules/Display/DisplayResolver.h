#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <memory>
#include <string>

class DisplayResolver
{
public:
    bool     Initialize();
    uint32_t GetDeviceCount() const;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap);
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out);

private:
    std::unique_ptr<IProvider> m_provider;
};
