# Restime

Restime is a lightweight desktop monitor for Codex usage limits.

It runs as a native menu bar/tray utility, reads your local Codex auth/session
data, and avoids spawning the Codex CLI during refresh so idle RAM and CPU stay
low.

## Downloads

Get the latest builds from the GitHub Releases page:

- Windows: `Restime-0.1.0-windows-x64.exe`
- macOS: `Restime-0.1.0-macos-universal.dmg`
- Checksums: `checksums-sha256.txt`

## Platforms

| Platform | Location | UI | Tech |
|---|---|---|---|
| Windows | `windows/` | System tray | C++20 + Win32 |
| macOS | `macos/` | Menu bar | Swift + AppKit |

## What It Shows

```text
5h X% HH:mm | W Y% d/M
```

- 5-hour remaining percent and reset time
- Weekly remaining percent and reset day
- Current Codex account
- Last refresh status
- Data source: Codex API or local session log

## How It Works

Restime reads:

```text
~/.codex/auth.json
~/.codex/sessions/**/*.jsonl
```

On Windows, the equivalent path is:

```text
%USERPROFILE%\.codex
```

The app first tries the Codex usage API. If that fails, it falls back to recent
local session logs.

## Install

### Windows

Download `Restime-0.1.0-windows-x64.exe` from Releases and run it.

The app appears in the notification area near the clock. If Windows hides it,
open the `^` overflow menu and drag Restime into the visible tray area.

### macOS

Download `Restime-0.1.0-macos-universal.dmg` from Releases, open it, and drag
Restime into Applications.

The app is ad-hoc signed. macOS may require right-clicking Restime and choosing
Open the first time.

## Build From Source

### Windows

Requirements:

- Visual Studio 2022 Build Tools
- Desktop development with C++
- CMake
- Ninja

```powershell
cd windows
.\tools\build-release.ps1
```

Output:

```text
windows\build\release\Restime.exe
```

### macOS

Requirements:

- macOS 13+
- Xcode command line tools
- Swift 6+

```sh
cd macos
make app
```

Output:

```text
macos/.build/Restime.app
```

To create the DMG:

```sh
make dmg
```

## Privacy

Restime reads Codex auth/session files locally so it can display usage. It does
not store access tokens in app settings, and it does not send usage data to any
third-party service.
