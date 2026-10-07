#pragma once
// Synthetic transaction generator: normal behaviour + labelled injected anomalies
// (amount spike, repeated, odd hour, ring/cycle, fan-out, fan-in).
#include <vector>
#include "types.hpp"

struct GeneratorConfig {
    int accounts = 60;
    int days = 30;
    double avgPerDay = 2.0;
    int anomalyGroups = 30;
    unsigned seed = 42;
};

class TransactionGenerator {
public:
    explicit TransactionGenerator(GeneratorConfig cfg = GeneratorConfig()) : cfg_(cfg) {}
    std::vector<Transaction> generate() const;
private:
    GeneratorConfig cfg_;
};
