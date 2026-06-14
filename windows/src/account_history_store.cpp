#include "account_history_store.h"

#include <Windows.h>

#include <algorithm>
#include <cwctype>
#include <sstream>
#include <utility>

namespace restime {

namespace {

constexpr wchar_t kHistoryValue[] = L"AccountHistory";

std::wstring lower_copy(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return value;
}

long long to_unix_seconds(std::chrono::system_clock::time_point value) {
    return std::chrono::duration_cast<std::chrono::seconds>(value.time_since_epoch()).count();
}

std::chrono::system_clock::time_point from_unix_seconds(long long value) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(value));
}

std::wstring encode_field(std::wstring value) {
    for (auto& c : value) {
        if (c == L'\t' || c == L'\r' || c == L'\n') {
            c = L' ';
        }
    }
    return value;
}

}  // namespace

bool same_email(const std::wstring& lhs, const std::wstring& rhs) {
    return lower_copy(lhs) == lower_copy(rhs);
}

AccountHistoryStore::AccountHistoryStore(std::wstring registry_subkey)
    : registry_subkey_(std::move(registry_subkey)) {}

std::vector<AccountSnapshot> AccountHistoryStore::accounts(const std::wstring& current_email) const {
    auto values = load();
    if (!current_email.empty()) {
        values.erase(
            std::remove_if(values.begin(), values.end(), [&](const AccountSnapshot& account) {
                return same_email(account.email, current_email);
            }),
            values.end());
    }
    std::sort(values.begin(), values.end(), [](const AccountSnapshot& lhs, const AccountSnapshot& rhs) {
        return lhs.last_seen_at > rhs.last_seen_at;
    });
    return values;
}

void AccountHistoryStore::save(const std::wstring& email, const std::wstring& status, std::chrono::system_clock::time_point seen_at) const {
    if (email.empty()) {
        return;
    }

    auto values = load();
    values.erase(
        std::remove_if(values.begin(), values.end(), [&](const AccountSnapshot& account) {
            return same_email(account.email, email);
        }),
        values.end());
    values.push_back(AccountSnapshot{email, status, seen_at});
    persist(values);
}

void AccountHistoryStore::record_login(const std::wstring& email, const std::wstring& default_status, std::chrono::system_clock::time_point seen_at) const {
    if (email.empty()) {
        return;
    }

    auto values = load();
    auto existing = std::find_if(values.begin(), values.end(), [&](const AccountSnapshot& account) {
        return same_email(account.email, email);
    });
    const std::wstring status = existing == values.end() ? default_status : existing->status;
    values.erase(
        std::remove_if(values.begin(), values.end(), [&](const AccountSnapshot& account) {
            return same_email(account.email, email);
        }),
        values.end());
    values.push_back(AccountSnapshot{email, status, seen_at});
    persist(values);
}

void AccountHistoryStore::remove(const std::wstring& email) const {
    auto values = load();
    values.erase(
        std::remove_if(values.begin(), values.end(), [&](const AccountSnapshot& account) {
            return same_email(account.email, email);
        }),
        values.end());
    persist(values);
}

std::vector<AccountSnapshot> AccountHistoryStore::load() const {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, registry_subkey_.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return {};
    }

    DWORD type = 0;
    DWORD bytes = 0;
    if (RegQueryValueExW(key, kHistoryValue, nullptr, &type, nullptr, &bytes) != ERROR_SUCCESS || type != REG_SZ || bytes == 0) {
        RegCloseKey(key);
        return {};
    }

    std::wstring data(bytes / sizeof(wchar_t), L'\0');
    if (RegQueryValueExW(key, kHistoryValue, nullptr, nullptr, reinterpret_cast<LPBYTE>(data.data()), &bytes) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return {};
    }
    RegCloseKey(key);

    if (!data.empty() && data.back() == L'\0') {
        data.pop_back();
    }

    std::vector<AccountSnapshot> values;
    std::wstringstream lines(data);
    std::wstring line;
    while (std::getline(lines, line)) {
        const size_t first = line.find(L'\t');
        const size_t second = first == std::wstring::npos ? std::wstring::npos : line.find(L'\t', first + 1);
        if (first == std::wstring::npos || second == std::wstring::npos) {
            continue;
        }

        const std::wstring email = line.substr(0, first);
        const std::wstring status = line.substr(first + 1, second - first - 1);
        long long seconds = 0;
        try {
            seconds = std::stoll(line.substr(second + 1));
        } catch (...) {
            continue;
        }
        if (!email.empty()) {
            values.push_back(AccountSnapshot{email, status, from_unix_seconds(seconds)});
        }
    }
    return values;
}

void AccountHistoryStore::persist(const std::vector<AccountSnapshot>& accounts) const {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, registry_subkey_.c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return;
    }

    std::wstring data;
    for (const auto& account : accounts) {
        data += encode_field(account.email);
        data += L'\t';
        data += encode_field(account.status);
        data += L'\t';
        data += std::to_wstring(to_unix_seconds(account.last_seen_at));
        data += L'\n';
    }

    RegSetValueExW(
        key,
        kHistoryValue,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(data.c_str()),
        static_cast<DWORD>((data.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
}

}  // namespace restime
