#pragma once

#include <windows.h>
#include <pdh.h>

#include <vector>

// PdhGetFormattedCounterArrayW wants its buffer sized by a first call that reports how
// much it needs, so every caller writes the same six lines. Both WinApiGpuProvider and
// WinApiStorageProvider had their own copy and they had already drifted - one passed
// PDH_FMT_NOCAP100, the other forgot to clear `count` before the sizing call.
//
// `format` is per call: GPU engine counters are percentages that legitimately exceed 100
// when summed, so they need PDH_FMT_NOCAP100; disk counters do not.
//
// The item type is the UNICODE one, which is what PDH_FMT_COUNTERVALUE_ITEM already
// resolves to in this project (CharacterSet=Unicode), so a caller using the unsuffixed
// name needs no cast.
inline bool PdhReadCounterArray(PDH_HCOUNTER counter, std::vector<BYTE>& buf,
                                PDH_FMT_COUNTERVALUE_ITEM_W*& items, DWORD& count,
                                DWORD format = PDH_FMT_DOUBLE)
{
    DWORD bufSize = 0;
    count = 0;

    PdhGetFormattedCounterArrayW(counter, format, &bufSize, &count, nullptr);
    if (bufSize == 0)
        return false;

    buf.resize(bufSize);
    items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buf.data());

    return PdhGetFormattedCounterArrayW(counter, format, &bufSize, &count, items) == ERROR_SUCCESS;
}
