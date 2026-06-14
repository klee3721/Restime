#include "simple_json.h"

#include <charconv>

namespace restime::json {

namespace {

std::optional<size_t> find_key(std::string_view json, std::string_view key) {
    std::string needle;
    needle.reserve(key.size() + 2);
    needle.push_back('"');
    needle.append(key);
    needle.push_back('"');

    const size_t pos = json.find(needle);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    const size_t colon = json.find(':', pos + needle.size());
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    return colon + 1;
}

size_t skip_ws(std::string_view value, size_t pos) {
    while (pos < value.size()) {
        const char c = value[pos];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            break;
        }
        ++pos;
    }
    return pos;
}

std::optional<std::string_view> nested_object(std::string_view json, std::string_view first, std::string_view second) {
    const auto outer = object_after_key(json, first);
    if (!outer) {
        return std::nullopt;
    }
    return object_after_key(*outer, second);
}

}  // namespace

std::optional<std::string> string_value(std::string_view json, std::string_view key) {
    const auto start_opt = find_key(json, key);
    if (!start_opt) {
        return std::nullopt;
    }

    size_t pos = skip_ws(json, *start_opt);
    if (pos >= json.size() || json[pos] != '"') {
        return std::nullopt;
    }
    ++pos;

    std::string result;
    while (pos < json.size()) {
        const char c = json[pos++];
        if (c == '"') {
            return result;
        }
        if (c == '\\' && pos < json.size()) {
            const char escaped = json[pos++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                result.push_back(escaped);
                break;
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            default:
                result.push_back(escaped);
                break;
            }
        } else {
            result.push_back(c);
        }
    }

    return std::nullopt;
}

std::optional<double> number_after_key(std::string_view json, std::string_view key) {
    const auto start_opt = find_key(json, key);
    if (!start_opt) {
        return std::nullopt;
    }

    size_t pos = skip_ws(json, *start_opt);
    const size_t begin = pos;
    while (pos < json.size()) {
        const char c = json[pos];
        if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
            ++pos;
        } else {
            break;
        }
    }

    if (begin == pos) {
        return std::nullopt;
    }

    double value = 0;
    const auto parsed = std::from_chars(json.data() + begin, json.data() + pos, value);
    if (parsed.ec != std::errc{}) {
        return std::nullopt;
    }

    return value;
}

std::optional<std::string_view> object_after_key(std::string_view json, std::string_view key) {
    const auto start_opt = find_key(json, key);
    if (!start_opt) {
        return std::nullopt;
    }

    size_t pos = skip_ws(json, *start_opt);
    if (pos >= json.size() || json[pos] != '{') {
        return std::nullopt;
    }

    const size_t begin = pos;
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (; pos < json.size(); ++pos) {
        const char c = json[pos];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }

        if (c == '"') {
            in_string = true;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                return json.substr(begin, pos - begin + 1);
            }
        }
    }

    return std::nullopt;
}

std::optional<CodexUsage> parse_api_usage(std::string_view text) {
    const auto rate_limit = object_after_key(text, "rate_limit");
    if (!rate_limit) {
        return std::nullopt;
    }
    const auto primary = object_after_key(*rate_limit, "primary_window");
    const auto secondary = object_after_key(*rate_limit, "secondary_window");
    if (!primary || !secondary) {
        return std::nullopt;
    }

    const auto primary_used = number_after_key(*primary, "used_percent");
    const auto secondary_used = number_after_key(*secondary, "used_percent");
    const auto primary_reset = number_after_key(*primary, "reset_at");
    const auto secondary_reset = number_after_key(*secondary, "reset_at");
    if (!primary_used || !secondary_used || !primary_reset || !secondary_reset) {
        return std::nullopt;
    }

    CodexUsage usage;
    usage.five_hour_remaining_percent = remaining_percent(*primary_used);
    usage.five_hour_reset = unix_time(*primary_reset);
    usage.weekly_remaining_percent = remaining_percent(*secondary_used);
    usage.weekly_reset = unix_time(*secondary_reset);
    usage.observed_at = std::chrono::system_clock::now();
    usage.source = UsageSource::CodexApi;
    usage.source_path = L"https://chatgpt.com/backend-api/wham/usage";
    return usage;
}

std::optional<CodexUsage> parse_session_usage_line(std::string_view line, const std::wstring& source_path) {
    if (line.find("\"rate_limits\"") == std::string_view::npos) {
        return std::nullopt;
    }

    const auto payload = object_after_key(line, "payload");
    if (!payload) {
        return std::nullopt;
    }
    const auto rate_limits = object_after_key(*payload, "rate_limits");
    if (!rate_limits) {
        return std::nullopt;
    }
    const auto primary = object_after_key(*rate_limits, "primary");
    const auto secondary = object_after_key(*rate_limits, "secondary");
    if (!primary || !secondary) {
        return std::nullopt;
    }

    const auto primary_used = number_after_key(*primary, "used_percent");
    const auto secondary_used = number_after_key(*secondary, "used_percent");
    const auto primary_reset = number_after_key(*primary, "resets_at");
    const auto secondary_reset = number_after_key(*secondary, "resets_at");
    if (!primary_used || !secondary_used || !primary_reset || !secondary_reset) {
        return std::nullopt;
    }

    CodexUsage usage;
    usage.five_hour_remaining_percent = remaining_percent(*primary_used);
    usage.five_hour_reset = unix_time(*primary_reset);
    usage.weekly_remaining_percent = remaining_percent(*secondary_used);
    usage.weekly_reset = unix_time(*secondary_reset);
    usage.observed_at = parse_iso8601_utc(string_value(line, "timestamp").value_or(""));
    usage.source = UsageSource::LocalLog;
    usage.source_path = source_path;
    return usage;
}

}  // namespace restime::json
