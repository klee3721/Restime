#pragma once

#include "usage.h"

#include <Windows.h>

#include <condition_variable>
#include <mutex>
#include <thread>

namespace restime {

constexpr UINT WM_RESTIME_REFRESH_DONE = WM_APP + 1;

class RefreshCoordinator {
public:
    explicit RefreshCoordinator(HWND target_window);
    ~RefreshCoordinator();

    void start();
    void stop();
    void request_refresh();

private:
    void run();
    RefreshSnapshot refresh_once() const;

    HWND target_window_ = nullptr;
    std::thread worker_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stopping_ = false;
    bool refresh_requested_ = false;
};

}  // namespace restime
