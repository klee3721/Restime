#pragma once

#include "refresh_coordinator.h"
#include "settings.h"
#include "tray_controller.h"
#include "usage.h"

#include <Windows.h>

#include <memory>

namespace restime {

class App {
public:
    App() = default;
    ~App();

    int run(HINSTANCE instance, int show_command);

private:
    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);

    bool create_window(HINSTANCE instance);
    void handle_command(UINT command);
    void apply_snapshot(std::unique_ptr<RefreshSnapshot> snapshot);

    HINSTANCE instance_ = nullptr;
    HWND window_ = nullptr;
    HANDLE mutex_ = nullptr;
    UINT taskbar_created_message_ = 0;
    RefreshSnapshot snapshot_;
    std::unique_ptr<TrayController> tray_;
    std::unique_ptr<RefreshCoordinator> refresh_;
    Settings settings_;
};

}  // namespace restime
