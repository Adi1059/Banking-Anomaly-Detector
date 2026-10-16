// ============================================================
// main.cpp  –  Entry point & simple CLI
// ============================================================
// Commands:
//   ./detector              → generate + detect (demo)
//   ./detector generate     → only create synthetic data
//   ./detector detect       → run detection on existing data
// ============================================================

#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <queue>
#include "rules.hpp"
#include "graph.hpp"
#include "scorer.hpp"
#include "generator.hpp"
#include "io.hpp"

void printHeader() {
    std::cout << "============================================================\n";
    std::cout << "  Banking Transaction Anomaly Detector  (PBL Phase-II)\n";
    std::cout << "  Team DSCPP-III-2026-T249\n";
    std::cout << "============================================================\n\n";
}

void runDetection(const std::vector<Transaction>& tx) {
    std::vector<Signals> signals(tx.size());

    // 1. Rule-based detection
    std::cout << "[1/3] Running rule engine (" << RuleEngine::createDefault().size()
              << " rules)...\n";
    auto rules = RuleEngine::createDefault();
    rules.run(tx, signals);

    // 2. Graph-based detection
    std::cout << "[2/3] Running graph analysis (cycles + fan-in/out)...\n";
    GraphDetector graph;
    graph.detect(tx, signals);

    // 3. Hybrid scoring
    std::cout << "[3/3] Computing hybrid risk scores...\n";
    HybridScorer scorer;
    scorer.score(signals);

    // ----- Summary -----
    int alerts = 0;
    std::map<std::string, int> levels;
    for (const auto& s : signals) {
        levels[s.level]++;
        if (scorer.isAlert(s)) alerts++;
    }

    std::cout << "\n---------------- RESULTS ----------------\n";
    std::cout << "Total transactions : " << tx.size() << "\n";
    std::cout << "Alerts raised      : " << alerts << "\n\n";

    std::cout << "Risk Level Distribution:\n";
    for (const auto& kv : levels) {
        std::cout << "  " << std::left << std::setw(10) << kv.first << " : " << kv.second << "\n";
    }

    // Top-5 riskiest (min-heap)
    using Item = std::pair<double, size_t>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;
    for (size_t i = 0; i < tx.size(); ++i) {
        heap.push({signals[i].risk, i});
        if (heap.size() > 5) heap.pop();
    }

    std::vector<Item> top;
    while (!heap.empty()) {
        top.push_back(heap.top());
        heap.pop();
    }

    std::cout << "\nTop 5 Riskiest Transactions:\n";
    for (auto it = top.rbegin(); it != top.rend(); ++it) {
        const auto& t = tx[it->second];
        const auto& s = signals[it->second];
        std::cout << "  " << t.id << "  " << t.sender << " → " << t.receiver
                  << "  ₹" << std::fixed << std::setprecision(2) << t.amount
                  << "  risk=" << s.risk << " [" << s.level << "]  reasons: ";
        for (size_t r = 0; r < s.reasons.size(); ++r) {
            if (r) std::cout << ", ";
            std::cout << s.reasons[r];
        }
        std::cout << "\n";
    }

    // Save results
    saveScoredCSV("data/scored_transactions.csv", tx, signals);
    std::cout << "\nScored file written to: data/scored_transactions.csv\n";
}

int main(int argc, char** argv) {
    printHeader();

    std::string cmd = (argc > 1) ? argv[1] : "demo";

    if (cmd == "generate" || cmd == "demo") {
        std::cout << "Generating synthetic transactions...\n";
        TransactionGenerator gen;
        auto tx = gen.generate(3500);
        // simple save
        std::ofstream out("data/demo_transactions.csv");
        out << "id,sender,receiver,amount,timestamp\n";
        for (const auto& t : tx) {
            out << t.id << "," << t.sender << "," << t.receiver << ","
                << t.amount << "," << t.timestamp << "\n";
        }
        out.close();
        std::cout << "Wrote " << tx.size() << " transactions to data/demo_transactions.csv\n\n";

        if (cmd == "generate") return 0;

        // continue to detection
        runDetection(tx);
        return 0;
    }

    if (cmd == "detect") {
        std::vector<Transaction> tx;
        if (!loadCSV("data/demo_transactions.csv", tx)) {
            std::cerr << "Could not load data/demo_transactions.csv\n";
            std::cerr << "Run './detector generate' first.\n";
            return 1;
        }
        runDetection(tx);
        return 0;
    }

    std::cerr << "Usage: ./detector [generate | detect | demo]\n";
    return 1;
}
