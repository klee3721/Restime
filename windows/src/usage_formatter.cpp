#include "usage_formatter.h"

#include <Windows.h>

#include <cwchar>
#include <iterator>

namespace restime {

namespace {

std::tm local_tm(std::chrono::system_clock::time_point value) {
    const auto time = std::chrono::system_clock::to_time_t(value);
    std::tm tm{};
    localtime_s(&tm, &time);
    return tm;
}

}  // namespace

std::wstring format_time(std::chrono::system_clock::time_point value) {
    const auto tm = local_tm(value);
    wchar_t buffer[16]{};
    std::swprintf(buffer, std::size(buffer), L"%02d:%02d", tm.tm_hour, tm.tm_min);
    return buffer;
}

std::wstring format_day(std::chrono::system_clock::time_point value) {
    const auto tm = local_tm(value);
    wchar_t buffer[16]{};
    std::swprintf(buffer, std::size(buffer), L"%d/%d", tm.tm_mday, tm.tm_mon + 1);
    return buffer;
}

std::wstring menu_title(const std::optional<CodexUsage>& usage) {
    if (!usage) {
        return L"5h --% --:-- | W --% --/--";
    }

    return L"5h " + std::to_wstring(usage->five_hour_remaining_percent) + L"% " +
           format_time(usage->five_hour_reset) + L" | W " +
           std::to_wstring(usage->weekly_remaining_percent) + L"% " +
           format_day(usage->weekly_reset);
}

std::wstring tooltip_text(const RefreshSnapshot& snapshot) {
    std::wstring text = L"Restime | " + menu_title(snapshot.usage);
    if (!snapshot.error.empty() && !snapshot.usage) {
        text += L" | " + snapshot.error;
    }
    if (text.size() > 127) {
        text.resize(124);
        text += L"...";
    }
    return text;
}

std::wstring source_label(UsageSource source) {
    switch (source) {
    case UsageSource::CodexApi:
        return L"Codex API";
    case UsageSource::LocalLog:
        return L"Local log";
    case UsageSource::None:
    default:
        return L"None";
    }
}

}  // namespace restime
