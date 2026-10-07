// Minimal self-contained unit tests (no framework needed).  Build & run:  make test
#include <cmath>
#include <iostream>
#include "graph.hpp"
#include "ml.hpp"
#include "rules.hpp"
#include "scorer.hpp"
#include "timeutil.hpp"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

static Transaction T(const char* s, const char* r, const char* when, double amt) {
    Transaction t; t.sender = s; t.receiver = r; t.amount = amt;
    parseTime(when, t.ts);
    return t;
}

static void testTime() {
    int64_t ts; parseTime("2026-03-05 14:07:09", ts);
    CHECK(formatTime(ts) == "2026-03-05 14:07:09");
}

static void testAmountSpike() {
    std::vector<Transaction> tx;
    for (int i = 1; i <= 7; ++i) tx.push_back(T("A", "MRC1", ("2026-01-0" + std::to_string(i) + " 12:00:00").c_str(), 100 + i));
    tx.push_back(T("A", "MRC1", "2026-01-09 12:00:00", 5000));
    const auto f = AmountSpikeRule().detect(tx);
    CHECK(f.back() == 1);
    for (size_t i = 0; i + 1 < f.size(); ++i) CHECK(f[i] == 0);
}

static void testRepeated() {
    std::vector<Transaction> tx;
    for (int i = 0; i < 4; ++i) tx.push_back(T("A", "MRC1", ("2026-01-01 12:0" + std::to_string(i) + ":00").c_str(), 250));
    tx.push_back(T("A", "MRC1", "2026-01-05 12:00:00", 250));
    const auto f = RepeatedTransactionRule().detect(tx);
    CHECK(f[0] && f[1] && f[2] && f[3] && !f[4]);
}

static void testOddHour() {
    std::vector<Transaction> tx = {T("A", "M", "2026-01-01 02:30:00", 1), T("A", "M", "2026-01-01 14:00:00", 1)};
    const auto f = OddHourRule().detect(tx);
    CHECK(f[0] == 1 && f[1] == 0);
}

static void testCycle() {
    std::vector<Transaction> tx = {T("ACC1", "ACC2", "2026-01-01 10:00:00", 1000), T("ACC2", "ACC3", "2026-01-01 10:30:00", 990),
                                   T("ACC3", "ACC1", "2026-01-01 11:00:00", 980), T("ACC1", "ACC9", "2026-01-03 10:00:00", 50)};
    const auto f = GraphDetector().findCycles(tx);
    CHECK(f[0] && f[1] && f[2] && !f[3]);
}

static void testFan() {
    std::vector<Transaction> tx;
    for (int i = 10; i < 20; ++i) tx.push_back(T("ACC1", ("ACC" + std::to_string(i)).c_str(), ("2026-01-01 10:" + std::to_string(i) + ":00").c_str(), 300));
    const auto f = GraphDetector().findFan(tx, true);
    for (char c : f) CHECK(c == 1);
    std::vector<Transaction> shop;
    for (int i = 10; i < 30; ++i) shop.push_back(T(("ACC" + std::to_string(i)).c_str(), "MRC1", "2026-01-01 10:00:00", 30));
    const auto g = GraphDetector().findFan(shop, false);
    for (char c : g) CHECK(c == 0);              // merchants never count as fan-in
}

static void testScorer() {
    std::vector<Signals> sig(3);
    sig[0].hitRule("amount spike", 0.5);
    sig[1].hitRule("amount spike", 0.5);
    sig[1].hitRule("repeated transaction", 0.6);
    HybridScorer().score(sig);
    CHECK(sig[0].risk < sig[1].risk);            // agreement raises risk
    CHECK(sig[2].risk == 0.0 && sig[2].level == "NONE");
    CHECK(sig[0].level == "LOW");                // one weak signal alone is not an alert
}

static void testForest() {
    std::vector<std::vector<double>> X;
    std::mt19937 g(1);
    std::normal_distribution<double> n(0, 1);
    for (int i = 0; i < 500; ++i) X.push_back({n(g), n(g)});
    IsolationForest f;
    f.fit(X);
    CHECK(f.score({10, 10}) > f.score({0, 0}));  // far-away point isolates faster
}

int main() {
    testTime(); testAmountSpike(); testRepeated(); testOddHour(); testCycle(); testFan(); testScorer(); testForest();
    if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
    std::cout << "All tests passed\n";
    return 0;
}
