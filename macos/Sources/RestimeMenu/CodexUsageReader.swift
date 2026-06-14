import Foundation

enum CodexUsageReaderError: LocalizedError {
    case sessionsDirectoryMissing(String)
    case noSessionFiles(String)
    case noRateLimitEvent

    var errorDescription: String? {
        switch self {
        case .sessionsDirectoryMissing(let path):
            return "Sessions directory not found: \(path)"
        case .noSessionFiles(let path):
            return "No Codex session files found in: \(path)"
        case .noRateLimitEvent:
            return "No rate limit event found in recent Codex sessions"
        }
    }
}

private final class HTTPResultBox: @unchecked Sendable {
    private let lock = NSLock()
    private var value: Result<(Data, HTTPURLResponse), Error>?

    func set(_ value: Result<(Data, HTTPURLResponse), Error>) {
        lock.lock()
        self.value = value
        lock.unlock()
    }

    func get() -> Result<(Data, HTTPURLResponse), Error>? {
        lock.lock()
        defer { lock.unlock() }
        return value
    }
}

final class CodexUsageReader {
    private let sessionsURL: URL
    private let authURL: URL
    private let fileManager: FileManager
    private let maxFilesToScan: Int
    private let largeFileTailBytes: UInt64
    private let largeFileThresholdBytes: UInt64
    private let usageEndpoint: URL

    init(
        sessionsURL: URL = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent(".codex")
            .appendingPathComponent("sessions"),
        authURL: URL = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent(".codex")
            .appendingPathComponent("auth.json"),
        fileManager: FileManager = .default,
        maxFilesToScan: Int = 12,
        largeFileTailBytes: UInt64 = 8 * 1024 * 1024,
        largeFileThresholdBytes: UInt64 = 16 * 1024 * 1024,
        usageEndpoint: URL = URL(string: "https://chatgpt.com/backend-api/wham/usage")!
    ) {
        self.sessionsURL = sessionsURL
        self.authURL = authURL
        self.fileManager = fileManager
        self.maxFilesToScan = maxFilesToScan
        self.largeFileTailBytes = largeFileTailBytes
        self.largeFileThresholdBytes = largeFileThresholdBytes
        self.usageEndpoint = usageEndpoint
    }

    func readLatestUsage(allowLocalFallback: Bool = true) throws -> CodexUsage {
        if let usage = try? readUsageFromAPI() {
            return usage
        }

        guard allowLocalFallback else {
            throw CodexUsageReaderError.noRateLimitEvent
        }

        guard fileManager.fileExists(atPath: sessionsURL.path) else {
            throw CodexUsageReaderError.sessionsDirectoryMissing(sessionsURL.path)
        }

        let files = try recentSessionFiles()
        guard !files.isEmpty else {
            throw CodexUsageReaderError.noSessionFiles(sessionsURL.path)
        }

        var newestUsage: CodexUsage?
        for file in files.prefix(maxFilesToScan) {
            let modified = (try? file.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate) ?? .distantPast
            if let newestUsage, modified < newestUsage.observedAt {
                break
            }

            if let usage = try latestUsage(in: file),
               newestUsage == nil || usage.observedAt > newestUsage!.observedAt {
                newestUsage = usage
            }
        }

        if let newestUsage {
            return newestUsage
        } else {
            throw CodexUsageReaderError.noRateLimitEvent
        }
    }

    func readAccountEmail() -> String? {
        guard let data = try? Data(contentsOf: authURL),
              let root = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let tokens = root["tokens"] as? [String: Any] else {
            return nil
        }

        if let idToken = tokens["id_token"] as? String,
           let email = jwtPayload(idToken)?["email"] as? String,
           !email.isEmpty {
            return email
        }

        if let accessToken = tokens["access_token"] as? String,
           let payload = jwtPayload(accessToken),
           let profile = payload["https://api.openai.com/profile"] as? [String: Any],
           let email = profile["email"] as? String,
           !email.isEmpty {
            return email
        }

        return nil
    }

    private func jwtPayload(_ token: String) -> [String: Any]? {
        let parts = token.split(separator: ".", omittingEmptySubsequences: false)
        guard parts.count >= 2 else {
            return nil
        }

        var encoded = String(parts[1])
            .replacingOccurrences(of: "-", with: "+")
            .replacingOccurrences(of: "_", with: "/")
        encoded += String(repeating: "=", count: (4 - encoded.count % 4) % 4)

        guard let data = Data(base64Encoded: encoded),
              let payload = try? JSONSerialization.jsonObject(with: data) as? [String: Any] else {
            return nil
        }

        return payload
    }

    private func readUsageFromAPI() throws -> CodexUsage {
        let token = try readAccessToken()
        var request = URLRequest(url: usageEndpoint, timeoutInterval: 5)
        request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
        request.setValue("application/json", forHTTPHeaderField: "Accept")

        let resultBox = HTTPResultBox()
        let semaphore = DispatchSemaphore(value: 0)
        let task = URLSession.shared.dataTask(with: request) { data, response, error in
            defer { semaphore.signal() }

            if let error {
                resultBox.set(.failure(error))
                return
            }

            guard let data,
                  let response = response as? HTTPURLResponse,
                  (200..<300).contains(response.statusCode) else {
                resultBox.set(.failure(CodexUsageReaderError.noRateLimitEvent))
                return
            }

            resultBox.set(.success((data, response)))
        }

        task.resume()
        _ = semaphore.wait(timeout: .now() + 6)

        guard let result = resultBox.get() else {
            task.cancel()
            throw CodexUsageReaderError.noRateLimitEvent
        }

        let (data, _) = try result.get()
        guard let root = try JSONSerialization.jsonObject(with: data) as? [String: Any],
              let rateLimit = root["rate_limit"] as? [String: Any],
              let primary = rateLimit["primary_window"] as? [String: Any],
              let secondary = rateLimit["secondary_window"] as? [String: Any],
              let primaryUsed = primary["used_percent"] as? Double,
              let secondaryUsed = secondary["used_percent"] as? Double,
              let primaryReset = primary["reset_at"] as? TimeInterval,
              let secondaryReset = secondary["reset_at"] as? TimeInterval else {
            throw CodexUsageReaderError.noRateLimitEvent
        }

        return CodexUsage(
            fiveHourRemainingPercent: remainingPercent(fromUsed: primaryUsed),
            fiveHourResetDate: Date(timeIntervalSince1970: primaryReset),
            weeklyRemainingPercent: remainingPercent(fromUsed: secondaryUsed),
            weeklyResetDate: Date(timeIntervalSince1970: secondaryReset),
            sourcePath: usageEndpoint.absoluteString,
            observedAt: Date()
        )
    }

    private func readAccessToken() throws -> String {
        let data = try Data(contentsOf: authURL)
        guard let root = try JSONSerialization.jsonObject(with: data) as? [String: Any],
              let tokens = root["tokens"] as? [String: Any],
              let token = tokens["access_token"] as? String,
              !token.isEmpty else {
            throw CodexUsageReaderError.noRateLimitEvent
        }

        return token
    }

    private func recentSessionFiles() throws -> [URL] {
        guard let enumerator = fileManager.enumerator(
            at: sessionsURL,
            includingPropertiesForKeys: [.contentModificationDateKey, .isRegularFileKey],
            options: [.skipsHiddenFiles]
        ) else {
            return []
        }

        var files: [(url: URL, modified: Date)] = []
        for case let url as URL in enumerator where url.pathExtension == "jsonl" {
            let values = try url.resourceValues(forKeys: [.contentModificationDateKey, .isRegularFileKey])
            guard values.isRegularFile == true else { continue }
            files.append((url, values.contentModificationDate ?? .distantPast))
        }

        return files
            .sorted { $0.modified > $1.modified }
            .map(\.url)
    }

    private func latestUsage(in url: URL) throws -> CodexUsage? {
        let size = (try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize).map(UInt64.init) ?? 0
        if size > largeFileThresholdBytes {
            return try latestUsageFromTail(in: url)
        } else {
            return try latestUsageStreaming(in: url)
        }
    }

    private func latestUsageStreaming(in url: URL) throws -> CodexUsage? {
        let data = try Data(contentsOf: url)
        guard let text = String(data: data, encoding: .utf8) else {
            return nil
        }
        var latest: CodexUsage?
        for line in text.split(separator: "\n", omittingEmptySubsequences: true) {
            let line = String(line)
            guard line.contains("\"rate_limits\""),
                  let usage = parseUsageLine(line, sourcePath: url.path) else {
                continue
            }

            if latest == nil || usage.observedAt > latest!.observedAt {
                latest = usage
            }
        }

        return latest
    }

    private func latestUsageFromTail(in url: URL) throws -> CodexUsage? {
        let handle = try FileHandle(forReadingFrom: url)
        defer {
            try? handle.close()
        }

        let size = try handle.seekToEnd()
        let offset = size > largeFileTailBytes ? size - largeFileTailBytes : 0
        try handle.seek(toOffset: offset)
        let data = try handle.readToEnd() ?? Data()
        guard let text = String(data: data, encoding: .utf8) else {
            return nil
        }

        let lines = text.split(separator: "\n", omittingEmptySubsequences: true).reversed()
        for line in lines where line.contains("\"rate_limits\"") {
            if let usage = parseUsageLine(String(line), sourcePath: url.path) {
                return usage
            }
        }

        return nil
    }

    func parseUsageLine(_ line: String, sourcePath: String) -> CodexUsage? {
        guard let data = line.data(using: .utf8),
              let root = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let payload = root["payload"] as? [String: Any],
              let rateLimits = payload["rate_limits"] as? [String: Any],
              let primary = rateLimits["primary"] as? [String: Any],
              let secondary = rateLimits["secondary"] as? [String: Any],
              let primaryUsed = primary["used_percent"] as? Double,
              let secondaryUsed = secondary["used_percent"] as? Double,
              let primaryReset = primary["resets_at"] as? TimeInterval,
              let secondaryReset = secondary["resets_at"] as? TimeInterval else {
            return nil
        }

        let observedAt = parseTimestamp(root["timestamp"] as? String) ?? Date()
        return CodexUsage(
            fiveHourRemainingPercent: remainingPercent(fromUsed: primaryUsed),
            fiveHourResetDate: Date(timeIntervalSince1970: primaryReset),
            weeklyRemainingPercent: remainingPercent(fromUsed: secondaryUsed),
            weeklyResetDate: Date(timeIntervalSince1970: secondaryReset),
            sourcePath: sourcePath,
            observedAt: observedAt
        )
    }

    private func remainingPercent(fromUsed used: Double) -> Int {
        let remaining = min(max(100.0 - used, 0), 100)
        return Int(remaining.rounded())
    }

    private func parseTimestamp(_ value: String?) -> Date? {
        guard let value else { return nil }
        return ISO8601DateFormatter().date(from: value)
    }
}
