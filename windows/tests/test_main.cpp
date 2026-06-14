#include "codex_auth_reader.h"
#include "account_history_store.h"
#include "simple_json.h"
#include "usage_formatter.h"

#include <Windows.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::wstring temp_file_path(const wchar_t* name) {
    wchar_t dir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, dir);
    return std::wstring(dir) + name;
}

void write_file(const std::wstring& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    file << text;
}

void parser_tests() {
    const std::string api = R"({
      "rate_limit": {
        "primary_window": {"used_percent": 77.0, "reset_at": 1780902290},
        "secondary_window": {"used_percent": 45.0, "reset_at": 1781375249}
      }
    })";
    const auto api_usage = restime::json::parse_api_usage(api);
    expect(api_usage.has_value(), "api usage parses");
    expect(api_usage->five_hour_remaining_percent == 23, "api primary remaining");
    expect(api_usage->weekly_remaining_percent == 55, "api weekly remaining");

    const std::string line = R"({"timestamp":"2026-06-08T06:54:31.939Z","payload":{"rate_limits":{"primary":{"used_percent":120.0,"resets_at":1780902290},"secondary":{"used_percent":-1.0,"resets_at":1781375249}}}})";
    const auto local_usage = restime::json::parse_session_usage_line(line, L"session.jsonl");
    expect(local_usage.has_value(), "local usage parses");
    expect(local_usage->five_hour_remaining_percent == 0, "remaining clamps low");
    expect(local_usage->weekly_remaining_percent == 100, "remaining clamps high");
    expect(restime::menu_title(local_usage).find(L"5h 0%") != std::wstring::npos, "menu title formats");
}

void auth_tests() {
    const auto path = temp_file_path(L"restime-auth-test.json");
    const std::string payload = "eyJlbWFpbCI6ImZyaWVuZEBleGFtcGxlLmNvbSJ9";
    write_file(path, "{\"tokens\":{\"access_token\":\"header." + payload + ".sig\",\"id_token\":\"header." + payload + ".sig\"}}");

    const restime::CodexAuthReader reader(path);
    std::wstring error;
    const auto auth = reader.read(&error);
    expect(auth.has_value(), "auth parses");
    expect(auth->email == L"friend@example.com", "email parses from id token");
    expect(!auth->access_token.empty(), "access token parses");
    DeleteFileW(path.c_str());
}

void account_history_tests() {
    const std::wstring key = L"Software\\RestimeTests\\" + std::to_wstring(GetTickCount64());
    const restime::AccountHistoryStore store(key);
    const auto older = std::chrono::system_clock::time_point(std::chrono::seconds(100));
    const auto newer = std::chrono::system_clock::time_point(std::chrono::seconds(200));

    store.save(L"first@example.com", L"old", older);
    store.save(L"second@example.com", L"second", newer);
    store.save(L"FIRST@example.com", L"updated", newer + std::chrono::seconds(1));

    auto accounts = store.accounts();
    expect(accounts.size() == 2, "history deduplicates accounts");
    expect(accounts[0].email == L"FIRST@example.com", "history sorts newest first");
    expect(accounts[0].status == L"updated", "history updates status");

    auto excluding = store.accounts(L"first@example.com");
    expect(excluding.size() == 1 && excluding[0].email == L"second@example.com", "history excludes current account");

    store.remove(L"SECOND@example.com");
    accounts = store.accounts();
    expect(accounts.size() == 1 && accounts[0].email == L"FIRST@example.com", "history removes case-insensitively");

    RegDeleteTreeW(HKEY_CURRENT_USER, key.c_str());
}

}  // namespace

int main() {
    parser_tests();
    auth_tests();
    account_history_tests();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
