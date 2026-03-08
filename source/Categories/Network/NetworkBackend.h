#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "Core/BaseBackend.h"

class NetworkBackend : public BaseBackend
{
public:
    double GetValue(uint32_t deviceIndex, uint32_t metricId) override;

    uint32_t GetDeviceCount() const;
    const std::wstring& GetDeviceName(uint32_t index) const;

protected:
    bool OnInitialize() override;
    void OnUpdate() override;

private:
    struct Adapter
    {
        std::wstring name;

        bool active = false;

        uint64_t prevRx = 0;
        uint64_t prevTx = 0;

        uint64_t currRx = 0;
        uint64_t currTx = 0;

        double rxPerSec = 0.0;
        double txPerSec = 0.0;
    };

    std::vector<Adapter> m_adapters;

    uint64_t m_prevTime = 0;

    bool ReadSnapshot();
};