#include "pipeline.hpp"
#include "graph.hpp"
#include "ml.hpp"
#include "rules.hpp"

std::vector<Signals> runPipeline(const std::vector<Transaction>& tx, const HybridScorer& scorer) {
    std::vector<Signals> sig(tx.size());
    RuleEngine::withDefaultRules().run(tx, sig);
    GraphDetector().detect(tx, sig);
    MLDetector().detect(tx, sig);
    scorer.score(sig);
    return sig;
}
