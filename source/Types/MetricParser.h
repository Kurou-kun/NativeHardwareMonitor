#pragma once

#include <string>
#include "Types/Category.h"

Category ParseCategory(const std::wstring& input);

uint32_t ParseMetric(Category category, const std::wstring& input);