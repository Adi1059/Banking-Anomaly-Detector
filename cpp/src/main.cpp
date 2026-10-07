// Banking Anomaly Detector - hybrid (rules + graph + ML) in C++17.
//   detector generate [out.csv]            create synthetic labelled data
//   detector detect [in.csv] [out.csv]     score transactions, print report
//   detector                               generate + detect with default paths
#include <chrono>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <map>
#include <queue>
#include <string>
#include "eval.hpp"
#include "generator.hpp"
#include "io.hpp"
#include "pipeline.hpp"

namespace fs = std::filesystem;

static void ensureParent(const std::string& path) {
    const fs::path p(path);
    if (p.has_parent_path()) fs::create_directories(p.parent_path());
}

static int doGenerate(const std::string& out) {
    ensureParent(out);
    auto tx = TransactionGenerator().generate();
    if (!writeTransactions(out, tx)) { std::cerr << "cannot write " << out << "\n"; return 1; }
    size_t inj = 0;
    for (const auto& t : tx) inj += t.injected;
    std::cout << "Wrote " << tx.size() << " transactions (" << inj << " injected anomalous) to " << out << "\n";
    return 0;
}

static void printMethod(const char* name, const Metrics& m) {
    std::cout << std::left << std::setw(26) << name << std::right << std::setw(8) << m.alerts << std::fixed << std::setprecision(2)
              << std::setw(11) << m.precision << std::setw(9) << m.recall << std::setw(7) << m.f1 << "\n";
}

static int doDetect(const std::string& in, const std::string& out) {
    std::vector<Transaction> tx;
    if (!readTransactions(in, tx)) { std::cerr << "cannot read " << in << " (run 'detector generate' first)\n"; return 1; }
    HybridScorer scorer;
    const auto t0 = std::chrono::steady_clock::now();
    const auto sig = runPipeline(tx, scorer);
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    ensureParent(out);
    writeScored(out, tx, sig, scorer.config().alert);

    std::map<std::string, int> levels;
    std::vector<char> rules(tx.size()), graph(tx.size()), ml(tx.size()), hybrid(tx.size());
    size_t alerts = 0;
    for (size_t i = 0; i < tx.size(); ++i) {
        ++levels[sig[i].level];
        rules[i] = sig[i].ruleHit; graph[i] = sig[i].graphHit; ml[i] = sig[i].ml >= 0.5; hybrid[i] = scorer.isAlert(sig[i]);
        alerts += hybrid[i];
    }
    std::cout << "Transactions: " << tx.size() << " | alerts: " << alerts << " | time: " << std::fixed << std::setprecision(2) << secs << "s\n";
    std::cout << "Risk levels:";
    for (const auto& kv : levels) std::cout << " " << kv.first << "=" << kv.second;
    std::cout << "\n\nMethod comparison (against injected labels)\n"
              << std::left << std::setw(26) << "method" << std::right << std::setw(8) << "alerts" << std::setw(11) << "precision"
              << std::setw(9) << "recall" << std::setw(7) << "F1" << "\n";
    printMethod("Rules only", evaluate(tx, rules));
    printMethod("Graph only", evaluate(tx, graph));
    printMethod("ML only", evaluate(tx, ml));
    printMethod("Hybrid", evaluate(tx, hybrid));

    std::map<std::string, std::pair<int, int>> byType;   // type -> (caught, total)
    for (size_t i = 0; i < tx.size(); ++i)
        if (tx[i].injected) { ++byType[tx[i].anomalyType].second; byType[tx[i].anomalyType].first += hybrid[i]; }
    std::cout << "\nHybrid recall by anomaly type\n";
    for (const auto& kv : byType)
        std::cout << "  " << std::left << std::setw(14) << kv.first << std::right << std::setw(4) << kv.second.first << "/"
                  << std::left << std::setw(4) << kv.second.second << std::right << " (" << std::setprecision(0)
                  << 100.0 * kv.second.first / kv.second.second << "%)\n";

    // Top-K risky transactions with a min-heap (priority_queue)
    using Item = std::pair<double, size_t>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;
    const size_t K = 5;
    for (size_t i = 0; i < tx.size(); ++i) {
        heap.push({sig[i].risk, i});
        if (heap.size() > K) heap.pop();
    }
    std::vector<Item> top;
    while (!heap.empty()) { top.push_back(heap.top()); heap.pop(); }
    std::cout << "\nTop " << K << " risky transactions\n";
    for (auto it = top.rbegin(); it != top.rend(); ++it) {
        const auto& t = tx[it->second];
        std::cout << "  " << t.id << " " << t.sender << "->" << t.receiver << " " << std::fixed << std::setprecision(2) << t.amount
                  << "  risk=" << it->first << " " << sig[it->second].level << "  [";
        for (size_t r = 0; r < sig[it->second].reasons.size(); ++r) std::cout << (r ? "; " : "") << sig[it->second].reasons[r];
        std::cout << "]\n";
    }
    std::cout << "\nScored file: " << out << "\n";
    return 0;
}

int main(int argc, char** argv) {
    const std::string cmd = argc > 1 ? argv[1] : "demo";
    if (cmd == "generate") return doGenerate(argc > 2 ? argv[2] : "data/demo_transactions.csv");
    if (cmd == "detect")
        return doDetect(argc > 2 ? argv[2] : "data/demo_transactions.csv", argc > 3 ? argv[3] : "data/scored_transactions.csv");
    if (cmd == "demo") {
        if (int rc = doGenerate("data/demo_transactions.csv")) return rc;
        std::cout << "\n";
        return doDetect("data/demo_transactions.csv", "data/scored_transactions.csv");
    }
    std::cerr << "usage: detector [generate [out] | detect [in] [out] | demo]\n";
    return 2;
}
