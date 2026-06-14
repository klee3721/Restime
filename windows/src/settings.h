#pragma once

#include <string>

namespace restime {

class Settings {
public:
    bool start_with_windows() const;
    void set_start_with_windows(bool enabled) const;

private:
    std::wstring executable_path() const;
};

}  // namespace restime
