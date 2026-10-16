#pragma once
// ============================================================
// graph.hpp  –  Graph-based cross-account analysis
// ============================================================
// Detects:
//   1. Cycles          (A → B → C → A)   – possible money laundering
//   2. Fan-out         (one account pays many others quickly)
//   3. Fan-in          (many accounts pay one account quickly)
//
// Data structures used:
//   - Adjacency list  (unordered_map of vectors)
//   - DFS with explicit stack for cycle detection
// ============================================================

#include <string>
#include <vector>
#include "types.hpp"

class GraphDetector {
public:
    GraphDetector(int windowSeconds = 7200, int maxCycleLength = 4)
        : windowSec_(windowSeconds), maxCycleLen_(maxCycleLength) {}

    // Main entry point – fills Signals for every transaction
    void detect(const std::vector<Transaction>& tx, std::vector<Signals>& signals) const;

private:
    int windowSec_;
    int maxCycleLen_;

    // Helper methods
    std::vector<bool> findCycles(const std::vector<Transaction>& tx) const;
    std::vector<bool> findFanOut(const std::vector<Transaction>& tx) const;
    std::vector<bool> findFanIn (const std::vector<Transaction>& tx) const;
};
