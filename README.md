# Restime

Restime is a lightweight Codex usage monitor for desktop.

It has two native apps:

- `windows/`: Windows system tray app built with C++/Win32 for minimal RAM and CPU usage.
- `macos/`: macOS menu bar app built with Swift/AppKit.

Both versions read Codex auth/session data from the local `.codex` directory and avoid spawning the Codex CLI during refresh.

## Windows

```powershell
cd windows
.\tools\build-release.ps1
```

The executable is created at:

```text
windows\build\release\Restime.exe
```

## macOS

```sh
cd macos
make app
```

The app bundle is created at:

```text
macos/.build/Restime.app
```
