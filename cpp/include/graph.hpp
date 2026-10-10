#pragma once
// Graph-based detection on the account-to-account transfer network.
//   cycle   : money returns to its origin within a short time (A->B->C->A), similar amounts  (DFS)
//   fan-out : one account pays many distinct accounts in a short window                      (sliding window + hash map)
//   fan-in  : many distinct accounts pay one account in a short window
// Merchant ids (prefix "MRC") are skipped: merchants legitimately receive from many senders.
#include <string>
#include <vector>
#include "types.hpp"

class GraphDetector {
public:
    GraphDetector(int64_t windowSec = 7200, size_t maxCycleLen = 4, double amountTolerance = 0.25, size_t fanThreshold = 6,
                  size_t clusterMin = 4)
        : windowSec_(windowSec), maxCycleLen_(maxCycleLen), tol_(amountTolerance), fanThreshold_(fanThreshold), clusterMin_(clusterMin) {}

    void detect(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const;

    std::vector<char> findCycles(const std::vector<Transaction>& tx) const;
    std::vector<char> findFan(const std::vector<Transaction>& tx, bool bySender) const;
    // Union-Find over flagged transfers: returns, per transaction, the number of accounts in its cluster (0 if not flagged).
    std::vector<size_t> clusterSizes(const std::vector<Transaction>& tx, const std::vector<char>& flagged) const;

private:
    static bool eligible(const Transaction& t);
    int64_t windowSec_;
    size_t maxCycleLen_;
    double tol_;
    size_t fanThreshold_;
    size_t clusterMin_;
};
