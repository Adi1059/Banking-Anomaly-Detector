#pragma once
// Precision / recall against the injected ground-truth labels (for the demo only).
#include <vector>
#include "types.hpp"

struct Metrics { size_t alerts = 0, tp = 0, positives = 0; double precision = 0, recall = 0, f1 = 0; };

inline Metrics evaluate(const std::vector<Transaction>& tx, const std::vector<char>& pred) {
    Metrics m;
    for (size_t i = 0; i < tx.size(); ++i) {
        if (pred[i]) { ++m.alerts; if (tx[i].injected) ++m.tp; }
        if (tx[i].injected) ++m.positives;
    }
    m.precision = m.alerts ? static_cast<double>(m.tp) / m.alerts : 0.0;
    m.recall = m.positives ? static_cast<double>(m.tp) / m.positives : 0.0;
    m.f1 = (m.precision + m.recall) > 0 ? 2 * m.precision * m.recall / (m.precision + m.recall) : 0.0;
    return m;
}
