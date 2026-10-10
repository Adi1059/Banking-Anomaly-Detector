#include "rules.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <unordered_set>
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

// ---- Velocity (deque sliding window) -------------------------------------------
std::vector<char> VelocityRule::detect(const std::vector<Transaction>& tx) const {
    std::unordered_map<std::string, std::vector<size_t>> bySender;
    for (size_t i = 0; i < tx.size(); ++i) bySender[tx[i].sender].push_back(i);

    std::vector<char> flags(tx.size(), 0);
    for (auto& kv : bySender) {
        auto& idx = kv.second;
        if (idx.size() < maxCount_) continue;
        std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return tx[a].ts < tx[b].ts; });
        std::deque<size_t> window;                                   // transactions inside the time window
        for (size_t i : idx) {
            window.push_back(i);
            while (tx[i].ts - tx[window.front()].ts > windowSec_) window.pop_front();
            if (window.size() >= maxCount_)
                for (size_t k : window) flags[k] = 1;
        }
    }
    return flags;
}

// ---- Large transfer to a new payee ---------------------------------------------
std::vector<char> NewPayeeLargeTransferRule::detect(const std::vector<Transaction>& tx) const {
    std::unordered_map<std::string, std::vector<double>> amounts;     // account -> amounts (for the median)
    for (const auto& t : tx) amounts[t.sender].push_back(t.amount);
    std::unordered_map<std::string, double> median;
    for (const auto& kv : amounts) median[kv.first] = medianOf(kv.second);

    std::vector<size_t> order(tx.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return tx[a].ts < tx[b].ts; });

    std::unordered_set<std::string> seen;                             // hash set of (sender, receiver) pairs
    std::vector<char> flags(tx.size(), 0);
    for (size_t i : order) {
        const Transaction& t = tx[i];
        const bool isNew = seen.insert(t.sender + "|" + t.receiver).second;
        if (isNew && t.sender != t.receiver && amounts[t.sender].size() >= minHistory_ && t.amount >= mult_ * median[t.sender])
            flags[i] = 1;
    }
    return flags;
}

// ---- Structuring (deque sliding window over amounts just under the limit) --------
std::vector<char> StructuringRule::detect(const std::vector<Transaction>& tx) const {
    std::unordered_map<std::string, std::vector<size_t>> bySender;
    for (size_t i = 0; i < tx.size(); ++i)
        if (tx[i].amount >= band_ * limit_ && tx[i].amount < limit_) bySender[tx[i].sender].push_back(i);

    std::vector<char> flags(tx.size(), 0);
    for (auto& kv : bySender) {
        auto& idx = kv.second;
        if (idx.size() < minCount_) continue;
        std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return tx[a].ts < tx[b].ts; });
        std::deque<size_t> window;
        for (size_t i : idx) {
            window.push_back(i);
            while (tx[i].ts - tx[window.front()].ts > windowSec_) window.pop_front();
            if (window.size() >= minCount_)
                for (size_t k : window) flags[k] = 1;
        }
    }
    return flags;
}

// ---- Engine -----------------------------------------------------------------
RuleEngine RuleEngine::withDefaultRules() {
    RuleEngine e;
    e.add(std::make_unique<AmountSpikeRule>());
    e.add(std::make_unique<RepeatedTransactionRule>());
    e.add(std::make_unique<OddHourRule>());
    e.add(std::make_unique<VelocityRule>());
    e.add(std::make_unique<NewPayeeLargeTransferRule>());
    e.add(std::make_unique<StructuringRule>());
    return e;
}

RuleEngine RuleEngine::fromConfig(const DetectorConfig& c) {
    RuleEngine e;
    e.add(std::make_unique<AmountSpikeRule>(c.amountThreshold, c.amountMinHistory));
    e.add(std::make_unique<RepeatedTransactionRule>(c.repeatedWindow, c.repeatedMin));
    e.add(std::make_unique<OddHourRule>(c.oddStart, c.oddEnd));
    e.add(std::make_unique<VelocityRule>(c.velocityWindow, c.velocityMax));
    e.add(std::make_unique<NewPayeeLargeTransferRule>(c.newPayeeMult, c.newPayeeMinHistory));
    e.add(std::make_unique<StructuringRule>(c.structLimit, c.structBand, c.structMin, c.structWindow));
    return e;
}

void RuleEngine::run(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const {
    for (const auto& r : rules_) r->apply(tx, sig);   // dynamic dispatch
}
