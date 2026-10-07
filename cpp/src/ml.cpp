#include "ml.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

// ---------------- Isolation Forest ----------------
double IsolationForest::c(double n) {
    if (n <= 1) return 0.0;
    if (n == 2) return 1.0;
    return 2.0 * (std::log(n - 1.0) + 0.5772156649) - 2.0 * (n - 1.0) / n;
}

int IsolationForest::build(Tree& t, std::vector<size_t>& idx, size_t lo, size_t hi, int depth, int maxDepth,
                           const std::vector<std::vector<double>>& X) {
    const int me = static_cast<int>(t.size());
    t.emplace_back();
    t[me].size = hi - lo;
    if (depth >= maxDepth || hi - lo <= 1) return me;

    const size_t F = X[0].size();
    for (size_t attempt = 0; attempt < F; ++attempt) {          // find a feature that is not constant here
        const size_t f = rng_() % F;
        double mn = X[idx[lo]][f], mx = mn;
        for (size_t i = lo; i < hi; ++i) { mn = std::min(mn, X[idx[i]][f]); mx = std::max(mx, X[idx[i]][f]); }
        if (mx <= mn) continue;
        const double split = std::uniform_real_distribution<double>(mn, mx)(rng_);
        auto mid = std::partition(idx.begin() + lo, idx.begin() + hi, [&](size_t r) { return X[r][f] < split; });
        const size_t m = static_cast<size_t>(mid - idx.begin());
        if (m == lo || m == hi) continue;
        t[me].feature = static_cast<int>(f);
        t[me].split = split;
        const int l = build(t, idx, lo, m, depth + 1, maxDepth, X);
        const int r = build(t, idx, m, hi, depth + 1, maxDepth, X);
        t[me].left = l;
        t[me].right = r;
        return me;
    }
    return me;                                                  // leaf
}

void IsolationForest::fit(const std::vector<std::vector<double>>& X) {
    forest_.clear();
    if (X.empty()) return;
    psi_ = std::min(sampleSize_, X.size());
    const int maxDepth = static_cast<int>(std::ceil(std::log2(std::max<size_t>(psi_, 2))));
    std::vector<size_t> all(X.size());
    std::iota(all.begin(), all.end(), 0);
    for (int k = 0; k < numTrees_; ++k) {
        std::shuffle(all.begin(), all.end(), rng_);             // random sub-sample
        std::vector<size_t> sample(all.begin(), all.begin() + psi_);
        Tree t;
        build(t, sample, 0, sample.size(), 0, maxDepth, X);
        forest_.push_back(std::move(t));
    }
}

double IsolationForest::pathLength(const Tree& t, const std::vector<double>& x) const {
    int n = 0;
    double depth = 0;
    while (t[n].feature >= 0) {
        n = x[t[n].feature] < t[n].split ? t[n].left : t[n].right;
        depth += 1;
    }
    return depth + c(static_cast<double>(t[n].size));
}

double IsolationForest::score(const std::vector<double>& x) const {
    if (forest_.empty()) return 0.5;
    double sum = 0;
    for (const auto& t : forest_) sum += pathLength(t, x);
    return std::pow(2.0, -(sum / forest_.size()) / std::max(c(static_cast<double>(psi_)), 1e-9));
}

// ---------------- Features + detector ----------------
std::vector<std::vector<double>> MLDetector::buildFeatures(const std::vector<Transaction>& tx) {
    const double kPi = 3.14159265358979323846;
    std::vector<std::vector<double>> X(tx.size(), std::vector<double>(kFeatures, 0.0));
    std::unordered_map<std::string, std::vector<size_t>> byAcc;
    for (size_t i = 0; i < tx.size(); ++i) byAcc[tx[i].sender].push_back(i);

    for (auto& kv : byAcc) {
        auto& idx = kv.second;
        std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return tx[a].ts < tx[b].ts; });
        std::vector<double> amts;
        for (size_t i : idx) amts.push_back(tx[i].amount);
        std::nth_element(amts.begin(), amts.begin() + amts.size() / 2, amts.end());
        const double med = std::max(amts[amts.size() / 2], 1e-6);

        std::unordered_set<std::string> seen;
        std::unordered_map<std::string, int> win;                // receivers inside the last hour
        size_t left = 0;
        for (size_t r = 0; r < idx.size(); ++r) {
            const Transaction& t = tx[idx[r]];
            win[t.receiver]++;
            while (t.ts - tx[idx[left]].ts > 3600) {
                if (--win[tx[idx[left]].receiver] == 0) win.erase(tx[idx[left]].receiver);
                ++left;
            }
            const double hour = (t.ts % 86400) / 3600.0;
            const double gap = r == 0 ? 7.0 * 86400 : static_cast<double>(t.ts - tx[idx[r - 1]].ts);
            auto& f = X[idx[r]];
            f[0] = std::log1p(t.amount);
            f[1] = std::min(t.amount / med, 100.0);
            f[2] = std::sin(2 * kPi * hour / 24);
            f[3] = std::cos(2 * kPi * hour / 24);
            f[4] = static_cast<double>(r - left + 1);            // velocity in last hour
            f[5] = std::log1p(std::max(gap, 0.0));
            f[6] = seen.insert(t.receiver).second ? 1.0 : 0.0;   // first time paying this receiver
            f[7] = static_cast<double>(win.size());              // distinct receivers in last hour
        }
    }
    return X;
}

void MLDetector::detect(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const {
    if (tx.empty()) return;
    const auto X = buildFeatures(tx);
    IsolationForest forest;
    forest.fit(X);
    std::vector<double> raw(tx.size());
    for (size_t i = 0; i < tx.size(); ++i) raw[i] = forest.score(X[i]);

    std::vector<double> sorted = raw;
    std::sort(sorted.begin(), sorted.end());
    auto q = [&](double p) { return sorted[std::min(sorted.size() - 1, static_cast<size_t>(p * sorted.size()))]; };
    const double lo = q(0.90), hi = q(0.995);                    // bottom 90% of behaviour counts as normal (ml = 0)
    for (size_t i = 0; i < tx.size(); ++i)
        sig[i].ml = std::clamp((raw[i] - lo) / std::max(hi - lo, 1e-9), 0.0, 1.0);
}
