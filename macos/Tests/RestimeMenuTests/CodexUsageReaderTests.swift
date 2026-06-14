import Foundation
import Testing
@testable import RestimeMenu

@Test func parsesRateLimitEvent() throws {
    let line = """
    {"timestamp":"2026-06-08T06:54:31.939Z","type":"event_msg","payload":{"type":"token_count","rate_limits":{"primary":{"used_percent":77.0,"window_minutes":300,"resets_at":1780902290},"secondary":{"used_percent":45.0,"window_minutes":10080,"resets_at":1781375249}}}}
    """

    let usage = try #require(CodexUsageReader().parseUsageLine(line, sourcePath: "/tmp/session.jsonl"))
    #expect(usage.fiveHourRemainingPercent == 23)
    #expect(usage.weeklyRemainingPercent == 55)
    #expect(UsageFormat.menuTitle(for: usage).contains("5h 23%"))
    #expect(UsageFormat.menuTitle(for: usage).contains("W 55%"))
}

@Test func clampsRemainingPercent() throws {
    let line = """
    {"timestamp":"2026-06-08T06:54:31.939Z","type":"event_msg","payload":{"rate_limits":{"primary":{"used_percent":120.0,"resets_at":1780902290},"secondary":{"used_percent":-1.0,"resets_at":1781375249}}}}
    """

    let usage = try #require(CodexUsageReader().parseUsageLine(line, sourcePath: "/tmp/session.jsonl"))
    #expect(usage.fiveHourRemainingPercent == 0)
    #expect(usage.weeklyRemainingPercent == 100)
}

@Test func readsAccountEmailFromIDToken() throws {
    let directory = FileManager.default.temporaryDirectory
        .appendingPathComponent(UUID().uuidString)
    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    defer { try? FileManager.default.removeItem(at: directory) }

    let authURL = directory.appendingPathComponent("auth.json")
    let payload = Data(#"{"email":"friend@example.com"}"#.utf8)
        .base64EncodedString()
        .replacingOccurrences(of: "+", with: "-")
        .replacingOccurrences(of: "/", with: "_")
        .replacingOccurrences(of: "=", with: "")
    let auth = #"{"tokens":{"id_token":"header.\#(payload).signature"}}"#
    try Data(auth.utf8).write(to: authURL)

    let reader = CodexUsageReader(authURL: authURL)
    #expect(reader.readAccountEmail() == "friend@example.com")
}
