#pragma once
// Hybrid risk scoring: fuses rule, graph and ML signals into one explainable score in [0,1].
//   risk = 1 - (1 - a*rule) * (1 - b*graph) * (1 - c*ml)        (weighted noisy-OR)
// One weak signal stays LOW; a strong signal, or several independent detectors agreeing, becomes an alert.
#include <vector>
#include "types.hpp"

struct ScoreConfig {
    double wRule = 0.90, wGraph = 0.90, wMl = 0.80;
    double low = 0.20, medium = 0.50, high = 0.70;
    double alert = 0.50;           // risk >= alert counts as an alert (MEDIUM and above)
};

class HybridScorer {
public:
    explicit HybridScorer(ScoreConfig cfg = ScoreConfig()) : cfg_(cfg) {}
    void score(std::vector<Signals>& sig) const;
    bool isAlert(const Signals& s) const { return s.risk >= cfg_.alert; }
    const ScoreConfig& config() const { return cfg_; }
private:
    ScoreConfig cfg_;
};
