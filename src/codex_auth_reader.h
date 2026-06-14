#pragma once

#include <optional>
#include <string>

namespace restime {

struct CodexAuth {
    std::wstring email;
    std::string access_token;
};

class CodexAuthReader {
public:
    explicit CodexAuthReader(std::wstring auth_path = {});

    std::optional<CodexAuth> read(std::wstring* error = nullptr) const;
    const std::wstring& auth_path() const { return auth_path_; }

private:
    std::wstring auth_path_;
};

std::wstring default_auth_path();
std::string wide_to_utf8(const std::wstring& value);
std::wstring utf8_to_wide(const std::string& value);

}  // namespace restime
