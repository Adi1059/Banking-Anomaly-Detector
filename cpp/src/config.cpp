#include "config.hpp"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "minijson.hpp"

bool DetectorConfig::load(const std::string& path, std::string* warn) {
    std::ifstream f(path);
    if (!f) return false;
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<minijson::Object> parsed;
    if (!minijson::Parser(text).parse(parsed) || parsed.empty()) { if (warn) *warn = "invalid JSON"; return false; }
    for (const auto& kv : parsed[0]) {
        const std::string& k = kv.first;
        const double d = std::atof(kv.second.c_str());
        if (k == "amount_threshold") amountThreshold = d;
        else if (k == "amount_min_history") amountMinHistory = static_cast<size_t>(d);
        else if (k == "repeated_window_sec") repeatedWindow = static_cast<int64_t>(d);
        else if (k == "repeated_min") repeatedMin = static_cast<size_t>(d);
        else if (k == "odd_start_hour") oddStart = static_cast<int>(d);
        else if (k == "odd_end_hour") oddEnd = static_cast<int>(d);
        else if (k == "velocity_window_sec") velocityWindow = static_cast<int64_t>(d);
        else if (k == "velocity_max") velocityMax = static_cast<size_t>(d);
        else if (k == "new_payee_multiplier") newPayeeMult = d;
        else if (k == "new_payee_min_history") newPayeeMinHistory = static_cast<size_t>(d);
        else if (k == "structuring_limit") structLimit = d;
        else if (k == "structuring_band") structBand = d;
        else if (k == "structuring_min") structMin = static_cast<size_t>(d);
        else if (k == "structuring_window_sec") structWindow = static_cast<int64_t>(d);
        else if (k == "cluster_min_accounts") clusterMin = static_cast<size_t>(d);
        else if (warn) *warn += "unknown key '" + k + "'\n";
    }
    return true;
}
