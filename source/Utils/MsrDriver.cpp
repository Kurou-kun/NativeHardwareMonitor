#include "Utils/MsrDriver.h"
#include "Utils/Debug.h"

#include <Windows.h>

static HANDLE g_driver = INVALID_HANDLE_VALUE;
static SC_HANDLE g_service = nullptr;

static const wchar_t* SERVICE_NAME = L"NHM_MSR";
static const wchar_t* DEVICE_NAME = L"\\\\.\\NHM_MSR";

#define IOCTL_READ_MSR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct MSR_REQUEST
{
    uint32_t core;
    uint32_t msr;
};

bool MsrDriver::Initialize()
{
    if (g_driver != INVALID_HANDLE_VALUE)
        return true;

    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);

    if (!scm)
    {
        LOG_ERROR(L"OpenSCManager failed: %lu", GetLastError());
        return false;
    }

    wchar_t path[MAX_PATH];
    GetTempPathW(MAX_PATH, path);
    wcscat_s(path, L"nhm_msr.sys");

    g_service = CreateServiceW(
        scm,
        SERVICE_NAME,
        SERVICE_NAME,
        SERVICE_START | DELETE | SERVICE_STOP,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        path,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    if (!g_service)
    {
        g_service = OpenServiceW(scm, SERVICE_NAME, SERVICE_START);
        if (!g_service)
        {
            LOG_ERROR(L"Create/Open service failed: %lu", GetLastError());
            CloseServiceHandle(scm);
            return false;
        }
    }

    StartServiceW(g_service, 0, nullptr);

    g_driver = CreateFileW(
        DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (g_driver == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR(L"Failed to open MSR device: %lu", GetLastError());
        CloseServiceHandle(scm);
        return false;
    }

    LOG_INFO(L"MSR driver loaded");

    CloseServiceHandle(scm);
    return true;
}

void MsrDriver::Shutdown()
{
    if (g_driver != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_driver);
        g_driver = INVALID_HANDLE_VALUE;
    }

    if (g_service)
    {
        SERVICE_STATUS status;
        ControlService(g_service, SERVICE_CONTROL_STOP, &status);
        DeleteService(g_service);
        CloseServiceHandle(g_service);
        g_service = nullptr;

        LOG_INFO(L"MSR driver unloaded");
    }
}

bool MsrDriver::IsAvailable()
{
    return g_driver != INVALID_HANDLE_VALUE;
}

bool MsrDriver::Read(uint32_t msr, uint64_t& value)
{
    return ReadCore(0, msr, value);
}

bool MsrDriver::ReadCore(uint32_t core, uint32_t msr, uint64_t& value)
{
    if (g_driver == INVALID_HANDLE_VALUE)
        return false;

    MSR_REQUEST req;

    req.core = core;
    req.msr = msr;

    DWORD returned = 0;

    BOOL ok = DeviceIoControl(
        g_driver,
        IOCTL_READ_MSR,
        &req,
        sizeof(req),
        &value,
        sizeof(value),
        &returned,
        nullptr
    );

    if (!ok)
    {
        LOG_ERROR(L"MSR read failed core=%u msr=0x%X error=%lu",
            core, msr, GetLastError());
        return false;
    }

    return true;
}