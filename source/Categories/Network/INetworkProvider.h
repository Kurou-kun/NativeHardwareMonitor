#pragma once

#include <cstdint>

class INetworkProvider
{
public:

    virtual ~INetworkProvider() = default;

    virtual bool Initialize() = 0;

    virtual void Update() = 0;

    virtual bool GetDownload(uint32_t deviceIndex, double& value) = 0;

    virtual bool GetUpload(uint32_t deviceIndex, double& value) = 0;

    virtual bool GetDownloadTotal(uint32_t deviceIndex, double& value) = 0;

    virtual bool GetUploadTotal(uint32_t deviceIndex, double& value) = 0;

    virtual bool GetSpeed(uint32_t deviceIndex, double& value) = 0;

    virtual uint32_t GetDeviceCount() const = 0;
};