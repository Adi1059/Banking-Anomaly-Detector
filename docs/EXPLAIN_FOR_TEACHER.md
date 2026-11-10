# How to Explain This Project (PBL viva notes)

## 1. Architecture in 30 seconds
```
CSV / JSON  ->  RuleEngine (6 rules, parallel)  --+
            ->  GraphDetector (DFS, fan, Union-Find) --+-->  HybridScorer  -->  risk, level, reasons
            ->  MLDetector (Isolation Forest)  --------+        -> ranked alerts, CSV/JSON, SQL script, Observer publishers
```
Three independent detectors look at the same transactions from different angles; the scorer combines their opinions with a
noisy-OR, so one weak signal stays LOW but two agreeing detectors raise an alert.

## 2. OOP (Data Structures + OOP with C++)
* `Rule` is abstract (`include/rules.hpp`); six subclasses override `detect()`. `RuleEngine` owns them as
  `std::unique_ptr<Rule>` and calls them through the base pointer - polymorphism and RAII.
* `Rule::apply()` is the template method; the subclass supplies only the detection step.
* `AlertPublisher` / `AlertObserver` (`include/alert_publisher.hpp`) is the Observer pattern: the pipeline publishes each alert and
  any number of observers (ranked console table, CSV log) react without the pipeline knowing about them.

## 3. Data structures - where and why
| Structure | Where | Why |
|---|---|---|
| `unordered_map` / `unordered_set` | rules, graph, features | O(1) expected per-account statistics and "seen payee" checks |
| `deque` | velocity, structuring | sliding window: push at the back, pop expired entries at the front |
| Adjacency list + DFS (explicit stack) | `graph.cpp` | money-flow graph; finds A->B->C->A cycles |
| Union-Find | `unionfind.hpp` | groups accounts linked by flagged transfers, near-constant time per operation |
| Binary trees in arrays | `ml.cpp` | Isolation Forest trees stored as vectors of nodes |
| Min-heap (`priority_queue`) | `main.cpp`, `alert_publisher.cpp` | keep only the K riskiest alerts in O(log K) each |

## 4. Key algorithms
1. **Robust z-score** (median and MAD) for amount spikes - not distorted by the spikes themselves.
2. **Sliding window** with a deque or two pointers for velocity, repeated payments, structuring and fan-in/out.
3. **DFS cycle search** limited by time window (2 h), amount tolerance (25 %), length (4) and a per-edge budget.
4. **Isolation Forest**: anomalies are isolated in fewer random splits; score = 2^(-average path length / c(psi)).
5. **Noisy-OR** fusion: risk = 1 - (1 - a*rule)(1 - b*graph)(1 - c*ml).

## 5. OS concept
`RuleEngine::run` starts one `std::async` task per rule. Each task only reads the shared, immutable transaction list; results are
merged afterwards on one thread in a fixed order, so there is no lock and the output is deterministic.

## 6. DBMS concept
`sql/schema.sql`: accounts, transactions, alerts with primary/foreign keys, indexes, CHECK constraint and a ranked-alerts view;
`sql/queries.sql`: analytical queries; `detector sql` exports an INSERT script. (No live connector yet - say so openly.)

## 7. Honest answers to likely questions
* *Is 0.98 precision realistic?* No - the data is synthetic and the thresholds were tuned on it; real data would be lower.
* *What did the hybrid add?* Rules alone: F1 0.85. Graph alone: 0.66. ML alone: 0.71. Hybrid: 0.98 - each covers different anomaly types.
* *What is missing?* Live MySQL connector, PaySim validation, weight calibration (1/5 amount spikes and 2/5 new-payee cases are missed).
* *Complexity?* O(n log n) overall; see `docs/COMPLEXITY.md`.
