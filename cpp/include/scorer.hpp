#pragma once
// ============================================================
// scorer.hpp  –  Hybrid risk scoring
// ============================================================
// Combines ruleScore + graphScore into a final risk value
// and assigns a severity level.
// ============================================================

#include <string>
#include <vector>
#include "types.hpp"

class HybridScorer {
public:
    // Weights for combining the two detectors
    HybridScorer(double ruleWeight = 0.6, double graphWeight = 0.4)
        : wRule_(ruleWeight), wGraph_(graphWeight) {}

    // Compute final risk and level for every transaction
    void score(std::vector<Signals>& signals) const;

    // Convenience: is this signal an alert?
    bool isAlert(const Signals& s) const {
        return s.risk >= 0.50;
    }

private:
    double wRule_;
    double wGraph_;

    static std::string levelFromRisk(double risk);
};
