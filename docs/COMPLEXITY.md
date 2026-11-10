# Big-O Analysis

Notation: **n** = number of transactions, **k** = transactions of one account (sum of all k = n),
**T** = trees in the Isolation Forest (100), **psi** = sub-sample size per tree (256),
**F** = features per transaction (8), **w** = transactions inside one time window, **A** = number of alerts, **K** = size of the ranked table.

All times below are for the code in `cpp/src/`. Sorting by timestamp is done inside every detector, so the input order never matters
(verified by `testInputOrderDoesNotMatter`).

## 1. Rules (`rules.cpp`) - each rule is one `std::async` task

| Rule | Data structure | Time | Why |
|---|---|---|---|
| Amount spike | hash map account -> amounts | O(n log n) | grouping O(n); one sort per account for the median and for the MAD, sum of k log k <= n log n |
| Repeated transaction | hash map (account, amount) -> indices, two pointers | O(n log n) | grouping O(n); sort inside each group; each pointer only moves forward |
| Odd hour | none | O(n) | one modulo per transaction |
| High velocity | hash map + `std::deque` window | O(n log n) + O(n w) worst case | sort per account; each element enters and leaves the deque once; flagging all window members costs O(w) per step only inside a burst |
| Large transfer to new payee | hash map, hash set of (sender, receiver) | O(n log n) | medians per account + one global sort by time; set insert is O(1) expected |
| Structuring | hash map + `std::deque` | O(m log m), m <= n | only amounts inside [90%, 100%) of the limit are considered |

`RuleEngine::run`: six tasks run concurrently, so wall-clock time is about the slowest rule; total work is the sum.
Space: O(n) for the flag vectors plus O(n) for the maps.

## 2. Graph module (`graph.cpp`, `unionfind.hpp`)

| Part | Technique | Time |
|---|---|---|
| Fan-in / fan-out | sort per account + sliding window with a hash map of distinct counterparties | O(n log n) |
| Cycle detection (A->B->C->A) | adjacency list + DFS with an explicit stack; time window 2 h, amount tolerance 25 %, maximum cycle length 4 | O(n * B) worst case, B = 20,000 (per-start-edge search budget); much smaller in practice because the time window and amount tolerance prune most edges |
| Account clusters | Union-Find (path halving, union by size), one structure per calendar day | O(n alpha(n)), alpha = inverse Ackermann, below 5 for any practical n |

Without the budget a plain DFS on a dense transfer graph can grow exponentially in the path length;
the length cap, time window, amount tolerance and budget are what keep it bounded.
Space: O(n) for the adjacency list; each DFS frame copies a path of at most 4 indices.

## 3. Machine learning (`ml.cpp`)

| Step | Time |
|---|---|
| Feature building (8 features, one pass per account after a sort) | O(n log n) |
| Forest training: shuffle of the index vector per tree + building a tree on psi points of depth <= log2(psi) = 8 | O(T n + T psi log psi) |
| Scoring: each transaction follows one root-to-leaf path in every tree | O(n T log psi) |
| Converting scores to [0,1] with percentiles (one sort) | O(n log n) |

T, psi and F are constants, so the ML detector is O(n log n) overall. Space: O(n F) for the feature matrix plus O(T psi) for the forest.

## 4. Scoring and ranking

* `HybridScorer::score`: O(n) - a constant-time noisy-OR per transaction.
* Top-K ranking with a min-heap (`std::priority_queue`): O(n log K) to scan n transactions; O(K log K) to print.
* `AlertPublisher::publish`: O(n + A * o) for o observers; `ConsoleRankedTable` costs O(log K) per alert.

## 5. Whole pipeline

Dominated by the sorts: **O(n log n)** time and **O(n)** space for realistic inputs (worst case is higher only through the
DFS budget B and bursts w, both bounded by constants in the configuration).

Measured on a 2-core Linux machine (g++ 13, -O2), synthetic data, median of 5 runs:

| Transactions | Pipeline time | Hybrid precision / recall |
|---|---|---|
| 3,850 | 0.027 s | 0.98 / 0.99 |
| 19,073 | 0.124 s | 0.98 / 0.99 |
| 75,884 | 0.488 s | 0.98 / 1.00 |
| 303,087 | 2.144 s | 0.96 / 0.99 |

A 5x larger input takes roughly 4-5x longer, which is consistent with O(n log n). Absolute times depend on the machine.
