#pragma once

#include "codex_auth_reader.h"
#include "usage.h"

#include <optional>
#include <string>

namespace restime {

class CodexUsageApiClient {
public:
    std::optional<CodexUsage> read_usage(const std::string& access_token, std::wstring* error = nullptr) const;
};

}  // namespace restime
