#pragma once

#include "usage.h"

#include <string>

namespace restime {

std::wstring format_time(std::chrono::system_clock::time_point value);
std::wstring format_day(std::chrono::system_clock::time_point value);
std::wstring menu_title(const std::optional<CodexUsage>& usage);
std::wstring tooltip_text(const RefreshSnapshot& snapshot);
std::wstring source_label(UsageSource source);

}  // namespace restime
