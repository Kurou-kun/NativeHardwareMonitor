#pragma once

#include "Core/ICategoryBackend.h"

class BaseBackend : public ICategoryBackend
{
public:
    bool Initialize() override
    {
        if (!m_initialized)
        {
            m_initialized = OnInitialize();
        }
        return m_initialized;
    }

    void Update() override
    {
        if (!m_initialized)
            return;

        OnUpdate();
    }

    uint32_t GetDeviceCount() const override
    {
        return 1;
    }

protected:
    virtual bool OnInitialize() = 0;
    virtual void OnUpdate() = 0;

    bool m_initialized = false;
};