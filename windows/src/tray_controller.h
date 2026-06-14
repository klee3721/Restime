#pragma once

#include "settings.h"
#include "usage.h"

#include <Windows.h>
#include <shellapi.h>

namespace restime {

constexpr UINT WM_RESTIME_TRAY = WM_APP + 10;

enum TrayCommand : UINT {
    CommandRefresh = 1001,
    CommandStartWithWindows = 1002,
    CommandAbout = 1003,
    CommandQuit = 1004
};

class TrayController {
public:
    explicit TrayController(HWND window);
    ~TrayController();

    void add();
    void remove();
    void restore_after_explorer_restart();
    void update(const RefreshSnapshot& snapshot);
    void show_menu(const RefreshSnapshot& snapshot);

private:
    HICON create_icon(const RefreshSnapshot& snapshot) const;
    void destroy_icon();
    void append_disabled(HMENU menu, const std::wstring& text) const;

    HWND window_ = nullptr;
    NOTIFYICONDATAW nid_{};
    HICON icon_ = nullptr;
    bool added_ = false;
    Settings settings_;
};

}  // namespace restime
