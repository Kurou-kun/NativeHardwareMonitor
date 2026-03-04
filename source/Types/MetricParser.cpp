#include "Types/MetricParser.h"

#include <algorithm>
#include <cwctype>

#include "Types/NetworkMetric.h"
#include "Types/MemoryMetric.h"
#include "Types/CpuMetric.h"
#include "Types/GpuMetric.h"
#include "Types/StorageMetric.h"

static std::wstring Normalize(const std::wstring& input)
{
    std::wstring str = input;

    str.erase(str.begin(),
        std::find_if(str.begin(), str.end(),
            [](wchar_t ch) { return !std::iswspace(ch); }));

    str.erase(
        std::find_if(str.rbegin(), str.rend(),
            [](wchar_t ch) { return !std::iswspace(ch); }).base(),
        str.end());

    std::transform(str.begin(), str.end(), str.begin(), std::towlower);

    return str;
}

Category ParseCategory(const std::wstring& input)
{
    std::wstring str = Normalize(input);

    if (str == L"gpu") return Category::GPU;
    if (str == L"cpu") return Category::CPU;
    if (str == L"memory") return Category::Memory;
    if (str == L"network") return Category::Network;
    if (str == L"storage") return Category::Storage;

    return Category::Unknown;
}

uint32_t ParseMetric(Category category, const std::wstring& input)
{
    std::wstring str = Normalize(input);

    switch (category)
    {
    case Category::Memory:
    {
        if (str == L"total") return static_cast<uint32_t>(MemoryMetric::Total);
        if (str == L"used") return static_cast<uint32_t>(MemoryMetric::Used);
        if (str == L"free") return static_cast<uint32_t>(MemoryMetric::Free);
        if (str == L"usagepercent") return static_cast<uint32_t>(MemoryMetric::UsagePercent);
        return static_cast<uint32_t>(MemoryMetric::Unknown);
    }

    case Category::CPU:
    {
        if (str == L"usagepercent")
            return static_cast<uint32_t>(CpuMetric::UsagePercent);

        return static_cast<uint32_t>(CpuMetric::Unknown);
    }

    case Category::GPU:
    {
        if (str == L"utilizationpercent")
            return static_cast<uint32_t>(GpuMetric::UtilizationPercent);

        if (str == L"memoryusedbytes")
            return static_cast<uint32_t>(GpuMetric::MemoryUsedBytes);

        return static_cast<uint32_t>(GpuMetric::Unknown);
    }

    case Category::Network:
    {
        if (str == L"rxbytespersec")
            return static_cast<uint32_t>(NetworkMetric::RxBytesPerSec);

        if (str == L"txbytespersec")
            return static_cast<uint32_t>(NetworkMetric::TxBytesPerSec);

        return static_cast<uint32_t>(NetworkMetric::Unknown);
    }

    case Category::Storage:
    {
        if (str == L"readbytespersec")
            return static_cast<uint32_t>(StorageMetric::ReadBytesPerSec);

        if (str == L"writebytespersec")
            return static_cast<uint32_t>(StorageMetric::WriteBytesPerSec);

        return static_cast<uint32_t>(StorageMetric::Unknown);
    }

    default:
        return static_cast<uint32_t>(-1);
    }
}