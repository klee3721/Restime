#include "tray_controller.h"

#include "usage_formatter.h"

#include <algorithm>
#include <iterator>
#include <strsafe.h>

namespace restime {

TrayController::TrayController(HWND window) : window_(window) {
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = window_;
    nid_.uID = 1;
    nid_.uCallbackMessage = WM_RESTIME_TRAY;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
}

TrayController::~TrayController() {
    remove();
}

void TrayController::add() {
    RefreshSnapshot empty;
    update(empty);
    Shell_NotifyIconW(NIM_ADD, &nid_);
    nid_.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);
    added_ = true;
}

void TrayController::remove() {
    if (added_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        added_ = false;
    }
    destroy_icon();
}

void TrayController::restore_after_explorer_restart() {
    added_ = false;
    Shell_NotifyIconW(NIM_ADD, &nid_);
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);
    added_ = true;
}

void TrayController::update(const RefreshSnapshot& snapshot) {
    destroy_icon();
    icon_ = create_icon(snapshot);
    nid_.hIcon = icon_;

    const auto tooltip = tooltip_text(snapshot);
    StringCchCopyW(nid_.szTip, std::size(nid_.szTip), tooltip.c_str());

    if (added_) {
        Shell_NotifyIconW(NIM_MODIFY, &nid_);
    }
}

void TrayController::show_menu(const RefreshSnapshot& snapshot) {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    append_disabled(menu, L"Codex Usage");
    if (!snapshot.account_email.empty()) {
        append_disabled(menu, L"Account: " + snapshot.account_email);
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    if (snapshot.usage) {
        append_disabled(menu, L"5h remaining: " + std::to_wstring(snapshot.usage->five_hour_remaining_percent) + L"%");
        append_disabled(menu, L"5h reset: " + format_time(snapshot.usage->five_hour_reset));
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        append_disabled(menu, L"Weekly remaining: " + std::to_wstring(snapshot.usage->weekly_remaining_percent) + L"%");
        append_disabled(menu, L"Weekly reset: " + format_day(snapshot.usage->weekly_reset));
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        append_disabled(menu, L"Last updated: " + format_time(snapshot.usage->observed_at));
        append_disabled(menu, L"Source: " + source_label(snapshot.usage->source));
    } else {
        append_disabled(menu, snapshot.error.empty() ? L"No usage data" : snapshot.error);
    }

    if (!snapshot.error.empty() && snapshot.usage) {
        append_disabled(menu, L"Last refresh failed: " + snapshot.error);
    }

    const auto accounts = account_history_.accounts(snapshot.account_email);
    removable_accounts_.clear();
    if (!accounts.empty()) {
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        append_disabled(menu, L"Accounts on this PC");

        UINT index = 0;
        for (const auto& account : accounts) {
            if (index > CommandRemoveAccountLast - CommandRemoveAccountBase) {
                break;
            }

            HMENU submenu = CreatePopupMenu();
            if (submenu) {
                AppendMenuW(submenu, MF_STRING, CommandRemoveAccountBase + index, L"Remove from list");
                const std::wstring title = account.email + L" - " + account.status;
                AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(submenu), title.c_str());
                removable_accounts_.push_back(account.email);
                ++index;
            }
        }
    }

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, CommandRefresh, L"Refresh now");
    AppendMenuW(
        menu,
        MF_STRING | (settings_.start_with_windows() ? MF_CHECKED : MF_UNCHECKED),
        CommandStartWithWindows,
        L"Start with Windows");
    AppendMenuW(menu, MF_STRING, CommandAbout, L"About");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, CommandQuit, L"Quit");

    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(window_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, point.x, point.y, 0, window_, nullptr);
    PostMessageW(window_, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

bool TrayController::remove_account_command(UINT command) {
    if (command < CommandRemoveAccountBase || command > CommandRemoveAccountLast) {
        return false;
    }

    const size_t index = command - CommandRemoveAccountBase;
    if (index >= removable_accounts_.size()) {
        return true;
    }

    account_history_.remove(removable_accounts_[index]);
    return true;
}

HICON TrayController::create_icon(const RefreshSnapshot& snapshot) const {
    const int size = GetSystemMetrics(SM_CXSMICON);
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP color = CreateCompatibleBitmap(screen, size, size);
    HBITMAP old = static_cast<HBITMAP>(SelectObject(dc, color));

    int percent = -1;
    COLORREF fill = RGB(110, 118, 129);
    if (snapshot.usage) {
        percent = snapshot.usage->five_hour_remaining_percent;
        fill = percent < 20 ? RGB(211, 47, 47) : percent < 50 ? RGB(245, 166, 35) : RGB(35, 134, 54);
    }

    HBRUSH brush = CreateSolidBrush(fill);
    RECT rect{0, 0, size, size};
    FillRect(dc, &rect, brush);
    DeleteObject(brush);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    HFONT font = CreateFontW(
        -MulDiv(8, GetDeviceCaps(screen, LOGPIXELSY), 72),
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI");
    HFONT old_font = static_cast<HFONT>(SelectObject(dc, font));

    std::wstring text = percent < 0 ? L"--" : std::to_wstring(std::min(percent, 99));
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, old_font);
    DeleteObject(font);
    SelectObject(dc, old);

    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    ICONINFO info{};
    info.fIcon = TRUE;
    info.hbmColor = color;
    info.hbmMask = mask;
    HICON icon = CreateIconIndirect(&info);

    DeleteObject(mask);
    DeleteObject(color);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    return icon ? icon : LoadIconW(nullptr, IDI_APPLICATION);
}

void TrayController::destroy_icon() {
    if (icon_) {
        DestroyIcon(icon_);
        icon_ = nullptr;
    }
}

void TrayController::append_disabled(HMENU menu, const std::wstring& text) const {
    AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, text.c_str());
}

}  // namespace restime
