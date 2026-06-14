#include "usage.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace restime {

int remaining_percent(double used_percent) {
    const double remaining = std::clamp(100.0 - used_percent, 0.0, 100.0);
    return static_cast<int>(std::round(remaining));
}

std::chrono::system_clock::time_point unix_time(double seconds) {
    const auto whole = static_cast<std::time_t>(seconds);
    return std::chrono::system_clock::from_time_t(whole);
}

std::chrono::system_clock::time_point parse_iso8601_utc(const std::string& value) {
    if (value.size() < 19) {
        return std::chrono::system_clock::now();
    }

    std::tm tm{};
    std::istringstream stream(value.substr(0, 19));
    stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (stream.fail()) {
        return std::chrono::system_clock::now();
    }

#ifdef _WIN32
    const auto seconds = _mkgmtime(&tm);
#else
    const auto seconds = timegm(&tm);
#endif
    if (seconds == -1) {
        return std::chrono::system_clock::now();
    }

    return std::chrono::system_clock::from_time_t(seconds);
}

}  // namespace restime
