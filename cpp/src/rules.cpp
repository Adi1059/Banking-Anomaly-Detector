#include "rules.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace {
double medianOf(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}
}  // namespace

void Rule::apply(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const {
    const std::vector<char> flags = detect(tx);
    for (size_t i = 0; i < flags.size(); ++i)
        if (flags[i]) sig[i].hitRule(name(), weight());
}

// ---- Amount spike -------------------------------------------------------
std::vector<char> AmountSpikeRule::detect(const std::vector<Transaction>& tx) const {
    std::unordered_map<std::string, std::vector<double>> byAccount;   // hash map: account -> amounts
    for (const auto& t : tx) byAccount[t.sender].push_back(t.amount);

    struct Stat { double median, scale; size_t n; };
    std::unordered_map<std::string, Stat> stats;
    for (const auto& kv : byAccount) {
        const double med = medianOf(kv.second);
        std::vector<double> dev;
        dev.reserve(kv.second.size());
        for (double a : kv.second) dev.push_back(std::fabs(a - med));
        const double mad = medianOf(dev);
        const double scale = mad > 0 ? 1.4826 * mad : med * 0.1;      // fallback for constant spenders
        stats[kv.first] = {med, std::max(scale, 1e-9), kv.second.size()};
    }

    std::vector<char> flags(tx.size(), 0);
    for (size_t i = 0; i < tx.size(); ++i) {
        const Stat& s = stats[tx[i].sender];
        flags[i] = s.n >= minHistory_ && (tx[i].amount - s.median) / s.scale > threshold_;
    }
    return flags;
}

// ---- Repeated transaction (sliding window / two pointers) ---------------------
std::vector<char> RepeatedTransactionRule::detect(const std::vector<Transaction>& tx) const {
    std::unordered_map<std::string, std::vector<size_t>> groups;      // (account, amount) -> tx indices
    for (size_t i = 0; i < tx.size(); ++i)
        groups[tx[i].sender + "|" + std::to_string(std::llround(tx[i].amount * 100))].push_back(i);

    std::vector<char> flags(tx.size(), 0);
    for (auto& kv : groups) {
        auto& idx = kv.second;
        if (idx.size() < minRepeats_) continue;
        std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return tx[a].ts < tx[b].ts; });
        size_t left = 0;
        for (size_t right = 0; right < idx.size(); ++right) {
            while (tx[idx[right]].ts - tx[idx[left]].ts > windowSec_) ++left;
            if (right - left + 1 >= minRepeats_)
                for (size_t k = left; k <= right; ++k) flags[idx[k]] = 1;
        }
    }
    return flags;
}

// ---- Odd hour ---------------------------------------------------------------
std::vector<char> OddHourRule::detect(const std::vector<Transaction>& tx) const {
    std::vector<char> flags(tx.size(), 0);
    for (size_t i = 0; i < tx.size(); ++i) {
        const int hour = static_cast<int>((tx[i].ts % 86400) / 3600);
        flags[i] = (start_ <= end_) ? (hour >= start_ && hour < end_) : (hour >= start_ || hour < end_);
    }
    return flags;
}

// ---- Engine -----------------------------------------------------------------
RuleEngine RuleEngine::withDefaultRules() {
    RuleEngine e;
    e.add(std::make_unique<AmountSpikeRule>());
    e.add(std::make_unique<RepeatedTransactionRule>());
    e.add(std::make_unique<OddHourRule>());
    return e;
}

void RuleEngine::run(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const {
    for (const auto& r : rules_) r->apply(tx, sig);   // dynamic dispatch
}
