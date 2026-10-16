#pragma once
// ============================================================
// types.hpp  –  Core data structures used across the project
// ============================================================
// This file defines the two most important types:
//   1. Transaction  – one banking transaction
//   2. Signals      – what the detectors discovered about it
// ============================================================

#include <cstdint>
#include <string>
#include <vector>

// ------------------------------------------------------------
// One row from the transaction file
// ------------------------------------------------------------
struct Transaction {
    std::string id;          // unique transaction id
    std::string sender;      // account that sent money
    std::string receiver;    // account that received money
    double      amount = 0.0;
    int64_t     timestamp = 0;   // Unix timestamp (seconds)
    bool        injected = false; // true only for synthetic anomalies (evaluation)
    std::string anomalyType;     // e.g. "spike", "cycle" (for evaluation)
};

// ------------------------------------------------------------
// Everything the detectors learned about ONE transaction
// ------------------------------------------------------------
struct Signals {
    double ruleScore  = 0.0;     // 0..1 from rules
    double graphScore = 0.0;     // 0..1 from graph
    double risk       = 0.0;     // final combined risk 0..1
    std::string level = "NONE";  // NONE / LOW / MEDIUM / HIGH / CRITICAL
    std::vector<std::string> reasons;  // human-readable explanations

    // Helper: add a rule hit
    void addRuleHit(const std::string& name, double weight) {
        ruleScore = std::min(1.0, ruleScore + weight);
        reasons.push_back(name);
    }

    // Helper: add a graph hit
    void addGraphHit(const std::string& name, double weight) {
        graphScore = std::min(1.0, graphScore + weight);
        reasons.push_back(name);
    }
};
