import Foundation

struct AccountSnapshot: Codable, Equatable {
    let email: String
    let status: String
    let lastSeenAt: Date
}

final class AccountHistoryStore {
    private let defaults: UserDefaults
    private let storageKey: String
    private let encoder = JSONEncoder()
    private let decoder = JSONDecoder()

    init(
        defaults: UserDefaults = .standard,
        storageKey: String = "accountHistory"
    ) {
        self.defaults = defaults
        self.storageKey = storageKey
    }

    func accounts(excluding currentEmail: String? = nil) -> [AccountSnapshot] {
        let currentEmail = currentEmail?.lowercased()
        return load()
            .filter { $0.email.lowercased() != currentEmail }
            .sorted { $0.lastSeenAt > $1.lastSeenAt }
    }

    func save(email: String, status: String, seenAt: Date = Date()) {
        var accounts = load()
        accounts.removeAll { $0.email.caseInsensitiveCompare(email) == .orderedSame }
        accounts.append(AccountSnapshot(email: email, status: status, lastSeenAt: seenAt))
        persist(accounts)
    }

    func recordLogin(email: String, defaultStatus: String, seenAt: Date = Date()) {
        var accounts = load()
        let existing = accounts.first { $0.email.caseInsensitiveCompare(email) == .orderedSame }
        accounts.removeAll { $0.email.caseInsensitiveCompare(email) == .orderedSame }
        accounts.append(AccountSnapshot(
            email: email,
            status: existing?.status ?? defaultStatus,
            lastSeenAt: seenAt
        ))
        persist(accounts)
    }

    func remove(email: String) {
        var accounts = load()
        accounts.removeAll { $0.email.caseInsensitiveCompare(email) == .orderedSame }
        persist(accounts)
    }

    private func load() -> [AccountSnapshot] {
        guard let data = defaults.data(forKey: storageKey),
              let accounts = try? decoder.decode([AccountSnapshot].self, from: data) else {
            return []
        }
        return accounts
    }

    private func persist(_ accounts: [AccountSnapshot]) {
        guard let data = try? encoder.encode(accounts) else {
            return
        }
        defaults.set(data, forKey: storageKey)
    }
}
