# Banking Anomaly Detector (C++17)

Hybrid transaction anomaly detection for banking data, built for an **OOP + Data Structures** course project.
Three independent detectors - **rules**, **graph analysis** and **machine learning** - are fused into one
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
Other commands: `detector generate [out.csv]`, `detector detect [in.csv] [out.csv]` (works on your own CSV with
columns `account_id, receiver_id, timestamp, amount`).

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
| Hash map (`unordered_map`) | per-account statistics, group-by (account, amount), distinct-counterparty counts |
| Sliding window / two pointers | repeated-transaction rule, fan-in / fan-out, hourly velocity features |
| Graph as adjacency list + DFS (explicit stack) | cycle detection A->B->C->A with time and amount constraints |
| Binary trees stored in arrays | Isolation Forest (100 random trees, path-length scoring) |
| Min-heap (`priority_queue`) | top-K riskiest transactions |
| Sorting / median / MAD / percentiles | robust amount-spike statistic, ML score scaling |

## Scoring
`risk = 1 - (1 - a*rule)(1 - b*graph)(1 - c*ml)` with rule/graph scores combined by noisy-OR.
One weak signal stays LOW; a strong signal or agreement between detectors becomes an alert (risk >= 0.5).

## Sample result (synthetic data, 3,779 transactions, 145 injected anomalies)
| Method | Precision | Recall | F1 |
|---|---|---|---|
| Rules only | 0.71 | 0.95 | 0.81 |
| Graph only | 0.97 | 0.79 | 0.87 |
| ML only | 0.96 | 0.55 | 0.70 |
| Hybrid | 0.94 | 0.99 | 0.96 |

The data is synthetic and the thresholds were tuned on it, so real-world numbers would be lower.

## Roadmap
Dashboard, more rules, real datasets (PaySim), parallel processing.
