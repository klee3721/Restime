#include "session_usage_reader.h"

#include "simple_json.h"

#include <Windows.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace restime {

namespace {

constexpr size_t kMaxFilesToScan = 12;
constexpr size_t kTailBytes = 4 * 1024 * 1024;

struct SessionFile {
    std::wstring path;
    FILETIME modified{};
};

bool newer_filetime(const FILETIME& a, const FILETIME& b) {
    return CompareFileTime(&a, &b) > 0;
}

void collect_jsonl_files(const std::wstring& root, std::vector<SessionFile>& files) {
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = root + L"\\*";
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        const std::wstring name = data.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        const std::wstring path = root + L"\\" + name;
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            collect_jsonl_files(path, files);
        } else if (path.size() >= 6 && path.substr(path.size() - 6) == L".jsonl") {
            files.push_back(SessionFile{path, data.ftLastWriteTime});
        }
    } while (FindNextFileW(find, &data));

    FindClose(find);
}

std::optional<CodexUsage> parse_tail(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::nullopt;
    }

    const auto end_pos = file.tellg();
    if (end_pos < std::streampos(0)) {
        return std::nullopt;
    }

    const auto size = static_cast<std::streamoff>(end_pos);
    const auto start = size > static_cast<std::streamoff>(kTailBytes)
                           ? size - static_cast<std::streamoff>(kTailBytes)
                           : std::streamoff{0};
    file.seekg(start, std::ios::beg);

    std::string text(static_cast<size_t>(size - start), '\0');
    file.read(text.data(), static_cast<std::streamsize>(text.size()));
    if (start != 0) {
        const size_t first_newline = text.find('\n');
        if (first_newline != std::string::npos) {
            text.erase(0, first_newline + 1);
        }
    }

    size_t end = text.size();
    while (end > 0) {
        size_t begin = text.rfind('\n', end - 1);
        if (begin == std::string::npos) {
            begin = 0;
        } else {
            ++begin;
        }

        if (end > begin && end - begin <= 1024 * 1024) {
            const std::string_view line(text.data() + begin, end - begin);
            if (auto usage = json::parse_session_usage_line(line, path)) {
                return usage;
            }
        }

        if (begin == 0) {
            break;
        }
        end = begin - 1;
    }

    return std::nullopt;
}

}  // namespace

std::wstring default_sessions_root() {
    wchar_t profile[MAX_PATH]{};
    const DWORD chars = GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
    if (chars == 0 || chars >= MAX_PATH) {
        return L".codex\\sessions";
    }
    return std::wstring(profile) + L"\\.codex\\sessions";
}

SessionUsageReader::SessionUsageReader(std::wstring sessions_root)
    : sessions_root_(sessions_root.empty() ? default_sessions_root() : std::move(sessions_root)) {}

std::optional<CodexUsage> SessionUsageReader::read_latest(std::wstring* error) const {
    const DWORD attributes = GetFileAttributesW(sessions_root_.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        if (error) *error = L"Sessions directory not found";
        return std::nullopt;
    }

    std::vector<SessionFile> files;
    collect_jsonl_files(sessions_root_, files);
    if (files.empty()) {
        if (error) *error = L"No Codex session files found";
        return std::nullopt;
    }

    std::sort(files.begin(), files.end(), [](const SessionFile& a, const SessionFile& b) {
        return newer_filetime(a.modified, b.modified);
    });

    const size_t limit = std::min(files.size(), kMaxFilesToScan);
    std::optional<CodexUsage> newest;
    for (size_t i = 0; i < limit; ++i) {
        if (auto usage = parse_tail(files[i].path)) {
            if (!newest || usage->observed_at > newest->observed_at) {
                newest = std::move(usage);
            }
        }
    }

    if (!newest && error) {
        *error = L"No rate limit event found in recent Codex sessions";
    }
    return newest;
}

}  // namespace restime
