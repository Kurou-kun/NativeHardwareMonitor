#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

class DummyProvider : public IProvider
{
public:
    bool     Initialize() override { return true; }
    uint32_t GetDeviceCount() const override { return 1; }
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override {}
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override { return false; }
};
