#pragma once
// Core data types shared by every module.
#include <cstdint>
#include <string>
#include <vector>

struct Transaction {
    std::string id, sender, receiver, merchant, anomalyType;
    int64_t ts = 0;          // seconds since Unix epoch (UTC)
    double amount = 0.0;
    bool injected = false;   // ground-truth label, used ONLY for evaluation
};

// What the detectors learned about one transaction.
// Rule and graph weights are combined with a "noisy-OR": keep = prod(1 - w).
struct Signals {
    double ruleKeep = 1.0, graphKeep = 1.0;
    double ml = 0.0, risk = 0.0;
    bool ruleHit = false, graphHit = false;
    std::string level = "NONE";
    std::vector<std::string> reasons;

    void hitRule(const std::string& name, double w) { ruleKeep *= (1.0 - w); ruleHit = true; reasons.push_back(name); }
    void hitGraph(const std::string& name, double w) { graphKeep *= (1.0 - w); graphHit = true; reasons.push_back(name); }
    double ruleScore() const { return 1.0 - ruleKeep; }
    double graphScore() const { return 1.0 - graphKeep; }
};
