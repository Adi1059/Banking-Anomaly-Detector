#pragma once
// ============================================================
// generator.hpp  –  Synthetic transaction data generator
// ============================================================
// Creates realistic-looking transactions + injects known anomalies
// so we can evaluate precision / recall.
// ============================================================

#include <vector>
#include "types.hpp"

class TransactionGenerator {
public:
    // Generate a dataset of the given size
    // (approximately 5-6% of transactions will be anomalous)
    std::vector<Transaction> generate(int numTransactions = 4000) const;
};
