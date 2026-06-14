# Restime

Lightweight macOS menu bar app for Codex usage.

Menu bar format:

```text
5h X% HH:mm | W Y% d/M
```

The app reads the latest Codex `rate_limits` event from `~/.codex/sessions/**/*.jsonl`.
It does not spawn the Codex CLI during refresh, keeping CPU and memory usage low.

## Build

```sh
make app
```

The generated app is:

```text
.build/Restime.app
```

The release build is universal and runs on both Apple Silicon and Intel Macs.

## Share

```sh
make dmg
```

The shareable disk image is:

```text
dist/Restime-0.1.0-universal.dmg
```

Open the DMG and drag Restime into Applications. The app is ad-hoc signed, so
macOS may require recipients to right-click Restime and choose Open the first
time. Distribution without that warning requires an Apple Developer ID
certificate and notarization.

## Run

```sh
make run
```

Clicking the menu bar item refreshes usage once before showing the dropdown.

## Install

```sh
make install
```

This copies the app to `/Applications/Restime.app`, so it can be opened from
Spotlight, Launchpad, or Finder without using Terminal.

## Icon

```sh
make icon
```

The app icon is generated from `Tools/GenerateAppIcon.swift` into
`Assets/AppIcon.icns`.
