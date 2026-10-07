#pragma once
// Calendar <-> epoch helpers (Howard Hinnant's civil-date algorithms), portable across Windows/Linux.
#include <cstdint>
#include <cstdio>
#include <string>

inline int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097LL + static_cast<int64_t>(doe) - 719468;
}

inline void civilFromDays(int64_t z, int& y, unsigned& m, unsigned& d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += (m <= 2);
}

inline int64_t makeTime(int y, unsigned mo, unsigned d, int h = 0, int mi = 0, int s = 0) {
    return daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60 + s;
}

inline std::string formatTime(int64_t ts) {
    int64_t days = ts / 86400, rem = ts % 86400;
    int y; unsigned m, d;
    civilFromDays(days, y, m, d);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02u-%02u %02d:%02d:%02d", y, m, d,
                  static_cast<int>(rem / 3600), static_cast<int>(rem % 3600 / 60), static_cast<int>(rem % 60));
    return buf;
}

inline bool parseTime(const std::string& s, int64_t& out) {
    int y, mo, d, h, mi, sec = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &sec) < 5) return false;
    out = makeTime(y, static_cast<unsigned>(mo), static_cast<unsigned>(d), h, mi, sec);
    return true;
}
