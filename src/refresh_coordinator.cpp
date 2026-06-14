#include "refresh_coordinator.h"

#include "codex_auth_reader.h"
#include "codex_usage_reader.h"
#include "session_usage_reader.h"

#include <utility>

namespace restime {

RefreshCoordinator::RefreshCoordinator(HWND target_window) : target_window_(target_window) {}

RefreshCoordinator::~RefreshCoordinator() {
    stop();
}

void RefreshCoordinator::start() {
    worker_ = std::thread([this] { run(); });
}

void RefreshCoordinator::stop() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
        refresh_requested_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void RefreshCoordinator::request_refresh() {
    {
        std::lock_guard lock(mutex_);
        refresh_requested_ = true;
    }
    cv_.notify_all();
}

void RefreshCoordinator::run() {
    auto next_refresh = std::chrono::steady_clock::now();

    for (;;) {
        {
            std::unique_lock lock(mutex_);
            cv_.wait_until(lock, next_refresh, [this] { return stopping_ || refresh_requested_; });
            if (stopping_) {
                break;
            }
            refresh_requested_ = false;
        }

        auto* snapshot = new RefreshSnapshot(refresh_once());
        {
            std::lock_guard lock(mutex_);
            if (stopping_) {
                delete snapshot;
                break;
            }
        }
        if (!PostMessageW(target_window_, WM_RESTIME_REFRESH_DONE, 0, reinterpret_cast<LPARAM>(snapshot))) {
            delete snapshot;
        }
        next_refresh = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    }
}

RefreshSnapshot RefreshCoordinator::refresh_once() const {
    RefreshSnapshot snapshot;
    std::wstring auth_error;
    const CodexAuthReader auth_reader;
    const auto auth = auth_reader.read(&auth_error);
    if (!auth) {
        snapshot.error = auth_error.empty() ? L"No Codex account signed in" : auth_error;
        return snapshot;
    }

    snapshot.account_email = auth->email;

    std::wstring api_error;
    const CodexUsageApiClient api;
    if (auto usage = api.read_usage(auth->access_token, &api_error)) {
        snapshot.usage = std::move(usage);
        return snapshot;
    }

    std::wstring local_error;
    const SessionUsageReader sessions;
    if (auto usage = sessions.read_latest(&local_error)) {
        snapshot.usage = std::move(usage);
        snapshot.error = api_error;
        return snapshot;
    }

    snapshot.error = !api_error.empty() ? api_error : local_error;
    if (!local_error.empty() && !api_error.empty()) {
        snapshot.error += L"; " + local_error;
    }
    return snapshot;
}

}  // namespace restime
