#include "generator.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <random>
#include <unordered_map>
#include "timeutil.hpp"

namespace {
std::string acc(int i) { char b[16]; std::snprintf(b, sizeof b, "ACC%04d", i); return b; }
std::string mrc(int i) { char b[16]; std::snprintf(b, sizeof b, "MRC%04d", i); return b; }
double round2(double x) { return std::round(x * 100.0) / 100.0; }
const char* kCategories[] = {"Grocery", "Fuel", "Restaurant", "Utilities", "Pharmacy", "Online Shop", "Transport"};
}  // namespace

std::vector<Transaction> TransactionGenerator::generate() const {
    std::mt19937 rng(cfg_.seed);
    auto uni = [&](double a, double b) { return std::uniform_real_distribution<double>(a, b)(rng); };
    auto rint = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); };  // inclusive

    const int64_t start = makeTime(2026, 1, 1);
    const int merchants = 40;
    std::vector<Transaction> tx;
    std::unordered_map<std::string, std::vector<double>> amounts;

    auto make = [&](const std::string& s, const std::string& r, int64_t ts, double amt, const std::string& cat,
                    bool inj, const std::string& type) {
        Transaction t;
        t.sender = s; t.receiver = r; t.ts = ts; t.amount = amt; t.merchant = cat; t.injected = inj; t.anomalyType = type;
        return t;
    };

    // ---- normal behaviour ----
    for (int a = 1; a <= cfg_.accounts; ++a) {
        const double base = uni(20, 200);
        std::vector<int> pool(merchants);
        std::iota(pool.begin(), pool.end(), 0);
        std::shuffle(pool.begin(), pool.end(), rng);
        std::vector<int> others;
        for (int x = 1; x <= cfg_.accounts; ++x) if (x != a) others.push_back(x);
        std::shuffle(others.begin(), others.end(), rng);

        const int n = std::poisson_distribution<int>(cfg_.avgPerDay * cfg_.days)(rng);
        std::lognormal_distribution<double> amt(std::log(base), 0.4);
        std::normal_distribution<double> hr(14.0, 4.0);
        for (int k = 0; k < n; ++k) {
            const int hour = static_cast<int>(std::clamp(hr(rng), 7.0, 22.0));
            const int64_t ts = start + rint(0, cfg_.days - 1) * 86400LL + hour * 3600 + rint(0, 59) * 60 + rint(0, 59);
            const std::string recv = uni(0, 1) < 0.1 ? acc(others[rint(0, 1)]) : mrc(pool[rint(0, 3)]);
            const double amount = round2(amt(rng));
            tx.push_back(make(acc(a), recv, ts, amount, kCategories[rint(0, 6)], false, "normal"));
            amounts[acc(a)].push_back(amount);
        }
    }
    auto medianOf = [&](const std::string& a) {
        auto it = amounts.find(a);
        if (it == amounts.end() || it->second.empty()) return 100.0;
        auto v = it->second;
        std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
        return v[v.size() / 2];
    };

    // ---- injected anomalies ----
    const char* kinds[] = {"amount_spike", "repeated", "odd_hour", "ring", "fan_out", "fan_in"};
    for (int g = 0; g < cfg_.anomalyGroups; ++g) {
        const std::string kind = kinds[g % 6];
        const int64_t ts = start + rint(1, cfg_.days - 2) * 86400LL + rint(9, 19) * 3600 + rint(0, 59) * 60;
        const std::string a = acc(rint(1, cfg_.accounts)), m = mrc(rint(0, merchants - 1));

        auto distinctOthers = [&](int k, int exclude) {
            std::vector<int> v;
            for (int x = 1; x <= cfg_.accounts; ++x) if (x != exclude) v.push_back(x);
            std::shuffle(v.begin(), v.end(), rng);
            v.resize(static_cast<size_t>(k));
            return v;
        };

        if (kind == "amount_spike") {
            tx.push_back(make(a, m, ts, round2(medianOf(a) * uni(8, 20)), "Online Shop", true, kind));
        } else if (kind == "repeated") {
            const double amt = round2(uni(50, 500));
            for (int k = 0, n = rint(3, 5); k < n; ++k) tx.push_back(make(a, m, ts + 120 * k, amt, "Online Shop", true, kind));
        } else if (kind == "odd_hour") {
            const int64_t t = ts - ts % 86400 + rint(1, 4) * 3600 + rint(0, 59) * 60;
            tx.push_back(make(a, m, t, round2(uni(100, 800)), "Online Shop", true, kind));
        } else if (kind == "ring") {
            const int xi = rint(1, cfg_.accounts);
            const auto ids = distinctOthers(2, xi);                     // two other accounts complete the ring
            const std::string x = acc(xi), y = acc(ids[0]), z = acc(ids[1]);
            const double amt = round2(uni(2000, 9000));
            const std::string ring[3] = {x, y, z};
            for (int j = 0; j < 3; ++j)
                tx.push_back(make(ring[j], ring[(j + 1) % 3], ts + 2400 * j, round2(amt * std::pow(0.98, j)), "Transfer", true, kind));
        } else if (kind == "fan_out") {
            const int ai = std::stoi(a.substr(3));
            const auto ts2 = distinctOthers(rint(8, 12), ai);
            for (size_t j = 0; j < ts2.size(); ++j)
                tx.push_back(make(a, acc(ts2[j]), ts + 240 * static_cast<int64_t>(j), round2(uni(300, 900)), "Transfer", true, kind));
        } else {  // fan_in
            const int ai = std::stoi(a.substr(3));
            const auto ss = distinctOthers(rint(8, 12), ai);
            for (size_t j = 0; j < ss.size(); ++j)
                tx.push_back(make(acc(ss[j]), a, ts + 240 * static_cast<int64_t>(j), round2(uni(300, 900)), "Transfer", true, kind));
        }
    }

    std::stable_sort(tx.begin(), tx.end(), [](const Transaction& l, const Transaction& r) { return l.ts < r.ts; });
    for (size_t i = 0; i < tx.size(); ++i) { char b[32]; std::snprintf(b, sizeof b, "TX%06d", static_cast<int>(i + 1)); tx[i].id = b; }
    return tx;
}
