#include "codex_auth_reader.h"

#include "simple_json.h"

#include <Windows.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace restime {

namespace {

constexpr size_t kMaxAuthBytes = 1024 * 1024;

std::string read_small_file(const std::wstring& path, std::wstring* error) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        if (error) {
            *error = L"No Codex account signed in";
        }
        return {};
    }

    const auto end_pos = file.tellg();
    if (end_pos < std::streampos(0)) {
        if (error) {
            *error = L"Could not read Codex auth file";
        }
        return {};
    }

    const auto size = static_cast<std::streamoff>(end_pos);
    if (static_cast<size_t>(size) > kMaxAuthBytes) {
        if (error) {
            *error = L"Codex auth file is too large";
        }
        return {};
    }

    file.seekg(0, std::ios::beg);
    std::string data(static_cast<size_t>(size), '\0');
    file.read(data.data(), static_cast<std::streamsize>(data.size()));
    return data;
}

int base64_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::optional<std::string> base64url_decode(std::string value) {
    std::replace(value.begin(), value.end(), '-', '+');
    std::replace(value.begin(), value.end(), '_', '/');
    while (value.size() % 4 != 0) {
        value.push_back('=');
    }

    std::string output;
    output.reserve(value.size() * 3 / 4);
    for (size_t i = 0; i < value.size(); i += 4) {
        int values[4]{};
        int padding = 0;
        for (int j = 0; j < 4; ++j) {
            const char c = value[i + j];
            if (c == '=') {
                values[j] = 0;
                ++padding;
            } else {
                values[j] = base64_value(c);
                if (values[j] < 0) {
                    return std::nullopt;
                }
            }
        }

        const int triple = (values[0] << 18) | (values[1] << 12) | (values[2] << 6) | values[3];
        output.push_back(static_cast<char>((triple >> 16) & 0xFF));
        if (padding < 2) output.push_back(static_cast<char>((triple >> 8) & 0xFF));
        if (padding < 1) output.push_back(static_cast<char>(triple & 0xFF));
    }

    return output;
}

std::optional<std::string> jwt_payload(const std::string& token) {
    const size_t first = token.find('.');
    if (first == std::string::npos) {
        return std::nullopt;
    }
    const size_t second = token.find('.', first + 1);
    if (second == std::string::npos || second <= first + 1) {
        return std::nullopt;
    }
    return base64url_decode(token.substr(first + 1, second - first - 1));
}

}  // namespace

std::string wide_to_utf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string output(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), output.data(), bytes, nullptr, nullptr);
    return output;
}

std::wstring utf8_to_wide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int chars = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring output(chars, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), output.data(), chars);
    return output;
}

std::wstring default_auth_path() {
    wchar_t profile[MAX_PATH]{};
    const DWORD chars = GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
    if (chars == 0 || chars >= MAX_PATH) {
        return L".codex\\auth.json";
    }
    return std::wstring(profile) + L"\\.codex\\auth.json";
}

CodexAuthReader::CodexAuthReader(std::wstring auth_path)
    : auth_path_(auth_path.empty() ? default_auth_path() : std::move(auth_path)) {}

std::optional<CodexAuth> CodexAuthReader::read(std::wstring* error) const {
    const std::string text = read_small_file(auth_path_, error);
    if (text.empty()) {
        return std::nullopt;
    }

    auto tokens = json::object_after_key(text, "tokens");
    if (!tokens) {
        if (error) *error = L"Codex auth file is missing tokens";
        return std::nullopt;
    }

    auto access_token = json::string_value(*tokens, "access_token");
    if (!access_token || access_token->empty()) {
        if (error) *error = L"Codex access token is missing";
        return std::nullopt;
    }

    std::string email;
    if (const auto id_token = json::string_value(*tokens, "id_token")) {
        if (const auto payload = jwt_payload(*id_token)) {
            email = json::string_value(*payload, "email").value_or("");
        }
    }

    if (email.empty()) {
        if (const auto payload = jwt_payload(*access_token)) {
            if (const auto profile = json::object_after_key(*payload, "https://api.openai.com/profile")) {
                email = json::string_value(*profile, "email").value_or("");
            }
        }
    }

    CodexAuth auth;
    auth.email = utf8_to_wide(email);
    auth.access_token = std::move(*access_token);
    return auth;
}

}  // namespace restime
