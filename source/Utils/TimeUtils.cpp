#include "Utils/TimeUtils.h"

#include <windows.h>

uint64_t GetTimeMs()
{
    return static_cast<uint64_t>(GetTickCount64());
}
