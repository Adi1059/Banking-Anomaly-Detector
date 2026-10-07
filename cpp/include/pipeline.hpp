#pragma once
#include <vector>
#include "scorer.hpp"
#include "types.hpp"

// transactions -> RuleEngine + GraphDetector + MLDetector -> HybridScorer
std::vector<Signals> runPipeline(const std::vector<Transaction>& tx, const HybridScorer& scorer);
