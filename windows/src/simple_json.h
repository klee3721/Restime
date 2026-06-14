#pragma once

#include "usage.h"

#include <optional>
#include <string>
#include <string_view>

namespace restime::json {

std::optional<std::string> string_value(std::string_view json, std::string_view key);
std::optional<double> number_after_key(std::string_view json, std::string_view key);
std::optional<std::string_view> object_after_key(std::string_view json, std::string_view key);
std::optional<CodexUsage> parse_api_usage(std::string_view json);
std::optional<CodexUsage> parse_session_usage_line(std::string_view line, const std::wstring& source_path);

}  // namespace restime::json
