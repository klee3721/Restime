import AppKit
import Foundation

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate, NSMenuDelegate {
    private let statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
    private let reader = CodexUsageReader()
    private let accountHistory = AccountHistoryStore()
    private let menu = NSMenu()
    private var timer: Timer?
    private var latestUsage: CodexUsage?
    private var accountEmail: String?
    private var latestError: String?
    private var isRefreshing = false

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.accessory)
        configureStatusItem()
        configureMenu()
        refreshUsage()

        timer = Timer.scheduledTimer(
            timeInterval: 15,
            target: self,
            selector: #selector(timerFired(_:)),
            userInfo: nil,
            repeats: true
        )
        timer?.tolerance = 5
    }

    func applicationWillTerminate(_ notification: Notification) {
        timer?.invalidate()
    }

    func menuWillOpen(_ menu: NSMenu) {
        refreshUsage()
        rebuildMenu()
    }

    private func configureStatusItem() {
        statusItem.button?.font = NSFont.monospacedDigitSystemFont(ofSize: NSFont.systemFontSize, weight: .regular)
        statusItem.button?.title = UsageFormat.menuTitle(for: nil)
        statusItem.menu = menu
    }

    private func configureMenu() {
        menu.delegate = self
        rebuildMenu()
    }

    @objc private func refreshNow(_ sender: Any?) {
        refreshUsage()
        rebuildMenu()
    }

    @objc private func timerFired(_ sender: Timer) {
        refreshUsage()
    }

    @objc private func quit(_ sender: Any?) {
        NSApp.terminate(nil)
    }

    @objc private func removeAccount(_ sender: NSMenuItem) {
        guard let email = sender.representedObject as? String else {
            return
        }
        accountHistory.remove(email: email)
        rebuildMenu()
    }

    private func refreshUsage() {
        guard !isRefreshing else { return }
        isRefreshing = true
        defer { isRefreshing = false }

        autoreleasepool {
            updateCurrentAccount(reader.readAccountEmail())

            guard let accountEmail else {
                latestError = "No Codex account signed in"
                statusItem.button?.title = UsageFormat.menuTitle(for: nil)
                return
            }

            do {
                let usage = try reader.readLatestUsage(allowLocalFallback: false)
                latestUsage = usage
                latestError = nil
                statusItem.button?.title = UsageFormat.menuTitle(for: usage)
                accountHistory.save(email: accountEmail, status: UsageFormat.menuTitle(for: usage))
            } catch {
                latestError = error.localizedDescription
                if latestUsage == nil {
                    statusItem.button?.title = UsageFormat.menuTitle(for: nil)
                } else {
                    accountHistory.save(email: accountEmail, status: UsageFormat.menuTitle(for: latestUsage))
                }
            }
        }
    }

    private func updateCurrentAccount(_ detectedEmail: String?) {
        let normalizedDetected = detectedEmail?.lowercased()
        let normalizedCurrent = accountEmail?.lowercased()
        guard normalizedDetected != normalizedCurrent else {
            return
        }

        if let accountEmail, let latestUsage {
            accountHistory.save(email: accountEmail, status: UsageFormat.menuTitle(for: latestUsage))
        }

        accountEmail = detectedEmail
        latestUsage = nil
        latestError = nil
        statusItem.button?.title = UsageFormat.menuTitle(for: nil)

        if let detectedEmail {
            accountHistory.recordLogin(
                email: detectedEmail,
                defaultStatus: UsageFormat.menuTitle(for: nil)
            )
        }
    }

    private func rebuildMenu() {
        menu.removeAllItems()

        if let usage = latestUsage {
            let title = NSMenuItem(title: "Codex Usage", action: nil, keyEquivalent: "")
            title.isEnabled = false
            menu.addItem(title)
            if let accountEmail {
                menu.addItem(disabledItem("Account: \(accountEmail)"))
            }
            menu.addItem(.separator())

            menu.addItem(disabledItem("5h remaining: \(usage.fiveHourRemainingPercent)%"))
            menu.addItem(disabledItem("5h reset: \(UsageFormat.time(usage.fiveHourResetDate))"))
            menu.addItem(.separator())
            menu.addItem(disabledItem("Weekly remaining: \(usage.weeklyRemainingPercent)%"))
            menu.addItem(disabledItem("Weekly reset: \(UsageFormat.day(usage.weeklyResetDate))"))
            menu.addItem(.separator())
            menu.addItem(disabledItem("Last updated: \(UsageFormat.time(usage.observedAt))"))
            menu.addItem(disabledItem("Source: \(sourceLabel(for: usage.sourcePath))"))
            if let latestError {
                menu.addItem(disabledItem("Last refresh failed: \(latestError)"))
            }
        } else {
            menu.addItem(disabledItem("Codex Usage"))
            if let accountEmail {
                menu.addItem(disabledItem("Account: \(accountEmail)"))
            }
            menu.addItem(.separator())
            menu.addItem(disabledItem(latestError ?? "No usage data"))
        }

        addAccountHistoryItems()
        menu.addItem(.separator())
        menu.addItem(NSMenuItem(title: "Refresh Now", action: #selector(refreshNow(_:)), keyEquivalent: "r"))
        menu.addItem(NSMenuItem(title: "Quit Restime", action: #selector(quit(_:)), keyEquivalent: "q"))
    }

    private func addAccountHistoryItems() {
        let accounts = accountHistory.accounts(excluding: accountEmail)
        guard !accounts.isEmpty else {
            return
        }

        menu.addItem(.separator())
        menu.addItem(disabledItem("Accounts on this Mac"))

        for account in accounts {
            let item = NSMenuItem(
                title: "\(account.email)\n\(account.status)",
                action: nil,
                keyEquivalent: ""
            )
            let actions = NSMenu(title: account.email)
            let remove = NSMenuItem(
                title: "Remove from list",
                action: #selector(removeAccount(_:)),
                keyEquivalent: ""
            )
            remove.target = self
            remove.representedObject = account.email
            actions.addItem(remove)
            item.submenu = actions
            menu.addItem(item)
        }
    }

    private func disabledItem(_ title: String) -> NSMenuItem {
        let item = NSMenuItem(title: title, action: nil, keyEquivalent: "")
        item.isEnabled = false
        return item
    }

    private func sourceLabel(for sourcePath: String) -> String {
        if sourcePath.contains("backend-api/wham/usage") {
            return "Codex API"
        }

        return "Local log"
    }
}
