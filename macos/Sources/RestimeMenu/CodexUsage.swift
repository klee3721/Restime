import Foundation

struct CodexUsage: Equatable {
    let fiveHourRemainingPercent: Int
    let fiveHourResetDate: Date
    let weeklyRemainingPercent: Int
    let weeklyResetDate: Date
    let sourcePath: String
    let observedAt: Date
}

enum UsageFormat {
    private static let timeFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.locale = Locale(identifier: "en_US_POSIX")
        formatter.dateFormat = "HH:mm"
        return formatter
    }()

    private static let dayFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.locale = Locale(identifier: "en_US_POSIX")
        formatter.dateFormat = "d/M"
        return formatter
    }()

    static func menuTitle(for usage: CodexUsage?) -> String {
        guard let usage else {
            return "5h --% --:-- | W --% --/--"
        }

        return "5h \(usage.fiveHourRemainingPercent)% \(timeFormatter.string(from: usage.fiveHourResetDate)) | W \(usage.weeklyRemainingPercent)% \(dayFormatter.string(from: usage.weeklyResetDate))"
    }

    static func time(_ date: Date) -> String {
        timeFormatter.string(from: date)
    }

    static func day(_ date: Date) -> String {
        dayFormatter.string(from: date)
    }
}
