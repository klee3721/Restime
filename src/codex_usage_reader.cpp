#include "codex_usage_reader.h"

#include "simple_json.h"

#include <Windows.h>
#include <winhttp.h>

#include <string>
#include <vector>

namespace restime {

namespace {

struct InternetHandle {
    HINTERNET value = nullptr;
    ~InternetHandle() {
        if (value) {
            WinHttpCloseHandle(value);
        }
    }
    InternetHandle() = default;
    explicit InternetHandle(HINTERNET handle) : value(handle) {}
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
};

}  // namespace

std::optional<CodexUsage> CodexUsageApiClient::read_usage(const std::string& access_token, std::wstring* error) const {
    InternetHandle session(WinHttpOpen(
        L"Restime/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));
    if (!session.value) {
        if (error) *error = L"Could not open WinHTTP session";
        return std::nullopt;
    }

    WinHttpSetTimeouts(session.value, 2000, 2000, 5000, 5000);

    InternetHandle connect(WinHttpConnect(session.value, L"chatgpt.com", INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connect.value) {
        if (error) *error = L"Could not connect to chatgpt.com";
        return std::nullopt;
    }

    InternetHandle request(WinHttpOpenRequest(
        connect.value,
        L"GET",
        L"/backend-api/wham/usage",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE));
    if (!request.value) {
        if (error) *error = L"Could not create usage request";
        return std::nullopt;
    }

    const std::wstring headers =
        L"Authorization: Bearer " + utf8_to_wide(access_token) + L"\r\n"
        L"Accept: application/json\r\n";

    if (!WinHttpSendRequest(
            request.value,
            headers.c_str(),
            static_cast<DWORD>(headers.size()),
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) {
        if (error) *error = L"Codex usage request failed";
        return std::nullopt;
    }

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    WinHttpQueryHeaders(
        request.value,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status,
        &status_size,
        WINHTTP_NO_HEADER_INDEX);
    if (status < 200 || status >= 300) {
        if (error) *error = L"Codex usage API returned HTTP " + std::to_wstring(status);
        return std::nullopt;
    }

    std::string body;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.value, &available)) {
            if (error) *error = L"Could not read usage response";
            return std::nullopt;
        }
        if (available == 0) {
            break;
        }
        if (body.size() + available > 1024 * 1024) {
            if (error) *error = L"Usage response is too large";
            return std::nullopt;
        }
        const size_t old_size = body.size();
        body.resize(old_size + available);
        DWORD read = 0;
        if (!WinHttpReadData(request.value, body.data() + old_size, available, &read)) {
            if (error) *error = L"Could not read usage response data";
            return std::nullopt;
        }
        body.resize(old_size + read);
    }

    auto usage = json::parse_api_usage(body);
    if (!usage && error) {
        *error = L"Could not parse Codex usage response";
    }
    return usage;
}

}  // namespace restime
