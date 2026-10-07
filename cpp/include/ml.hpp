#pragma once
// Unsupervised ML detector: Isolation Forest written from scratch (random binary trees).
#include <array>
#include <random>
#include <vector>
#include "types.hpp"

class IsolationForest {
public:
    explicit IsolationForest(int trees = 100, size_t sampleSize = 256, unsigned seed = 42)
        : numTrees_(trees), sampleSize_(sampleSize), rng_(seed) {}
    void fit(const std::vector<std::vector<double>>& X);
    double score(const std::vector<double>& x) const;   // ~0.5 normal ... ->1 anomalous
private:
    struct Node { int feature = -1; double split = 0.0; int left = -1, right = -1; size_t size = 0; };
    using Tree = std::vector<Node>;                      // tree stored as an array of nodes
    int build(Tree& t, std::vector<size_t>& idx, size_t lo, size_t hi, int depth, int maxDepth,
              const std::vector<std::vector<double>>& X);
    double pathLength(const Tree& t, const std::vector<double>& x) const;
    static double c(double n);                           // average path length of unsuccessful BST search

    int numTrees_;
    size_t sampleSize_, psi_ = 0;
    std::mt19937 rng_;
    std::vector<Tree> forest_;
};

// Builds behavioural features per transaction and converts forest scores to ml in [0,1].
class MLDetector {
public:
    static constexpr size_t kFeatures = 8;
    static std::vector<std::vector<double>> buildFeatures(const std::vector<Transaction>& tx);
    void detect(const std::vector<Transaction>& tx, std::vector<Signals>& sig) const;
};
