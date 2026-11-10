#include "pipeline.hpp"
#include "graph.hpp"
#include "ml.hpp"
#include "rules.hpp"

std::vector<Signals> runPipeline(const std::vector<Transaction>& tx, const HybridScorer& scorer, const DetectorConfig& cfg) {
    std::vector<Signals> sig(tx.size());
    RuleEngine::fromConfig(cfg).run(tx, sig);
    GraphDetector(7200, 4, 0.25, 6, cfg.clusterMin).detect(tx, sig);
    MLDetector().detect(tx, sig);
    scorer.score(sig);
    return sig;
}
