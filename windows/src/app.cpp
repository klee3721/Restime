#include "app.h"

#include "usage_formatter.h"

#include <shellapi.h>
#include <utility>

namespace restime {

namespace {

constexpr wchar_t kWindowClass[] = L"RestimeTrayWindow";

}  // namespace

App::~App() {
    if (refresh_) {
        refresh_->stop();
    }
    tray_.reset();
    if (window_) {
        DestroyWindow(window_);
    }
    if (mutex_) {
        CloseHandle(mutex_);
    }
}

int App::run(HINSTANCE instance, int) {
    instance_ = instance;
    mutex_ = CreateMutexW(nullptr, TRUE, L"Local\\RestimeWindowsSingleInstance");
    if (!mutex_ || GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    if (!create_window(instance_)) {
        return 1;
    }

    taskbar_created_message_ = RegisterWindowMessageW(L"TaskbarCreated");
    tray_ = std::make_unique<TrayController>(window_);
    tray_->add();

    refresh_ = std::make_unique<RefreshCoordinator>(window_);
    refresh_->start();
    refresh_->request_refresh();

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

bool App::create_window(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = App::window_proc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"Could not register Restime window class.", L"Restime", MB_OK | MB_ICONERROR);
        return false;
    }

    window_ = CreateWindowExW(
        0,
        kWindowClass,
        L"Restime",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        nullptr,
        nullptr,
        instance,
        this);

    if (!window_) {
        const DWORD error = GetLastError();
        MessageBoxW(
            nullptr,
            (L"Could not create Restime hidden window. Error: " + std::to_wstring(error)).c_str(),
            L"Restime",
            MB_OK | MB_ICONERROR);
    }

    return window_ != nullptr;
}

LRESULT CALLBACK App::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    App* app = nullptr;
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        app = static_cast<App*>(create->lpCreateParams);
        app->window_ = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (app) {
        return app->handle_message(message, wparam, lparam);
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT App::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == taskbar_created_message_) {
        if (tray_) {
            tray_->restore_after_explorer_restart();
            tray_->update(snapshot_);
        }
        return 0;
    }

    switch (message) {
    case WM_COMMAND:
        handle_command(LOWORD(wparam));
        return 0;

    case WM_RESTIME_REFRESH_DONE:
        apply_snapshot(std::unique_ptr<RefreshSnapshot>(reinterpret_cast<RefreshSnapshot*>(lparam)));
        return 0;

    case WM_RESTIME_TRAY:
        if (LOWORD(lparam) == WM_CONTEXTMENU || LOWORD(lparam) == WM_RBUTTONUP ||
            LOWORD(lparam) == WM_LBUTTONUP || LOWORD(lparam) == NIN_SELECT ||
            LOWORD(lparam) == NIN_KEYSELECT) {
            if (tray_) {
                tray_->show_menu(snapshot_);
            }
        }
        return 0;

    case WM_DESTROY:
        if (tray_) {
            tray_->remove();
        }
        PostQuitMessage(0);
        return 0;

    case WM_NCDESTROY: {
        const LRESULT result = DefWindowProcW(window_, message, wparam, lparam);
        window_ = nullptr;
        return result;
    }

    default:
        return DefWindowProcW(window_, message, wparam, lparam);
    }
}

void App::handle_command(UINT command) {
    switch (command) {
    case CommandRefresh:
        if (refresh_) {
            refresh_->request_refresh();
        }
        break;
    case CommandStartWithWindows:
        settings_.set_start_with_windows(!settings_.start_with_windows());
        break;
    case CommandAbout:
        MessageBoxW(
            window_,
            L"Restime for Windows\nNative system tray Codex usage monitor.",
            L"Restime",
            MB_OK | MB_ICONINFORMATION);
        break;
    case CommandQuit:
        if (refresh_) {
            refresh_->stop();
        }
        DestroyWindow(window_);
        break;
    default:
        if (tray_ && tray_->remove_account_command(command)) {
            break;
        }
        break;
    }
}

void App::apply_snapshot(std::unique_ptr<RefreshSnapshot> snapshot) {
    if (!snapshot) {
        return;
    }

    if (!snapshot->usage && snapshot_.usage) {
        snapshot->usage = snapshot_.usage;
    }
    if (snapshot->account_email.empty()) {
        snapshot->account_email = snapshot_.account_email;
    }

    if (!snapshot->account_email.empty() && !same_email(snapshot->account_email, snapshot_.account_email)) {
        if (!snapshot_.account_email.empty() && snapshot_.usage) {
            account_history_.save(snapshot_.account_email, menu_title(snapshot_.usage));
        }
        account_history_.record_login(snapshot->account_email, menu_title(std::nullopt));
    }

    if (!snapshot->account_email.empty() && snapshot->usage) {
        account_history_.save(snapshot->account_email, menu_title(snapshot->usage));
    }

    snapshot_ = std::move(*snapshot);
    if (tray_) {
        tray_->update(snapshot_);
    }
}

}  // namespace restime
