#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace restime {

enum class UsageSource {
    None,
    CodexApi,
    LocalLog
};

struct CodexUsage {
    int five_hour_remaining_percent = 0;
    std::chrono::system_clock::time_point five_hour_reset{};
    int weekly_remaining_percent = 0;
    std::chrono::system_clock::time_point weekly_reset{};
    std::chrono::system_clock::time_point observed_at{};
    UsageSource source = UsageSource::None;
    std::wstring source_path;
};

struct RefreshSnapshot {
    std::optional<CodexUsage> usage;
    std::wstring account_email;
    std::wstring error;
};

int remaining_percent(double used_percent);
std::chrono::system_clock::time_point unix_time(double seconds);
std::chrono::system_clock::time_point parse_iso8601_utc(const std::string& value);

}  // namespace restime
