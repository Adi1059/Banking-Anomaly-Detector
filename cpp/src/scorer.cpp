#include "scorer.hpp"

void HybridScorer::score(std::vector<Signals>& sig) const {
    for (auto& s : sig) {
        s.risk = 1.0 - (1.0 - cfg_.wRule * s.ruleScore()) * (1.0 - cfg_.wGraph * s.graphScore()) * (1.0 - cfg_.wMl * s.ml);
        s.level = s.risk >= cfg_.high ? "HIGH" : s.risk >= cfg_.medium ? "MEDIUM" : s.risk >= cfg_.low ? "LOW" : "NONE";
        if (s.ml >= 0.5) s.reasons.push_back("ML outlier");
    }
}
