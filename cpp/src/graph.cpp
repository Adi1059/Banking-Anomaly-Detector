#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "unionfind.hpp"

bool GraphDetector::eligible(const Transaction& t) {
    return t.sender != t.receiver && t.receiver.rfind("MRC", 0) != 0;
}

void GraphDetector::detect(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const {
    const auto cyc = findCycles(tx), fo = findFan(tx, true), fi = findFan(tx, false);
    for (size_t i = 0; i < tx.size(); ++i) {
        if (cyc[i]) sig[i].hitGraph("graph: cycle", 0.90);
        if (fo[i])  sig[i].hitGraph("graph: fan out", 0.70);
        if (fi[i])  sig[i].hitGraph("graph: fan in", 0.70);
    }
    std::vector<char> any(tx.size(), 0);
    for (size_t i = 0; i < tx.size(); ++i) any[i] = cyc[i] || fo[i] || fi[i];
    const auto sizes = clusterSizes(tx, any);
    for (size_t i = 0; i < tx.size(); ++i)
        if (sizes[i] >= clusterMin_) sig[i].hitGraph("graph: linked cluster of " + std::to_string(sizes[i]) + " accounts", 0.30);
}

// Accounts joined by flagged transfers on the same calendar day form one cluster (Union-Find per day).
std::vector<size_t> GraphDetector::clusterSizes(const std::vector<Transaction>& tx, const std::vector<char>& flagged) const {
    std::unordered_map<int64_t, std::vector<size_t>> byDay;          // hash map: day -> flagged tx indices
    for (size_t i = 0; i < tx.size(); ++i)
        if (flagged[i]) byDay[tx[i].ts / 86400].push_back(i);

    std::vector<size_t> out(tx.size(), 0);
    for (const auto& kv : byDay) {
        std::unordered_map<std::string, size_t> id;                  // account -> dense id
        auto idOf = [&](const std::string& a) { return id.emplace(a, id.size()).first->second; };
        for (size_t i : kv.second) { idOf(tx[i].sender); idOf(tx[i].receiver); }
        UnionFind uf(id.size());
        for (size_t i : kv.second) uf.unite(id[tx[i].sender], id[tx[i].receiver]);
        for (size_t i : kv.second) out[i] = uf.sizeOf(id[tx[i].sender]);
    }
    return out;
}

// Depth-first search along time-ordered edges that returns to the starting account.
std::vector<char> GraphDetector::findCycles(const std::vector<Transaction>& tx) const {
    std::unordered_map<std::string, std::vector<size_t>> out;        // adjacency list: account -> outgoing tx
    for (size_t i = 0; i < tx.size(); ++i)
        if (eligible(tx[i])) out[tx[i].sender].push_back(i);

    struct Frame { std::string node; int64_t lastTs; std::vector<size_t> path; };
    std::vector<char> flagged(tx.size(), 0);

    for (size_t i = 0; i < tx.size(); ++i) {
        if (!eligible(tx[i])) continue;
        const Transaction& t0 = tx[i];
        std::vector<Frame> stack;                                    // explicit DFS stack
        stack.push_back({t0.receiver, t0.ts, {i}});
        size_t budget = 20000;                                       // safety cap per start edge
        while (!stack.empty() && budget-- > 0) {
            Frame f = std::move(stack.back());
            stack.pop_back();
            if (f.path.size() >= maxCycleLen_) continue;
            auto it = out.find(f.node);
            if (it == out.end()) continue;
            for (size_t j : it->second) {
                const Transaction& t = tx[j];
                if (t.ts < f.lastTs || t.ts - t0.ts > windowSec_) continue;
                if (std::find(f.path.begin(), f.path.end(), j) != f.path.end()) continue;
                if (std::fabs(t.amount - t0.amount) > tol_ * t0.amount) continue;
                if (t.receiver == t0.sender) {
                    for (size_t p : f.path) flagged[p] = 1;
                    flagged[j] = 1;
                } else {
                    Frame nf{t.receiver, t.ts, f.path};
                    nf.path.push_back(j);
                    stack.push_back(std::move(nf));
                }
            }
        }
    }
    return flagged;
}

// Sliding window over each account's transfers, counting distinct counterparties in a hash map.
std::vector<char> GraphDetector::findFan(const std::vector<Transaction>& tx, bool bySender) const {
    std::unordered_map<std::string, std::vector<size_t>> groups;
    for (size_t i = 0; i < tx.size(); ++i)
        if (eligible(tx[i])) groups[bySender ? tx[i].sender : tx[i].receiver].push_back(i);

    std::vector<char> flagged(tx.size(), 0);
    for (auto& kv : groups) {
        auto& idx = kv.second;
        if (idx.size() < fanThreshold_) continue;
        std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return tx[a].ts < tx[b].ts; });
        std::unordered_map<std::string, int> counts;
        size_t left = 0;
        for (size_t right = 0; right < idx.size(); ++right) {
            counts[bySender ? tx[idx[right]].receiver : tx[idx[right]].sender]++;
            while (tx[idx[right]].ts - tx[idx[left]].ts > windowSec_) {
                const std::string& o = bySender ? tx[idx[left]].receiver : tx[idx[left]].sender;
                if (--counts[o] == 0) counts.erase(o);
                ++left;
            }
            if (counts.size() >= fanThreshold_)
                for (size_t k = left; k <= right; ++k) flagged[idx[k]] = 1;
        }
    }
    return flagged;
}
