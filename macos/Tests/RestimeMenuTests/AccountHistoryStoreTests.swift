import Foundation
import Testing
@testable import RestimeMenu

@Test func accountHistorySavesUpdatesExcludesAndRemovesAccounts() throws {
    let suiteName = "AccountHistoryStoreTests-\(UUID().uuidString)"
    let defaults = try #require(UserDefaults(suiteName: suiteName))
    defer { defaults.removePersistentDomain(forName: suiteName) }

    let store = AccountHistoryStore(defaults: defaults, storageKey: "testHistory")
    let older = Date(timeIntervalSince1970: 100)
    let newer = Date(timeIntervalSince1970: 200)

    store.save(email: "first@example.com", status: "old", seenAt: older)
    store.save(email: "second@example.com", status: "second", seenAt: newer)
    store.save(email: "FIRST@example.com", status: "updated", seenAt: newer.addingTimeInterval(1))

    let accounts = store.accounts()
    #expect(accounts.count == 2)
    #expect(accounts.first?.email == "FIRST@example.com")
    #expect(accounts.first?.status == "updated")
    #expect(store.accounts(excluding: "first@example.com").map(\.email) == ["second@example.com"])

    store.remove(email: "SECOND@example.com")
    #expect(store.accounts().map(\.email) == ["FIRST@example.com"])
}

@Test func loggingBackInRestoresDeletedAccountWithoutReplacingItsLastStatus() throws {
    let suiteName = "AccountHistoryStoreTests-\(UUID().uuidString)"
    let defaults = try #require(UserDefaults(suiteName: suiteName))
    defer { defaults.removePersistentDomain(forName: suiteName) }

    let store = AccountHistoryStore(defaults: defaults, storageKey: "testHistory")
    store.save(email: "friend@example.com", status: "last usage")
    store.recordLogin(email: "friend@example.com", defaultStatus: "empty")
    #expect(store.accounts().first?.status == "last usage")

    store.remove(email: "friend@example.com")
    #expect(store.accounts().isEmpty)

    store.recordLogin(email: "friend@example.com", defaultStatus: "empty")
    #expect(store.accounts().first?.status == "empty")
}
