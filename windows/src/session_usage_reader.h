#pragma once

#include "usage.h"

#include <optional>
#include <string>

namespace restime {

class SessionUsageReader {
public:
    explicit SessionUsageReader(std::wstring sessions_root = {});
    std::optional<CodexUsage> read_latest(std::wstring* error = nullptr) const;

private:
    std::wstring sessions_root_;
};

std::wstring default_sessions_root();

}  // namespace restime
