.PHONY: build test icon app dmg install run clean

APP_NAME := Restime
VERSION := 0.1.0
BUNDLE := .build/$(APP_NAME).app
EXECUTABLE := .build/apple/Products/Release/RestimeMenu
INSTALLED_APP := /Applications/$(APP_NAME).app
RELEASE_FLAGS := -c release --arch arm64 --arch x86_64 -Xswiftc -Osize -Xlinker -dead_strip
ICONSET := Assets/AppIcon.iconset
ICON := Assets/AppIcon.icns
DIST := dist
DMG_ROOT := .build/dmg-root
DMG := $(DIST)/$(APP_NAME)-$(VERSION)-universal.dmg

build:
	swift build $(RELEASE_FLAGS)

test:
	swift test

icon:
	mkdir -p Assets
	swift Tools/GenerateAppIcon.swift
	iconutil -c icns "$(ICONSET)" -o "$(ICON)"

app: build icon
	rm -rf "$(BUNDLE)"
	mkdir -p "$(BUNDLE)/Contents/MacOS"
	mkdir -p "$(BUNDLE)/Contents/Resources"
	cp "$(EXECUTABLE)" "$(BUNDLE)/Contents/MacOS/RestimeMenu"
	cp "$(ICON)" "$(BUNDLE)/Contents/Resources/AppIcon.icns"
	strip "$(BUNDLE)/Contents/MacOS/RestimeMenu"
	plutil -create xml1 "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundleName -string "$(APP_NAME)" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundleDisplayName -string "$(APP_NAME)" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundleIdentifier -string "local.restime.menu" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundleVersion -string "1" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundleShortVersionString -string "$(VERSION)" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundleExecutable -string "RestimeMenu" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundleIconFile -string "AppIcon" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert CFBundlePackageType -string "APPL" "$(BUNDLE)/Contents/Info.plist"
	plutil -insert LSUIElement -bool true "$(BUNDLE)/Contents/Info.plist"
	xattr -cr "$(BUNDLE)" 2>/dev/null || true
	codesign --force --sign - "$(BUNDLE)"

dmg: app
	rm -rf "$(DMG_ROOT)"
	mkdir -p "$(DMG_ROOT)" "$(DIST)"
	cp -R "$(BUNDLE)" "$(DMG_ROOT)/$(APP_NAME).app"
	ln -s /Applications "$(DMG_ROOT)/Applications"
	rm -f "$(DMG)"
	hdiutil create -volname "$(APP_NAME)" -srcfolder "$(DMG_ROOT)" -ov -format UDZO "$(DMG)"
	@echo "Created $(DMG)"

install: app
	rm -rf "$(INSTALLED_APP)"
	cp -R "$(BUNDLE)" "$(INSTALLED_APP)"
	xattr -dr com.apple.quarantine "$(INSTALLED_APP)" 2>/dev/null || true

run: app
	open "$(BUNDLE)"

clean:
	rm -rf .build dist
