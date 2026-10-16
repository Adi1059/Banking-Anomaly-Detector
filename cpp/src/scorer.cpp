// ============================================================
// scorer.cpp  –  Hybrid risk scoring
// ============================================================

#include "scorer.hpp"
#include <algorithm>

std::string HybridScorer::levelFromRisk(double risk) {
    if (risk >= 0.85) return "CRITICAL";
    if (risk >= 0.70) return "HIGH";
    if (risk >= 0.50) return "MEDIUM";
    if (risk >= 0.30) return "LOW";
    return "NONE";
}

void HybridScorer::score(std::vector<Signals>& signals) const {
    for (auto& s : signals) {
        // Weighted combination (simple and explainable)
        s.risk = std::min(1.0, wRule_ * s.ruleScore + wGraph_ * s.graphScore);

        // Small boost when both detectors agree
        if (s.ruleScore > 0.3 && s.graphScore > 0.3) {
            s.risk = std::min(1.0, s.risk + 0.15);
        }

        s.level = levelFromRisk(s.risk);
    }
}
