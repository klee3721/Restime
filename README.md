# Restime for Windows

Lightweight Windows system tray app for Codex usage.

This project intentionally uses native C++/Win32 APIs instead of Electron,
WebView, WPF, or WinUI so the app can stay small in RAM while idle.

## Build

Install:

- Visual Studio 2022 Build Tools
- Desktop development with C++
- Windows 10/11 SDK
- CMake
- Ninja

Then build:

```powershell
.\tools\build-release.ps1
```

Run the command from "x64 Native Tools Command Prompt for VS" or
"Developer PowerShell for VS" so `cl.exe` and the Windows SDK are available.

The executable will be:

```text
build\release\Restime.exe
```

## Run Tests

```powershell
.\tools\test.ps1
```

## Behavior

- Shows a tray icon in the Windows notification area.
- Tooltip displays `5h X% HH:mm | W Y% d/M`.
- Reads Codex auth from `%USERPROFILE%\.codex\auth.json`.
- Calls `https://chatgpt.com/backend-api/wham/usage`.
- Falls back to `%USERPROFILE%\.codex\sessions\**\*.jsonl` when the API fails.
- Does not spawn the Codex CLI.
- Supports `Refresh now`, `Start with Windows`, and `Quit`.
