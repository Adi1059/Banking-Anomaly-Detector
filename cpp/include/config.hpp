#pragma once
// Central, file-configurable thresholds for every rule (flat JSON object).
#include <cstdint>
#include <string>

struct DetectorConfig {
    // amount spike
    double amountThreshold = 3.5;  size_t amountMinHistory = 5;
    // repeated transaction
    int64_t repeatedWindow = 600;  size_t repeatedMin = 3;
    // odd hour [start, end)
    int oddStart = 0, oddEnd = 5;
    // velocity: >= velocityMax transactions by one account inside velocityWindow seconds
    int64_t velocityWindow = 600;  size_t velocityMax = 6;
    // large transfer to a new payee: first-ever payment to a receiver, amount >= mult * account median
    double newPayeeMult = 5.0;     size_t newPayeeMinHistory = 5;
    // structuring: >= structMin payments in [band*limit, limit) inside structWindow seconds
    double structLimit = 10000.0;  double structBand = 0.9;
    size_t structMin = 3;          int64_t structWindow = 86400;
    // graph
    size_t clusterMin = 4;         // accounts in one suspicious cluster before it is reported

    // Returns false when the file cannot be opened; unknown keys are reported through 'warn' (may be null).
    bool load(const std::string& path, std::string* warn = nullptr);
};
