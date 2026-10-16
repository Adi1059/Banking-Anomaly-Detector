# Banking Anomaly Detector (C++ & Python)

**PBL Project – Phase II (Working Prototype)**  
Team: DSCPP-III-2026-T249  
Graphic Era (Deemed to be University)

Hybrid transaction anomaly detection for banking data, built for an **OOP + Data Structures** course project.
Three independent detectors — **rules** (six configurable rules), **graph analysis** and **machine learning** — are fused into one
explainable risk score.

```
transactions -> RuleEngine ------------+
             -> GraphDetector ---------+---> HybridScorer --> risk 0-1, level, reasons
             -> MLDetector (Isolation Forest, from scratch)
```

## Features (Current Status ~70-75%)

- 6 configurable detection rules (Amount Spike, Repeated Tx, Odd Hour, High Velocity, New Payee, Structuring)
- Graph analysis (cycle detection + fan-in/fan-out)
- Explainable alerts with severity levels and reasons
- Hybrid risk scoring
- Synthetic data generator
- CSV input/output
- Clean modular design (easy to explain)

## Project Structure

```
cpp/
├── include/          # Header files (one concept per file)
│   ├── types.hpp     # Core data structures
│   ├── rules.hpp     # Rule hierarchy (OOP)
│   ├── graph.hpp     # Graph-based detection
│   ├── scorer.hpp    # Hybrid scoring
│   ├── generator.hpp # Synthetic data
│   └── io.hpp        # File loading/saving
├── src/              # Implementations
├── config/           # Thresholds
├── data/             # Sample data
└── Makefile
```

## Build and Run

```bash
cd cpp
make              # builds ./detector
make test         # unit tests
./detector        # generates data + runs detection + prints report
./detector generate     # only generate synthetic data
./detector detect       # run on existing data
```

Windows (g++ from MinGW-w64):
```bash
build.bat
detector.exe
```

Other commands (CSV or JSON chosen from file extension):
```bash
detector generate data/demo_transactions.json
detector detect data/demo_transactions.csv data/scored.json --config config/default.json
detector sql data/demo_transactions.csv data/load_alerts.sql
```

`detect` also works on your own file with columns `account_id, receiver_id, timestamp, amount`.

## Rules (all thresholds in `config/default.json`)

| Rule | Idea | Data Structure |
|---|---|---|
| Amount spike | robust z-score (median / MAD) per account | hash map |
| Repeated transaction | same amount repeated in a short window | hash map + two pointers |
| Odd hour | activity between 00:00 and 05:00 | — |
| High velocity | >= 6 transactions by one account in 10 min | deque sliding window |
| Large transfer to new payee | first payment to a receiver, >= 5x the account median | hash set + hash map |
| Structuring | >= 3 payments in [90%, 100%) of the 10,000 limit within 24 h | deque sliding window |

Graph module: DFS cycle detection, fan-in / fan-out windows, and **Union-Find** clusters of accounts linked by flagged transfers on the same day.

Persistence: `sql/schema.sql` (accounts, transactions, alerts, ranked-alerts view) and `sql/queries.sql`; `detector sql` exports a MySQL load script.

## OOP Concepts Used

| Concept | Where |
|---|---|
| Abstraction / inheritance / polymorphism | `Rule` (abstract) → `AmountSpikeRule`, `RepeatedTransactionRule`, `OddHourRule`; `RuleEngine` calls them through base pointers |
| Template-method pattern | `Rule::apply()` calls the virtual `detect()` |
| Encapsulation / single responsibility | one class per job: `GraphDetector`, `IsolationForest`, `MLDetector`, `HybridScorer`, `TransactionGenerator` |
| RAII / smart pointers | `std::unique_ptr<Rule>` owned by `RuleEngine` |

## Data Structures & Algorithms Used

| Structure / Algorithm | Where |
|---|---|
| Deque (`std::deque`) | velocity and structuring sliding windows |
| Union-Find (path halving + union by size) | `UnionFind`, account clusters in `GraphDetector` |
| Threads (`std::async`, `-pthread`) | `RuleEngine::run` evaluates all rules concurrently, results applied in order (deterministic) |
| Hash map (`unordered_map`) | per-account statistics, group-by (account, amount), distinct-counterparty counts |
| Sliding window / two pointers | repeated-transaction rule, fan-in / fan-out, hourly velocity features |
| Graph as adjacency list + DFS (explicit stack) | cycle detection A→B→C→A with time and amount constraints |
| Binary trees stored in arrays | Isolation Forest (100 random trees, path-length scoring) |
| Min-heap (`priority_queue`) | top-K riskiest transactions |
| Sorting / median / MAD / percentiles | robust amount-spike statistic, ML score scaling |

## Scoring

`risk = 1 - (1 - a*rule)(1 - b*graph)(1 - c*ml)` with rule/graph scores combined by noisy-OR.
One weak signal stays LOW; a strong signal or agreement between detectors becomes an alert (risk >= 0.5).

## Sample Result (synthetic data, 3,850 transactions, 216 injected anomalies, 9 anomaly types)

| Method | Precision | Recall | F1 |
|---|---|---|---|
| Rules only | 0.78 | 0.93 | 0.85 |
| Graph only | 1.00 | 0.49 | 0.66 |
| ML only | 0.98 | 0.56 | 0.71 |
| Hybrid | 0.98 | 0.99 | 0.98 |

Runtime: about 75,000 transactions in 0.85 s. The data is synthetic and the thresholds were tuned on it, so real-world numbers would be lower.

## For the PBL Meeting

This code is intentionally modular so you can explain:

1. **OOP** → `Rule` abstract base class + inheritance + polymorphism
2. **Data Structures** → `unordered_map`, `deque`, adjacency list, priority queue
3. **Algorithms** → Sliding window, DFS cycle detection, robust statistics (median/MAD)
4. **Design** → Separation of concerns (each module has one job)

## Next Steps (remaining 25-30%)

- Full Isolation Forest (ML detector)
- JSON config loading
- SQL export
- Unit tests expansion
- Report exporter (Markdown/PDF)
- Live dashboard (optional)

## Roadmap

Live MySQL connector (C API), alert dashboard, Observer-style alert publishers, real datasets (PaySim), parallel processing, Big-O write-up.
