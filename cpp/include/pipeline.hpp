#pragma once
#include <vector>
#include "config.hpp"
#include "scorer.hpp"
#include "types.hpp"

// transactions -> RuleEngine + GraphDetector + MLDetector -> HybridScorer
std::vector<Signals> runPipeline(const std::vector<Transaction>& tx, const HybridScorer& scorer,
                                 const DetectorConfig& cfg = DetectorConfig());
