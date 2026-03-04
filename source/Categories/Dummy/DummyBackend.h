#pragma once

#include "Core/BaseBackend.h"

class DummyBackend : public BaseBackend
{
public:
    double GetValue(uint32_t deviceIndex, uint32_t metricId) override
    {
        return 42.0; // testowa stała
    }

protected:
    bool OnInitialize() override
    {
        return true;
    }

    void OnUpdate() override
    {
        // nic
    }
};