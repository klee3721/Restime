#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace restime {

struct AccountSnapshot {
    std::wstring email;
    std::wstring status;
    std::chrono::system_clock::time_point last_seen_at{};
};

class AccountHistoryStore {
public:
    explicit AccountHistoryStore(std::wstring registry_subkey = L"Software\\Restime");

    std::vector<AccountSnapshot> accounts(const std::wstring& current_email = L"") const;
    void save(const std::wstring& email, const std::wstring& status, std::chrono::system_clock::time_point seen_at = std::chrono::system_clock::now()) const;
    void record_login(const std::wstring& email, const std::wstring& default_status, std::chrono::system_clock::time_point seen_at = std::chrono::system_clock::now()) const;
    void remove(const std::wstring& email) const;

private:
    std::vector<AccountSnapshot> load() const;
    void persist(const std::vector<AccountSnapshot>& accounts) const;

    std::wstring registry_subkey_;
};

bool same_email(const std::wstring& lhs, const std::wstring& rhs);

}  // namespace restime
