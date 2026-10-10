#pragma once
// Disjoint-set union (Union-Find) with path compression and union by size.
// Used by the graph detector to group accounts that are linked by suspicious transfers.
#include <numeric>
#include <vector>

class UnionFind {
public:
    explicit UnionFind(size_t n = 0) : parent_(n), size_(n, 1) { std::iota(parent_.begin(), parent_.end(), 0); }

    size_t find(size_t x) {
        while (parent_[x] != x) {            // iterative path halving
            parent_[x] = parent_[parent_[x]];
            x = parent_[x];
        }
        return x;
    }
    bool unite(size_t a, size_t b) {
        a = find(a); b = find(b);
        if (a == b) return false;
        if (size_[a] < size_[b]) std::swap(a, b);
        parent_[b] = a;
        size_[a] += size_[b];
        return true;
    }
    bool connected(size_t a, size_t b) { return find(a) == find(b); }
    size_t sizeOf(size_t x) { return size_[find(x)]; }
    size_t components() const {
        size_t c = 0;
        for (size_t i = 0; i < parent_.size(); ++i) c += parent_[i] == i;
        return c;
    }
private:
    std::vector<size_t> parent_, size_;
};
