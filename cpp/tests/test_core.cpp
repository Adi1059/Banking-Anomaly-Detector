// Minimal self-contained unit tests (no framework needed).  Build & run:  make test
#include <cmath>
#include <iostream>
#include <cstdio>
#include "config.hpp"
#include "graph.hpp"
#include "io.hpp"
#include "unionfind.hpp"
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
    int64_t ts = 0; parseTime("2026-03-05 14:07:09", ts);
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

static void testVelocity() {
    std::vector<Transaction> tx;
    for (int i = 0; i < 7; ++i) tx.push_back(T("A", "MRC1", ("2026-01-01 12:0" + std::to_string(i) + ":00").c_str(), 20 + i));   // 7 in 7 min
    tx.push_back(T("A", "MRC1", "2026-01-02 12:00:00", 25));
    tx.push_back(T("B", "MRC1", "2026-01-01 12:00:00", 25));
    const auto f = VelocityRule().detect(tx);
    for (int i = 0; i < 7; ++i) CHECK(f[i] == 1);
    CHECK(f[7] == 0 && f[8] == 0);
}

static void testNewPayee() {
    std::vector<Transaction> tx;
    for (int i = 1; i <= 6; ++i) tx.push_back(T("A", "MRC1", ("2026-01-0" + std::to_string(i) + " 12:00:00").c_str(), 100));
    tx.push_back(T("A", "ACC9", "2026-01-07 12:00:00", 900));     // new payee, 9x median -> flag
    tx.push_back(T("A", "ACC9", "2026-01-08 12:00:00", 900));     // same payee again -> not new
    tx.push_back(T("A", "ACC7", "2026-01-09 12:00:00", 110));     // new payee but ordinary amount
    const auto f = NewPayeeLargeTransferRule().detect(tx);
    CHECK(f[6] == 1 && f[7] == 0 && f[8] == 0);
}

static void testStructuring() {
    std::vector<Transaction> tx = {T("A", "B", "2026-01-01 09:00:00", 9500), T("A", "C", "2026-01-01 12:00:00", 9800),
                                   T("A", "D", "2026-01-01 15:00:00", 9900), T("A", "B", "2026-01-05 09:00:00", 9600),
                                   T("E", "B", "2026-01-01 09:00:00", 9500), T("A", "B", "2026-01-01 10:00:00", 12000)};
    const auto f = StructuringRule().detect(tx);
    CHECK(f[0] && f[1] && f[2] && !f[3] && !f[4] && !f[5]);       // amounts over the limit or other accounts are ignored
}

static void testUnionFind() {
    UnionFind uf(6);
    CHECK(uf.components() == 6);
    uf.unite(0, 1); uf.unite(1, 2); uf.unite(4, 5);
    CHECK(uf.connected(0, 2) && !uf.connected(0, 4));
    CHECK(uf.sizeOf(2) == 3 && uf.sizeOf(5) == 2 && uf.components() == 3);
    CHECK(!uf.unite(0, 2));                                       // already connected
}

static void testClusters() {
    std::vector<Transaction> tx = {T("ACC1", "ACC2", "2026-01-01 10:00:00", 1), T("ACC2", "ACC3", "2026-01-01 10:01:00", 1),
                                   T("ACC3", "ACC4", "2026-01-01 10:02:00", 1), T("ACC8", "ACC9", "2026-01-01 10:03:00", 1),
                                   T("ACC5", "ACC6", "2026-01-01 10:04:00", 1)};
    const auto s = GraphDetector().clusterSizes(tx, {1, 1, 1, 1, 0});
    CHECK(s[0] == 4 && s[1] == 4 && s[2] == 4 && s[3] == 2 && s[4] == 0);
}

static void testJsonRoundTrip() {
    std::vector<Transaction> a = {T("ACC1", "MRC2", "2026-03-05 14:07:09", 123.45), T("ACC2", "ACC3", "2026-03-06 01:00:00", 9999.5)};
    a[0].id = "TX1"; a[1].id = "TX2"; a[1].injected = true; a[1].anomalyType = "structuring"; a[0].merchant = "Fuel \"Pump\"";
    CHECK(writeTransactionsJson("_t.json", a));
    std::vector<Transaction> b;
    CHECK(readTransactionsJson("_t.json", b) && b.size() == 2);
    CHECK(b[0].id == "TX1" && b[0].sender == "ACC1" && std::fabs(b[0].amount - 123.45) < 1e-9 && b[0].ts == a[0].ts);
    CHECK(b[0].merchant == "Fuel \"Pump\"" && b[1].injected && b[1].anomalyType == "structuring");
    std::remove("_t.json");
}

static void testConfig() {
    DetectorConfig c;
    FILE* f = std::fopen("_c.json", "w");
    std::fputs("{\"velocity_max\": 9, \"structuring_limit\": 5000.5, \"bogus\": 1}", f);
    std::fclose(f);
    std::string warn;
    CHECK(c.load("_c.json", &warn));
    CHECK(c.velocityMax == 9 && std::fabs(c.structLimit - 5000.5) < 1e-9 && c.repeatedMin == 3);   // untouched keys keep defaults
    CHECK(warn.find("bogus") != std::string::npos);
    CHECK(RuleEngine::fromConfig(c).size() == 6);
    std::remove("_c.json");
}

static void testSqlExport() {
    std::vector<Transaction> a = {T("ACC1", "MRC2", "2026-03-05 14:07:09", 10), T("ACC1", "ACC3", "2026-03-05 14:08:09", 99)};
    a[0].id = "TX1"; a[1].id = "TX2"; a[1].merchant = "O'Neil";
    std::vector<Signals> sig(2);
    sig[1].hitRule("amount spike", 0.5); sig[1].risk = 0.9; sig[1].level = "HIGH";
    CHECK(writeSqlScript("_t.sql", a, sig, 0.5));
    std::FILE* f = std::fopen("_t.sql", "r");
    std::string text; char buf[512];
    while (f && std::fgets(buf, sizeof buf, f)) text += buf;
    if (f) std::fclose(f);
    CHECK(text.find("INSERT INTO alerts") != std::string::npos && text.find("'TX2'") != std::string::npos);
    CHECK(text.find("O\\'Neil") != std::string::npos);                // quote escaped
    CHECK(text.find("COMMIT;") != std::string::npos);
    std::remove("_t.sql");
}

static void testEngineSize() { CHECK(RuleEngine::withDefaultRules().size() == 6); }

int main() {
    testTime(); testAmountSpike(); testRepeated(); testOddHour(); testCycle(); testFan(); testScorer(); testForest();
    testVelocity(); testNewPayee(); testStructuring(); testUnionFind(); testClusters(); testJsonRoundTrip(); testConfig(); testSqlExport(); testEngineSize();
    if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
    std::cout << "All tests passed\n";
    return 0;
}
