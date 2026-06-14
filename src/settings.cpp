#include "settings.h"

#include <Windows.h>

namespace restime {

namespace {

constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"Restime";

}  // namespace

std::wstring Settings::executable_path() const {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return path;
}

namespace {

std::wstring startup_command(const std::wstring& path) {
    return L"\"" + path + L"\"";
}

}  // namespace

bool Settings::start_with_windows() const {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t value[MAX_PATH]{};
    DWORD bytes = sizeof(value);
    const auto result = RegQueryValueExW(key, kRunValue, nullptr, nullptr, reinterpret_cast<LPBYTE>(value), &bytes);
    RegCloseKey(key);
    const std::wstring path = executable_path();
    return result == ERROR_SUCCESS && (path == value || startup_command(path) == value);
}

void Settings::set_start_with_windows(bool enabled) const {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return;
    }

    if (enabled) {
        const std::wstring path = startup_command(executable_path());
        RegSetValueExW(
            key,
            kRunValue,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(path.c_str()),
            static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kRunValue);
    }

    RegCloseKey(key);
}

}  // namespace restime
