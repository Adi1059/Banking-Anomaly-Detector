# Banking Anomaly Detector (C++17)

Hybrid transaction anomaly detection for banking data, built for an **OOP + Data Structures** course project.
Three independent detectors - **rules** (six configurable rules), **graph analysis** and **machine learning** - are fused into one
explainable risk score.

```
transactions -> RuleEngine ------------+
             -> GraphDetector ---------+--> HybridScorer --> risk 0-1, level, reasons
             -> MLDetector (Isolation Forest, from scratch)
```

## Build and run (folder `cpp/`)
```bash
cd cpp
# Linux / macOS / MSYS2
make            # builds ./detector
make test       # unit tests
./detector      # generates data/demo_transactions.csv, scores it, prints the report
# Windows (g++ from MinGW-w64)
build.bat
detector.exe
test_core.exe
```
Other commands (CSV or JSON is chosen from the file extension):
```bash
detector generate data/demo_transactions.json
detector detect data/demo_transactions.csv data/scored.json --config config/default.json
detector sql data/demo_transactions.csv data/load_alerts.sql     # then: mysql < sql/schema.sql ; mysql < data/load_alerts.sql
```
`detect` also works on your own file with columns `account_id, receiver_id, timestamp, amount`.

## Rules (all thresholds in `config/default.json`)
| Rule | Idea | Data structure |
|---|---|---|
| Amount spike | robust z-score (median / MAD) per account | hash map |
| Repeated transaction | same amount repeated in a short window | hash map + two pointers |
| Odd hour | activity between 00:00 and 05:00 | - |
| High velocity | >= 6 transactions by one account in 10 min | deque sliding window |
| Large transfer to new payee | first payment to a receiver, >= 5x the account median | hash set + hash map |
| Structuring | >= 3 payments in [90%, 100%) of the 10,000 limit within 24 h | deque sliding window |

Graph module: DFS cycle detection, fan-in / fan-out windows, and **Union-Find** clusters of accounts linked by flagged transfers on the same day.
Persistence: `sql/schema.sql` (accounts, transactions, alerts, ranked-alerts view) and `sql/queries.sql`; `detector sql` exports a MySQL load script.

## OOP concepts used
| Concept | Where |
|---|---|
| Abstraction / inheritance / polymorphism | `Rule` (abstract) -> `AmountSpikeRule`, `RepeatedTransactionRule`, `OddHourRule`; `RuleEngine` calls them through base pointers |
| Template-method pattern | `Rule::apply()` calls the virtual `detect()` |
| Encapsulation / single responsibility | one class per job: `GraphDetector`, `IsolationForest`, `MLDetector`, `HybridScorer`, `TransactionGenerator` |
| RAII / smart pointers | `std::unique_ptr<Rule>` owned by `RuleEngine` |

## Data structures and algorithms used
| Structure / algorithm | Where |
|---|---|
| Deque (`std::deque`) | velocity and structuring sliding windows |
| Union-Find (path halving + union by size) | `UnionFind`, account clusters in `GraphDetector` |
| Hash map (`unordered_map`) | per-account statistics, group-by (account, amount), distinct-counterparty counts |
| Sliding window / two pointers | repeated-transaction rule, fan-in / fan-out, hourly velocity features |
| Graph as adjacency list + DFS (explicit stack) | cycle detection A->B->C->A with time and amount constraints |
| Binary trees stored in arrays | Isolation Forest (100 random trees, path-length scoring) |
| Min-heap (`priority_queue`) | top-K riskiest transactions |
| Sorting / median / MAD / percentiles | robust amount-spike statistic, ML score scaling |

## Scoring
`risk = 1 - (1 - a*rule)(1 - b*graph)(1 - c*ml)` with rule/graph scores combined by noisy-OR.
One weak signal stays LOW; a strong signal or agreement between detectors becomes an alert (risk >= 0.5).

## Sample result (synthetic data, 3,850 transactions, 216 injected anomalies, 9 anomaly types)
| Method | Precision | Recall | F1 |
|---|---|---|---|
| Rules only | 0.78 | 0.93 | 0.85 |
| Graph only | 1.00 | 0.49 | 0.66 |
| ML only | 0.98 | 0.56 | 0.71 |
| Hybrid | 0.98 | 0.99 | 0.98 |

Runtime: about 75,000 transactions in 0.85 s. The data is synthetic and the thresholds were tuned on it, so real-world numbers would be lower.

## Roadmap
Live MySQL connector (C API), alert dashboard, Observer-style alert publishers, real datasets (PaySim), parallel processing, Big-O write-up.
