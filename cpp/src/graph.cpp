// ============================================================
// graph.cpp  –  Graph-based detection (cycles + fan-in/out)
// ============================================================

#include "graph.hpp"
#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ------------------------------------------------------------
// Cycle detection with DFS (explicit stack to avoid recursion limits)
// Looks for simple cycles of length 2..maxCycleLen
// ------------------------------------------------------------
std::vector<bool> GraphDetector::findCycles(const std::vector<Transaction>& tx) const {
    std::vector<bool> flags(tx.size(), false);
    if (tx.empty()) return flags;

    // Sort by time so we only consider forward edges in time
    std::vector<size_t> order(tx.size());
    for (size_t i = 0; i < tx.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](size_t a, size_t b) { return tx[a].timestamp < tx[b].timestamp; });

    // Adjacency: sender → list of (receiver, tx_index)
    std::unordered_map<std::string, std::vector<std::pair<std::string, size_t>>> adj;
    for (size_t idx : order) {
        adj[tx[idx].sender].emplace_back(tx[idx].receiver, idx);
    }

    // For each starting transaction, try to find a cycle back to the sender
    for (size_t startIdx : order) {
        const std::string& startAcc = tx[startIdx].sender;
        const std::string& firstHop = tx[startIdx].receiver;

        // DFS state: current account, path of transaction indices, depth
        struct Frame {
            std::string account;
            std::vector<size_t> path;
        };

        std::vector<Frame> stack;
        stack.push_back({firstHop, {startIdx}});

        while (!stack.empty()) {
            Frame cur = stack.back();
            stack.pop_back();

            if ((int)cur.path.size() > maxCycleLen_) continue;

            // Look at outgoing edges from current account
            auto it = adj.find(cur.account);
            if (it == adj.end()) continue;

            for (const auto& edge : it->second) {
                const auto& nextAcc = edge.first;
                const auto& nextIdx = edge.second;
                // Only consider edges that happen after the last one in the path
                if (tx[nextIdx].timestamp <= tx[cur.path.back()].timestamp) continue;
                if (tx[nextIdx].timestamp - tx[cur.path.front()].timestamp > windowSec_) continue;

                // Found a cycle?
                if (nextAcc == startAcc && cur.path.size() >= 2) {
                    for (size_t p : cur.path) flags[p] = true;
                    flags[nextIdx] = true;
                    break; // one cycle is enough for this start
                }

                // Continue DFS
                if (cur.path.size() + 1 <= (size_t)maxCycleLen_) {
                    auto newPath = cur.path;
                    newPath.push_back(nextIdx);
                    stack.push_back({nextAcc, newPath});
                }
            }
        }
    }
    return flags;
}

// ------------------------------------------------------------
// Fan-out: one account pays many distinct receivers quickly
// ------------------------------------------------------------
std::vector<bool> GraphDetector::findFanOut(const std::vector<Transaction>& tx) const {
    std::vector<bool> flags(tx.size(), false);
    const int threshold = 6;  // 6 different receivers in the window

    std::unordered_map<std::string, std::vector<size_t>> bySender;
    for (size_t i = 0; i < tx.size(); ++i) {
        bySender[tx[i].sender].push_back(i);
    }

    for (auto& kv : bySender) {
        auto& indices = kv.second;
        // Sliding window of distinct receivers
        std::deque<size_t> window;
        std::unordered_map<std::string, int> count;

        for (size_t idx : indices) {
            while (!window.empty() &&
                   tx[idx].timestamp - tx[window.front()].timestamp > windowSec_) {
                std::string oldRecv = tx[window.front()].receiver;
                if (--count[oldRecv] == 0) count.erase(oldRecv);
                window.pop_front();
            }
            window.push_back(idx);
            count[tx[idx].receiver]++;

            if ((int)count.size() >= threshold) {
                for (size_t j : window) flags[j] = true;
            }
        }
    }
    return flags;
}

// ------------------------------------------------------------
// Fan-in: many distinct senders pay one account quickly
// ------------------------------------------------------------
std::vector<bool> GraphDetector::findFanIn(const std::vector<Transaction>& tx) const {
    std::vector<bool> flags(tx.size(), false);
    const int threshold = 6;

    std::unordered_map<std::string, std::vector<size_t>> byReceiver;
    for (size_t i = 0; i < tx.size(); ++i) {
        byReceiver[tx[i].receiver].push_back(i);
    }

    for (auto& kv : byReceiver) {
        auto& indices = kv.second;
        std::deque<size_t> window;
        std::unordered_map<std::string, int> count;

        for (size_t idx : indices) {
            while (!window.empty() &&
                   tx[idx].timestamp - tx[window.front()].timestamp > windowSec_) {
                std::string oldSend = tx[window.front()].sender;
                if (--count[oldSend] == 0) count.erase(oldSend);
                window.pop_front();
            }
            window.push_back(idx);
            count[tx[idx].sender]++;

            if ((int)count.size() >= threshold) {
                for (size_t j : window) flags[j] = true;
            }
        }
    }
    return flags;
}

// ------------------------------------------------------------
// Main entry
// ------------------------------------------------------------
void GraphDetector::detect(const std::vector<Transaction>& tx, std::vector<Signals>& signals) const {
    auto cycles  = findCycles(tx);
    auto fanOut  = findFanOut(tx);
    auto fanIn   = findFanIn(tx);

    for (size_t i = 0; i < tx.size(); ++i) {
        if (cycles[i])  signals[i].addGraphHit("cycle", 0.70);
        if (fanOut[i])  signals[i].addGraphHit("fan_out", 0.50);
        if (fanIn[i])   signals[i].addGraphHit("fan_in", 0.50);
    }
}
