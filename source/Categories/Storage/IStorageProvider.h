#pragma once

#include <cstdint>

class IStorageProvider
{
public:

    virtual ~IStorageProvider() = default;

    virtual bool Initialize() = 0;

    virtual void Update() = 0;

    virtual uint32_t GetDeviceCount() const = 0;

    virtual bool GetReadBytes(uint32_t device, double& value) = 0;
    virtual bool GetWriteBytes(uint32_t device, double& value) = 0;

    virtual bool GetReadSpeed(uint32_t device, double& value) = 0;
    virtual bool GetWriteSpeed(uint32_t device, double& value) = 0;

    virtual bool GetUsedSpace(uint32_t device, double& value) = 0;
    virtual bool GetFreeSpaceBytes(uint32_t device, double& value) = 0;
    virtual bool GetTotalSpace(uint32_t device, double& value) = 0;
};