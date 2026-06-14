#include "app.h"

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    restime::App app;
    return app.run(instance, show_command);
}
